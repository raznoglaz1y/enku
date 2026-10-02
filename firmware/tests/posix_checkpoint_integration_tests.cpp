#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace enku;

namespace {

std::vector<std::filesystem::path> stateFiles(
    const std::filesystem::path& root
) {
    std::vector<std::filesystem::path> result;
    const auto dir = root / "system" / "state";

    if (!std::filesystem::exists(dir)) {
        return result;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".cbor") {
            result.push_back(entry.path());
        }
    }

    return result;
}

void corruptLastByte(
    const std::filesystem::path& path
) {
    std::fstream file(
        path,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );
    assert(file);

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    assert(size > 0);

    file.seekg(size - std::streamoff(1));
    char value = 0;
    file.read(&value, 1);
    assert(file);

    value ^= static_cast<char>(0xFF);

    file.seekp(size - std::streamoff(1));
    file.write(&value, 1);
    file.flush();
    assert(file);
}

} // namespace

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-posix-checkpoint-test";

    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);

    {
        PosixStateFileStore files(root);
        CborReaderCheckpointService checkpoints(files);

        const SemanticPosition first{
            "physical-book",
            "txt:body",
            128,
        };

        assert(
            checkpoints.checkpoint(
                "physical-book",
                first,
                0.20F,
                ReadingState::Reading
            ) == PersistStatus::Ok
        );

        const SemanticPosition second{
            "physical-book",
            "txt:body",
            768,
        };

        assert(
            checkpoints.checkpoint(
                "physical-book",
                second,
                0.80F,
                ReadingState::Reading
            ) == PersistStatus::Ok
        );
    }

    auto files_on_disk = stateFiles(root);
    assert(files_on_disk.size() == 2);

    // Recreate both adapter and checkpoint service to prove restore does not
    // depend on in-memory state from the writer instance.
    {
        PosixStateFileStore files(root);
        CborReaderCheckpointService checkpoints(files);

        ReaderCheckpoint restored;
        assert(
            checkpoints.load(
                "physical-book",
                restored
            ) == PersistStatus::Ok
        );
        assert(restored.position.text_offset == 768);
        assert(restored.progress == 0.80F);
    }

    // The second write is normally the newest physical file. Corrupt it and
    // verify recovery from the previous valid A/B generation.
    //
    // If timestamp resolution makes both equal, corrupt either slot: the
    // service must still return whichever valid generation remains.
    std::filesystem::path damaged;
    for (const auto& file : files_on_disk) {
        if (file.filename().string().find(".b.cbor") !=
            std::string::npos) {
            damaged = file;
            break;
        }
    }
    assert(!damaged.empty());
    corruptLastByte(damaged);

    {
        PosixStateFileStore files(root);
        CborReaderCheckpointService checkpoints(files);

        ReaderCheckpoint restored;
        assert(
            checkpoints.load(
                "physical-book",
                restored
            ) == PersistStatus::Ok
        );

        assert(restored.position.text_offset == 128);
        assert(restored.progress == 0.20F);

        // The newest B slot was damaged, so the surviving A generation must
        // be selected and remain internally consistent.
        assert(restored.position.book_id == "physical-book");
        assert(restored.position.section_id == "txt:body");
        assert(restored.reading_state == ReadingState::Reading);
    }

    std::filesystem::remove_all(root, cleanup_error);
    return 0;
}
