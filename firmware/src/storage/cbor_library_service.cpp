#include "enku/storage/cbor_library_service.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string_view>
#include <utility>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;
constexpr std::uint64_t kRecordTypeLibraryIndex = 0;
constexpr std::uint64_t kBookRecordFieldCount = 22;

void appendTypeValue(
    std::vector<std::uint8_t>& out,
    std::uint8_t major,
    std::uint64_t value
) {
    const auto prefix = static_cast<std::uint8_t>(major << 5U);

    if (value < 24U) {
        out.push_back(static_cast<std::uint8_t>(prefix | value));
    } else if (value <= 0xFFU) {
        out.push_back(static_cast<std::uint8_t>(prefix | 24U));
        out.push_back(static_cast<std::uint8_t>(value));
    } else if (value <= 0xFFFFU) {
        out.push_back(static_cast<std::uint8_t>(prefix | 25U));
        out.push_back(static_cast<std::uint8_t>(value >> 8U));
        out.push_back(static_cast<std::uint8_t>(value));
    } else if (value <= 0xFFFFFFFFULL) {
        out.push_back(static_cast<std::uint8_t>(prefix | 26U));
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(
                static_cast<std::uint8_t>(value >> shift)
            );
        }
    } else {
        out.push_back(static_cast<std::uint8_t>(prefix | 27U));
        for (int shift = 56; shift >= 0; shift -= 8) {
            out.push_back(
                static_cast<std::uint8_t>(value >> shift)
            );
        }
    }
}

void appendUnsigned(
    std::vector<std::uint8_t>& out,
    std::uint64_t value
) {
    appendTypeValue(out, 0, value);
}

void appendArray(
    std::vector<std::uint8_t>& out,
    std::uint64_t count
) {
    appendTypeValue(out, 4, count);
}

