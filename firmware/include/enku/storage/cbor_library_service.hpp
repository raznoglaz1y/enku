#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../services/services.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborLibraryService final : public LibraryService {
public:
    explicit CborLibraryService(StateFileStore& files);

    LibraryStatus load();
    LibraryStatus upsert(const BookRecord& record);

    std::optional<BookRecord> get(
        const BookId& book_id
    ) const override;

    LibraryStatus query(
        const LibraryQuery& query,
        LibraryPage& page
    ) const override;

    std::optional<BookId> findByFingerprint(
        const std::string& fingerprint
    ) const override;

    LibraryStatus updateSummary(
        const BookId& book_id,
        ReadingState reading_state,
        float progress,
        std::uint64_t last_opened_order
    ) override;

    const std::vector<BookRecord>& records() const;

private:
    struct DecodedIndex {
        std::uint32_t generation{0};
        std::vector<BookRecord> records;
    };

    StateFileStore& files_;
    std::vector<BookRecord> records_;
    std::uint32_t generation_{0};

    static constexpr const char* kSlotA =
        "/system/library.a.cbor";
    static constexpr const char* kSlotB =
        "/system/library.b.cbor";

    LibraryStatus commit();

    LibraryStatus readSlot(
        const std::string& path,
        DecodedIndex& index
    ) const;

    static std::vector<std::uint8_t> encode(
        std::uint32_t generation,
        const std::vector<BookRecord>& records
    );

    static bool decode(
        const std::vector<std::uint8_t>& bytes,
        DecodedIndex& index
    );
};

} // namespace enku
