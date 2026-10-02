#include "enku/storage/cbor_reader_checkpoint.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace enku;

namespace {

class MemoryStateFileStore final : public StateFileStore {
public:
    StateFileStatus read(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    ) override {
        if (read_error) {
            return StateFileStatus::IoError;
        }

        const auto it = files.find(path);
        if (it == files.end()) {
            return StateFileStatus::NotFound;
        }

        bytes = it->second;
        return StateFileStatus::Ok;
    }

    StateFileStatus write(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    ) override {
        ++writes;
        last_write_path = path;

        if (write_status != StateFileStatus::Ok) {
            return write_status;
        }

        files[path] = bytes;
        return StateFileStatus::Ok;
    }

    StateFileStatus remove(
        const std::string& path
    ) override {
        const auto erased = files.erase(path);
        return erased > 0
            ? StateFileStatus::Ok
            : StateFileStatus::NotFound;
    }

    std::map<std::string, std::vector<std::uint8_t>> files;
    StateFileStatus write_status{StateFileStatus::Ok};
    bool read_error{false};
    std::uint32_t writes{0};
    std::string last_write_path;
};

} // namespace

int main() {
    MemoryStateFileStore files;
    CborReaderCheckpointService checkpoints(files);

    ReaderCheckpoint restored;

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::NotFound
    );

    const SemanticPosition first{
        "book-1",
        "txt:body",
        120,
    };

    assert(
        checkpoints.checkpoint(
            "book-1",
            first,
            0.25F,
            ReadingState::Reading
        ) == PersistStatus::Ok
    );
    assert(files.writes == 1);

    const auto first_slot = files.last_write_path;

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::Ok
    );
    assert(restored.position.book_id == "book-1");
    assert(restored.position.section_id == "txt:body");
    assert(restored.position.text_offset == 120);
    assert(restored.progress == 0.25F);
    assert(restored.reading_state == ReadingState::Reading);

    const SemanticPosition second{
        "book-1",
        "txt:body",
        640,
    };

    assert(
        checkpoints.checkpoint(
            "book-1",
            second,
            0.75F,
            ReadingState::Reading
        ) == PersistStatus::Ok
    );
    assert(files.writes == 2);

    const auto second_slot = files.last_write_path;
    assert(second_slot != first_slot);

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::Ok
    );
    assert(restored.position.text_offset == 640);
    assert(restored.progress == 0.75F);

    // Corrupt the newest slot. Recovery must fall back to the previous
    // valid generation rather than losing the checkpoint entirely.
    assert(!files.files[second_slot].empty());
    files.files[second_slot].back() ^= 0xFFU;

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::Ok
    );
    assert(restored.position.text_offset == 120);
    assert(restored.progress == 0.25F);

    // The next successful write replaces the invalid slot and becomes the
    // newest generation again.
    const SemanticPosition third{
        "book-1",
        "txt:body",
        900,
    };

    assert(
        checkpoints.checkpoint(
            "book-1",
            third,
            1.0F,
            ReadingState::Finished
        ) == PersistStatus::Ok
    );

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::Ok
    );
    assert(restored.position.text_offset == 900);
    assert(restored.progress == 1.0F);
    assert(restored.reading_state == ReadingState::Finished);

    // Failed writes leave the last valid generation recoverable.
    files.write_status = StateFileStatus::NoSpace;

    const SemanticPosition failed{
        "book-1",
        "txt:body",
        1000,
    };

    assert(
        checkpoints.checkpoint(
            "book-1",
            failed,
            1.0F,
            ReadingState::Finished
        ) == PersistStatus::NoSpace
    );

    files.write_status = StateFileStatus::Ok;

    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::Ok
    );
    assert(restored.position.text_offset == 900);

    // Cross-book positions must never be persisted under another book id.
    assert(
        checkpoints.checkpoint(
            "book-2",
            SemanticPosition{"book-1", "txt:body", 10},
            0.1F,
            ReadingState::Reading
        ) == PersistStatus::InvalidRecord
    );

    // A transport-level read failure is distinguishable from no checkpoint.
    files.read_error = true;
    assert(
        checkpoints.load("book-1", restored) ==
        PersistStatus::IoError
    );

    return 0;
}