void appendText(
    std::vector<std::uint8_t>& out,
    const std::string& value
) {
    appendTypeValue(out, 3, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void appendBytes(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& value
) {
    appendTypeValue(out, 2, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void appendNull(std::vector<std::uint8_t>& out) {
    out.push_back(0xF6);
}

void appendBool(
    std::vector<std::uint8_t>& out,
    bool value
) {
    out.push_back(value ? 0xF5 : 0xF4);
}

void appendFloat32(
    std::vector<std::uint8_t>& out,
    float value
) {
    out.push_back(0xFA);

    std::uint32_t raw = 0;
    static_assert(sizeof(raw) == sizeof(value));
    std::memcpy(&raw, &value, sizeof(raw));

    out.push_back(static_cast<std::uint8_t>(raw >> 24U));
    out.push_back(static_cast<std::uint8_t>(raw >> 16U));
    out.push_back(static_cast<std::uint8_t>(raw >> 8U));
    out.push_back(static_cast<std::uint8_t>(raw));
}

void appendOptionalText(
    std::vector<std::uint8_t>& out,
    const std::optional<std::string>& value
) {
    if (value.has_value()) {
        appendText(out, *value);
    } else {
        appendNull(out);
    }
}

void appendOptionalFloat(
    std::vector<std::uint8_t>& out,
    const std::optional<float>& value
) {
    if (value.has_value()) {
        appendFloat32(out, *value);
    } else {
        appendNull(out);
    }
}

std::uint32_t crc32(
    const std::vector<std::uint8_t>& bytes
) {
    std::uint32_t crc = 0xFFFFFFFFU;

    for (const auto byte : bytes) {
        crc ^= byte;

        for (int bit = 0; bit < 8; ++bit) {
            const auto mask =
                static_cast<std::uint32_t>(
                    -(static_cast<std::int32_t>(crc & 1U))
                );
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }

    return ~crc;
}

class CborReader {
public:
    explicit CborReader(
        const std::vector<std::uint8_t>& bytes
    )
        : bytes_(bytes) {}

    bool array(std::uint64_t& count) {
        return typedValue(4, count);
    }

    bool unsignedValue(std::uint64_t& value) {
        return typedValue(0, value);
    }

    bool text(std::string& value) {
        std::uint64_t length = 0;
        if (!typedValue(3, length) ||
            length > remaining()) {
            return false;
        }

        value.assign(
            reinterpret_cast<const char*>(
                bytes_.data() + offset_
            ),
            static_cast<std::size_t>(length)
        );
        offset_ += static_cast<std::size_t>(length);
        return true;
    }

    bool byteString(std::vector<std::uint8_t>& value) {
        std::uint64_t length = 0;
        if (!typedValue(2, length) ||
            length > remaining()) {
            return false;
        }

        const auto begin = bytes_.begin() +
            static_cast<std::ptrdiff_t>(offset_);
        const auto end = begin +
            static_cast<std::ptrdiff_t>(length);
        value.assign(begin, end);
        offset_ += static_cast<std::size_t>(length);
        return true;
    }

    bool nullValue() {
        if (remaining() == 0 ||
            bytes_[offset_] != 0xF6) {
            return false;
        }

        ++offset_;
        return true;
    }

    bool boolean(bool& value) {
        if (remaining() == 0) {
            return false;
        }

        if (bytes_[offset_] == 0xF4) {
            value = false;
            ++offset_;
            return true;
        }

        if (bytes_[offset_] == 0xF5) {
            value = true;
            ++offset_;
            return true;
        }

        return false;
    }

    bool float32(float& value) {
        if (remaining() < 5U ||
            bytes_[offset_] != 0xFA) {
            return false;
        }

        ++offset_;
        std::uint32_t raw = 0;
        for (int i = 0; i < 4; ++i) {
            raw = (raw << 8U) | bytes_[offset_++];
        }

        std::memcpy(&value, &raw, sizeof(value));
        return std::isfinite(value);
    }

    bool optionalText(
        std::optional<std::string>& value
    ) {
        if (peekNull()) {
            ++offset_;
            value.reset();
            return true;
        }

        std::string decoded;
        if (!text(decoded)) {
            return false;
        }

        value = std::move(decoded);
        return true;
    }

    bool optionalFloat(
        std::optional<float>& value
    ) {
        if (peekNull()) {
            ++offset_;
            value.reset();
            return true;
        }

        float decoded = 0.0F;
        if (!float32(decoded)) {
            return false;
        }

        value = decoded;
        return true;
    }

    bool finished() const {
        return offset_ == bytes_.size();
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_{0};

    std::size_t remaining() const {
        return bytes_.size() - offset_;
    }

    bool peekNull() const {
        return remaining() > 0 &&
            bytes_[offset_] == 0xF6;
    }

    bool typedValue(
        std::uint8_t expected_major,
        std::uint64_t& value
    ) {
        if (remaining() == 0) {
            return false;
        }

        const auto initial = bytes_[offset_++];
        const auto major =
            static_cast<std::uint8_t>(initial >> 5U);
        const auto additional =
            static_cast<std::uint8_t>(initial & 0x1FU);

        if (major != expected_major) {
            return false;
        }

        if (additional < 24U) {
            value = additional;
            return true;
        }

        std::size_t width = 0;
        switch (additional) {
            case 24: width = 1; break;
            case 25: width = 2; break;
            case 26: width = 4; break;
            case 27: width = 8; break;
            default: return false;
        }

        if (remaining() < width) {
            return false;
        }

        value = 0;
        for (std::size_t i = 0; i < width; ++i) {
            value = (value << 8U) | bytes_[offset_++];
        }
        return true;
    }
};

void encodeBook(
    std::vector<std::uint8_t>& out,
    const BookRecord& record
) {
    appendArray(out, kBookRecordFieldCount);

    appendText(out, record.book_id);
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(record.format)
    );

    appendText(out, record.metadata.title);
    appendText(out, record.metadata.author_display);

    appendArray(out, record.metadata.authors.size());
    for (const auto& author : record.metadata.authors) {
        appendText(out, author);
    }

    appendOptionalText(out, record.metadata.language);
    appendOptionalText(out, record.metadata.description);
    appendOptionalText(out, record.metadata.series_name);
    appendOptionalFloat(out, record.metadata.series_index);
    appendOptionalText(out, record.metadata.publisher);
    appendOptionalText(out, record.metadata.published_date);
    appendOptionalText(out, record.metadata.identifier);
    appendBool(out, record.metadata.toc_available);

    appendText(out, record.source_path);
    appendText(out, record.source_filename);
    appendUnsigned(out, record.file_size);
    appendText(out, record.fingerprint);

    appendUnsigned(
        out,
        static_cast<std::uint8_t>(record.reading_state)
    );
    appendFloat32(out, record.progress);
    appendUnsigned(out, record.added_order);
    appendUnsigned(out, record.last_opened_order);
    appendOptionalText(out, record.cover_cache_ref);
}

bool decodeBook(
    CborReader& reader,
    BookRecord& record
) {
    std::uint64_t count = 0;
    if (!reader.array(count) ||
        count != kBookRecordFieldCount) {
        return false;
    }

    std::uint64_t format = 0;
    std::uint64_t reading_state = 0;
    std::uint64_t authors_count = 0;

    if (!reader.text(record.book_id) ||
        record.book_id.empty() ||
        !reader.unsignedValue(format) ||
        format > static_cast<std::uint8_t>(BookFormat::Txt) ||
        !reader.text(record.metadata.title) ||
        !reader.text(record.metadata.author_display) ||
        !reader.array(authors_count)) {
        return false;
    }

    record.metadata.authors.clear();
    record.metadata.authors.reserve(
        static_cast<std::size_t>(authors_count)
    );

    for (std::uint64_t i = 0;
         i < authors_count;
         ++i) {
        std::string author;
        if (!reader.text(author)) {
            return false;
        }
        record.metadata.authors.push_back(
            std::move(author)
        );
    }

    if (!reader.optionalText(record.metadata.language) ||
        !reader.optionalText(record.metadata.description) ||
        !reader.optionalText(record.metadata.series_name) ||
        !reader.optionalFloat(record.metadata.series_index) ||
        !reader.optionalText(record.metadata.publisher) ||
        !reader.optionalText(record.metadata.published_date) ||
        !reader.optionalText(record.metadata.identifier) ||
        !reader.boolean(record.metadata.toc_available) ||
        !reader.text(record.source_path) ||
        !reader.text(record.source_filename) ||
        !reader.unsignedValue(record.file_size) ||
        !reader.text(record.fingerprint) ||
        !reader.unsignedValue(reading_state) ||
        reading_state >
            static_cast<std::uint8_t>(
                ReadingState::Finished
            ) ||
        !reader.float32(record.progress) ||
        record.progress < 0.0F ||
        record.progress > 1.0F ||
        !reader.unsignedValue(record.added_order) ||
        !reader.unsignedValue(record.last_opened_order) ||
        !reader.optionalText(record.cover_cache_ref)) {
        return false;
    }

    record.format = static_cast<BookFormat>(format);
    record.reading_state =
        static_cast<ReadingState>(reading_state);

    return true;
}

std::vector<std::uint8_t> encodePayload(
    const std::vector<BookRecord>& records
) {
    std::vector<std::uint8_t> out;
    appendArray(out, records.size());

    for (const auto& record : records) {
        encodeBook(out, record);
    }

    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    std::vector<BookRecord>& records
) {
    CborReader reader(payload);

    std::uint64_t count = 0;
    if (!reader.array(count)) {
        return false;
    }

    records.clear();
    records.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t i = 0; i < count; ++i) {
        BookRecord record;
        if (!decodeBook(reader, record)) {
            return false;
        }

        const auto duplicate = std::find_if(
            records.begin(),
            records.end(),
            [&](const BookRecord& existing) {
                return existing.book_id == record.book_id;
            }
        );

        if (duplicate != records.end()) {
            return false;
        }

        records.push_back(std::move(record));
    }

    return reader.finished();
}

LibraryStatus mapFileStatus(StateFileStatus status) {
    switch (status) {
        case StateFileStatus::Ok:
            return LibraryStatus::Ok;
        case StateFileStatus::NotFound:
            return LibraryStatus::NotFound;
        case StateFileStatus::NoSpace:
            return LibraryStatus::NoSpace;
        case StateFileStatus::IoError:
        default:
            return LibraryStatus::PersistenceFailure;
    }
}

std::string simpleUtf8Fold(
    std::string_view value
) {
    std::string out;
    out.reserve(value.size());

    std::size_t offset = 0;

    while (offset < value.size()) {
        const auto first =
            static_cast<unsigned char>(
                value[offset]
            );

        if (first < 0x80U) {
            char ch =
                static_cast<char>(first);

            if (ch >= 'A' && ch <= 'Z') {
                ch = static_cast<char>(
                    ch - 'A' + 'a'
                );
            }

            out.push_back(ch);
            ++offset;
            continue;
        }

        if (offset + 1U < value.size() &&
            (first & 0xE0U) == 0xC0U) {
            const auto second =
                static_cast<unsigned char>(
                    value[offset + 1U]
                );

            if ((second & 0xC0U) == 0x80U) {
                std::uint32_t codepoint =
                    ((first & 0x1FU) << 6U) |
                    (second & 0x3FU);

                if (codepoint >= 0x0410U &&
                    codepoint <= 0x042FU) {
                    codepoint += 0x20U;
                } else if (codepoint == 0x0401U) {
                    codepoint = 0x0451U;
                }

                out.push_back(
                    static_cast<char>(
                        0xC0U |
                        ((codepoint >> 6U) &
                         0x1FU)
                    )
                );
                out.push_back(
                    static_cast<char>(
                        0x80U |
                        (codepoint & 0x3FU)
                    )
                );
                offset += 2U;
                continue;
            }
        }

        out.push_back(
            static_cast<char>(first)
        );
        ++offset;
    }

    return out;
}

bool containsSearch(
    const BookRecord& record,
    const std::string& needle
) {
    if (needle.empty()) {
        return true;
    }

    const auto title =
        simpleUtf8Fold(record.metadata.title);
    if (title.find(needle) != std::string::npos) {
        return true;
    }

    const auto display =
        simpleUtf8Fold(record.metadata.author_display);
    if (display.find(needle) != std::string::npos) {
        return true;
    }

    for (const auto& author : record.metadata.authors) {
        if (simpleUtf8Fold(author).find(needle) !=
            std::string::npos) {
            return true;
        }
    }

    return false;
}

bool filterMatches(
    const BookRecord& record,
    LibraryFilter filter
) {
    switch (filter) {
        case LibraryFilter::All:
            return true;
        case LibraryFilter::New:
            return record.reading_state ==
                ReadingState::New;
        case LibraryFilter::Reading:
            return record.reading_state ==
                ReadingState::Reading;
        case LibraryFilter::Finished:
            return record.reading_state ==
                ReadingState::Finished;
        default:
            return false;
    }
}

} // namespace

CborLibraryService::CborLibraryService(
    StateFileStore& files
)
    : files_(files) {}

const std::vector<BookRecord>&
CborLibraryService::records() const {
    return records_;
}

std::vector<std::uint8_t>
CborLibraryService::encode(
    std::uint32_t generation,
    const std::vector<BookRecord>& records
) {
    const auto payload = encodePayload(records);

    std::vector<std::uint8_t> out;
    appendArray(out, 5);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeLibraryIndex);
    appendUnsigned(out, generation);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));
    return out;
}

