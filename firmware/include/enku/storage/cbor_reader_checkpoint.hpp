#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../services/services.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborReaderCheckpointService final : public ReaderCheckpointService {
public:
    explicit CborReaderCheckpointService(StateFileStore& files);

    PersistStatus load(
        const BookId& book_id,
        ReaderCheckpoint& checkpoint
    ) override;

    PersistStatus checkpoint(
        const BookId& book_id,
        const SemanticPosition& position,
        float progress,
        ReadingState reading_state
    ) override;

    PersistStatus erase(
        const BookId& book_id
    );

private:
    struct DecodedRecord {
        std::uint32_t generation{0};
        ReaderCheckpoint checkpoint;
    };

    StateFileStore& files_;

    static std::string slotPath(
        const BookId& book_id,
        char slot
    );

    PersistStatus readSlot(
        const std::string& path,
        DecodedRecord& record
    );

    static std::vector<std::uint8_t> encode(
        std::uint32_t generation,
        const ReaderCheckpoint& checkpoint
    );

    static bool decode(
        const std::vector<std::uint8_t>& bytes,
        DecodedRecord& record
    );
};

} // namespace enku