bool CborLibraryService::decode(
    const std::vector<std::uint8_t>& bytes,
    DecodedIndex& index
) {
    CborReader reader(bytes);

    std::uint64_t count = 0;
    std::uint64_t schema = 0;
    std::uint64_t type = 0;
    std::uint64_t generation = 0;
    std::vector<std::uint8_t> payload;
    std::uint64_t stored_crc = 0;

    if (!reader.array(count) || count != 5U ||
        !reader.unsignedValue(schema) ||
        !reader.unsignedValue(type) ||
        !reader.unsignedValue(generation) ||
        !reader.byteString(payload) ||
        !reader.unsignedValue(stored_crc) ||
        !reader.finished()) {
        return false;
    }

    if (schema != kSchemaVersion ||
        type != kRecordTypeLibraryIndex ||
        generation >
            std::numeric_limits<std::uint32_t>::max() ||
        stored_crc != crc32(payload)) {
        return false;
    }

    std::vector<BookRecord> records;
    if (!decodePayload(payload, records)) {
        return false;
    }

    index.generation =
        static_cast<std::uint32_t>(generation);
    index.records = std::move(records);
    return true;
}

LibraryStatus CborLibraryService::readSlot(
    const std::string& path,
    DecodedIndex& index
) const {
    std::vector<std::uint8_t> bytes;
    const auto status = files_.read(path, bytes);

    if (status != StateFileStatus::Ok) {
        return mapFileStatus(status);
    }

    if (!decode(bytes, index)) {
        return LibraryStatus::InvalidRecord;
    }

    return LibraryStatus::Ok;
}

LibraryStatus CborLibraryService::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    DecodedIndex a;
    DecodedIndex b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    const bool a_ok = a_status == LibraryStatus::Ok;
    const bool b_ok = b_status == LibraryStatus::Ok;

    if (a_ok || b_ok) {
        const auto& selected =
            a_ok && (!b_ok || a.generation >= b.generation)
                ? a
                : b;

        generation_ = selected.generation;
        records_ = selected.records;
        return LibraryStatus::Ok;
    }

    if (a_status == LibraryStatus::NotFound &&
        b_status == LibraryStatus::NotFound) {
        records_.clear();
        generation_ = 0;
        return LibraryStatus::Ok;
    }

    if (a_status == LibraryStatus::PersistenceFailure ||
        b_status == LibraryStatus::PersistenceFailure) {
        return LibraryStatus::PersistenceFailure;
    }

    return LibraryStatus::InvalidRecord;
}

LibraryStatus CborLibraryService::commit() {
    DecodedIndex a;
    DecodedIndex b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    if (a_status == LibraryStatus::PersistenceFailure ||
        b_status == LibraryStatus::PersistenceFailure) {
        return LibraryStatus::PersistenceFailure;
    }

    const bool a_ok = a_status == LibraryStatus::Ok;
    const bool b_ok = b_status == LibraryStatus::Ok;

    const auto max_generation = std::max(
        a_ok ? a.generation : 0U,
        b_ok ? b.generation : 0U
    );

    if (max_generation ==
        std::numeric_limits<std::uint32_t>::max()) {
        return LibraryStatus::InvalidRecord;
    }

    const char* target = kSlotA;

    if (!a_ok) {
        target = kSlotA;
    } else if (!b_ok) {
        target = kSlotB;
    } else {
        target =
            a.generation <= b.generation
                ? kSlotA
                : kSlotB;
    }

    const auto next_generation =
        max_generation + 1U;
    const auto bytes =
        encode(next_generation, records_);

    const auto write_status =
        mapFileStatus(files_.write(target, bytes));

    if (write_status != LibraryStatus::Ok) {
        return write_status;
    }

    generation_ = next_generation;
    return LibraryStatus::Ok;
}

LibraryStatus CborLibraryService::upsert(
    const BookRecord& record
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (record.book_id.empty() ||
        !std::isfinite(record.progress) ||
        record.progress < 0.0F ||
        record.progress > 1.0F) {
        return LibraryStatus::InvalidRecord;
    }

    const auto previous = records_;

    const auto it = std::find_if(
        records_.begin(),
        records_.end(),
        [&](const BookRecord& existing) {
            return existing.book_id == record.book_id;
        }
    );

    if (it == records_.end()) {
        records_.push_back(record);
    } else {
        *it = record;
    }

    const auto status = commit();
    if (status != LibraryStatus::Ok) {
        records_ = previous;
    }

    return status;
}

LibraryStatus CborLibraryService::remove(
    const BookId& book_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(
        records_.begin(),
        records_.end(),
        [&](const BookRecord& record) {
            return record.book_id == book_id;
        }
    );

    if (it == records_.end()) {
        return LibraryStatus::NotFound;
    }

    const auto previous = records_;
    records_.erase(it);

    const auto status = commit();
    if (status != LibraryStatus::Ok) {
        records_ = previous;
    }

    return status;
}

std::optional<BookRecord> CborLibraryService::get(
    const BookId& book_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(
        records_.begin(),
        records_.end(),
        [&](const BookRecord& record) {
            return record.book_id == book_id;
        }
    );

    if (it == records_.end()) {
        return std::nullopt;
    }

    return *it;
}

std::optional<BookId>
CborLibraryService::findByFingerprint(
    const std::string& fingerprint
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(
        records_.begin(),
        records_.end(),
        [&](const BookRecord& record) {
            return record.fingerprint == fingerprint;
        }
    );

    if (it == records_.end()) {
        return std::nullopt;
    }

    return it->book_id;
}

LibraryStatus CborLibraryService::query(
    const LibraryQuery& query_request,
    LibraryPage& page
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (query_request.limit == 0) {
        return LibraryStatus::InvalidQuery;
    }

    std::vector<BookRecord> matches;
    matches.reserve(records_.size());

    const auto needle =
        simpleUtf8Fold(query_request.search_text);

    for (const auto& record : records_) {
        const bool include =
            query_request.mode == LibraryQueryMode::Search
                ? containsSearch(record, needle)
                : filterMatches(
                    record,
                    query_request.filter
                );

        if (include) {
            matches.push_back(record);
        }
    }

    const auto ascending =
        query_request.direction ==
        SortDirection::Ascending;

    std::sort(
        matches.begin(),
        matches.end(),
        [&](const BookRecord& lhs,
            const BookRecord& rhs) {
            int comparison = 0;

            switch (query_request.sort) {
                case LibrarySort::Title:
                    comparison =
                        lhs.metadata.title.compare(
                            rhs.metadata.title
                        );
                    break;

                case LibrarySort::Author:
                    comparison =
                        lhs.metadata.author_display.compare(
                            rhs.metadata.author_display
                        );
                    break;

                case LibrarySort::RecentlyOpened:
                    if (lhs.last_opened_order <
                        rhs.last_opened_order) {
                        comparison = -1;
                    } else if (lhs.last_opened_order >
                               rhs.last_opened_order) {
                        comparison = 1;
                    }
                    break;

                case LibrarySort::RecentlyAdded:
                    if (lhs.added_order <
                        rhs.added_order) {
                        comparison = -1;
                    } else if (lhs.added_order >
                               rhs.added_order) {
                        comparison = 1;
                    }
                    break;
            }

            if (comparison == 0) {
                comparison =
                    lhs.book_id.compare(rhs.book_id);
            }

            return ascending
                ? comparison < 0
                : comparison > 0;
        }
    );

    page.items.clear();
    page.total_matches =
        static_cast<std::uint32_t>(matches.size());
    page.offset = query_request.offset;

    if (query_request.offset >= matches.size()) {
        return LibraryStatus::Ok;
    }

    const auto begin =
        matches.begin() +
        static_cast<std::ptrdiff_t>(
            query_request.offset
        );

    const auto available =
        matches.size() - query_request.offset;
    const auto count = std::min<std::size_t>(
        query_request.limit,
        available
    );

    page.items.assign(
        begin,
        begin + static_cast<std::ptrdiff_t>(count)
    );

    return LibraryStatus::Ok;
}

LibraryStatus CborLibraryService::updateSummary(
    const BookId& book_id,
    ReadingState reading_state,
    float progress,
    std::uint64_t last_opened_order
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!std::isfinite(progress) ||
        progress < 0.0F ||
        progress > 1.0F) {
        return LibraryStatus::InvalidRecord;
    }

    const auto it = std::find_if(
        records_.begin(),
        records_.end(),
        [&](const BookRecord& record) {
            return record.book_id == book_id;
        }
    );

    if (it == records_.end()) {
        return LibraryStatus::NotFound;
    }

    const auto previous = *it;

    it->reading_state = reading_state;
    it->progress = progress;
    it->last_opened_order = last_opened_order;

    const auto status = commit();
    if (status != LibraryStatus::Ok) {
        *it = previous;
    }

    return status;
}

} // namespace enku
