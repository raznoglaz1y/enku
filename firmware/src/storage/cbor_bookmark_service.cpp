#include "enku/storage/cbor_bookmark_service.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;
constexpr std::uint64_t kRecordTypeBookmarks = 6;

void appendTypeValue(
    std::vector<std::uint8_t>& out,
    std::uint8_t major,
    std::uint64_t value
) {
    const auto prefix =
        static_cast<std::uint8_t>(major << 5U);

    if (value < 24U) {
        out.push_back(
            static_cast<std::uint8_t>(
                prefix | value
            )
        );
    } else if (value <= 0xFFU) {
        out.push_back(
            static_cast<std::uint8_t>(
                prefix | 24U
            )
        );
        out.push_back(
            static_cast<std::uint8_t>(value)
        );
    } else if (value <= 0xFFFFU) {
        out.push_back(
            static_cast<std::uint8_t>(
                prefix | 25U
            )
        );
        out.push_back(
            static_cast<std::uint8_t>(
                value >> 8U
            )
        );
        out.push_back(
            static_cast<std::uint8_t>(value)
        );
    } else if (value <= 0xFFFFFFFFULL) {
        out.push_back(
            static_cast<std::uint8_t>(
                prefix | 26U
            )
        );
        for (int shift = 24;
             shift >= 0;
             shift -= 8) {
            out.push_back(
                static_cast<std::uint8_t>(
                    value >> shift
                )
            );
        }
    } else {
        out.push_back(
            static_cast<std::uint8_t>(
                prefix | 27U
            )
        );
        for (int shift = 56;
             shift >= 0;
             shift -= 8) {
            out.push_back(
                static_cast<std::uint8_t>(
                    value >> shift
                )
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

void appendText(
    std::vector<std::uint8_t>& out,
    const std::string& value
) {
    appendTypeValue(out, 3, value.size());
    out.insert(
        out.end(),
        value.begin(),
        value.end()
    );
}

void appendBytes(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& value
) {
    appendTypeValue(out, 2, value.size());
    out.insert(
        out.end(),
        value.begin(),
        value.end()
    );
}

void appendArray(
    std::vector<std::uint8_t>& out,
    std::uint64_t count
) {
    appendTypeValue(out, 4, count);
}

void appendMap(
    std::vector<std::uint8_t>& out,
    std::uint64_t count
) {
    appendTypeValue(out, 5, count);
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
                    -static_cast<std::int32_t>(
                        crc & 1U
                    )
                );
            crc =
                (crc >> 1U) ^
                (0xEDB88320U & mask);
        }
    }

    return ~crc;
}

std::string hexBookId(
    const BookId& book_id
) {
    static constexpr char kHex[] =
        "0123456789abcdef";

    std::string encoded;
    encoded.reserve(book_id.size() * 2U);

    for (const unsigned char value : book_id) {
        encoded.push_back(
            kHex[value >> 4U]
        );
        encoded.push_back(
            kHex[value & 0x0FU]
        );
    }

    return encoded;
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

    bool map(std::uint64_t& count) {
        return typedValue(5, count);
    }

    bool unsignedValue(
        std::uint64_t& value
    ) {
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

        offset_ +=
            static_cast<std::size_t>(length);

        return true;
    }

    bool byteString(
        std::vector<std::uint8_t>& value
    ) {
        std::uint64_t length = 0;

        if (!typedValue(2, length) ||
            length > remaining()) {
            return false;
        }

        const auto begin =
            bytes_.begin() +
            static_cast<std::ptrdiff_t>(
                offset_
            );

        const auto end =
            begin +
            static_cast<std::ptrdiff_t>(
                length
            );

        value.assign(begin, end);

        offset_ +=
            static_cast<std::size_t>(length);

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

    bool typedValue(
        std::uint8_t expected_major,
        std::uint64_t& value
    ) {
        if (remaining() == 0U) {
            return false;
        }

        const auto initial =
            bytes_[offset_++];

        const auto major =
            static_cast<std::uint8_t>(
                initial >> 5U
            );

        const auto additional =
            static_cast<std::uint8_t>(
                initial & 0x1FU
            );

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

        for (std::size_t i = 0;
             i < width;
             ++i) {
            value =
                (value << 8U) |
                bytes_[offset_++];
        }

        return true;
    }
};

std::vector<std::uint8_t> encodePayload(
    const std::vector<BookmarkRecord>& bookmarks
) {
    std::vector<std::uint8_t> out;

    appendArray(out, bookmarks.size());

    for (const auto& bookmark : bookmarks) {
        appendMap(out, 4);

        appendUnsigned(out, 1);
        appendText(
            out,
            bookmark.position.book_id
        );

        appendUnsigned(out, 2);
        appendText(
            out,
            bookmark.position.section_id
        );

        appendUnsigned(out, 3);
        appendUnsigned(
            out,
            bookmark.position.text_offset
        );

        appendUnsigned(out, 4);
        appendText(
            out,
            bookmark.label
        );
    }

    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    std::vector<BookmarkRecord>& bookmarks
) {
    CborReader reader(payload);

    std::uint64_t count = 0;

    if (!reader.array(count) ||
        count > 4096U) {
        return false;
    }

    std::vector<BookmarkRecord> decoded;
    decoded.reserve(
        static_cast<std::size_t>(count)
    );

    for (std::uint64_t i = 0;
         i < count;
         ++i) {
        std::uint64_t fields = 0;

        if (!reader.map(fields) ||
            fields != 4U) {
            return false;
        }

        BookmarkRecord bookmark;
        bool have_book = false;
        bool have_section = false;
        bool have_offset = false;
        bool have_label = false;

        for (std::uint64_t field = 0;
             field < fields;
             ++field) {
            std::uint64_t key = 0;

            if (!reader.unsignedValue(key)) {
                return false;
            }

            switch (key) {
                case 1:
                    if (!reader.text(
                            bookmark.position.book_id
                        )) {
                        return false;
                    }
                    have_book = true;
                    break;

                case 2:
                    if (!reader.text(
                            bookmark.position.section_id
                        )) {
                        return false;
                    }
                    have_section = true;
                    break;

                case 3:
                    if (!reader.unsignedValue(
                            bookmark.position.text_offset
                        )) {
                        return false;
                    }
                    have_offset = true;
                    break;

                case 4:
                    if (!reader.text(
                            bookmark.label
                        )) {
                        return false;
                    }
                    have_label = true;
                    break;

                default:
                    return false;
            }
        }

        if (!have_book ||
            !have_section ||
            !have_offset ||
            !have_label ||
            bookmark.position.book_id.empty()) {
            return false;
        }

        decoded.push_back(
            std::move(bookmark)
        );
    }

    if (!reader.finished()) {
        return false;
    }

    bookmarks = std::move(decoded);
    return true;
}

std::vector<std::uint8_t> encodeRecord(
    const std::vector<BookmarkRecord>& bookmarks
) {
    const auto payload =
        encodePayload(bookmarks);

    std::vector<std::uint8_t> out;

    appendArray(out, 4);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeBookmarks);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));

    return out;
}

bool decodeRecord(
    const std::vector<std::uint8_t>& bytes,
    std::vector<BookmarkRecord>& bookmarks
) {
    CborReader reader(bytes);

    std::uint64_t count = 0;
    std::uint64_t schema = 0;
    std::uint64_t type = 0;
    std::vector<std::uint8_t> payload;
    std::uint64_t stored_crc = 0;

    if (!reader.array(count) ||
        count != 4U ||
        !reader.unsignedValue(schema) ||
        !reader.unsignedValue(type) ||
        !reader.byteString(payload) ||
        !reader.unsignedValue(stored_crc) ||
        !reader.finished()) {
        return false;
    }

    if (schema != kSchemaVersion ||
        type != kRecordTypeBookmarks ||
        stored_crc != crc32(payload)) {
        return false;
    }

    return decodePayload(
        payload,
        bookmarks
    );
}

BookmarkStatus mapFileStatus(
    StateFileStatus status
) {
    switch (status) {
        case StateFileStatus::Ok:
            return BookmarkStatus::Ok;
        case StateFileStatus::NotFound:
            return BookmarkStatus::NotFound;
        case StateFileStatus::IoError:
        case StateFileStatus::NoSpace:
        default:
            return BookmarkStatus::IoError;
    }
}

} // namespace

CborBookmarkService::CborBookmarkService(
    StateFileStore& files
)
    : files_(files) {}

std::string CborBookmarkService::pathFor(
    const BookId& book_id
) {
    return "/system/bookmarks/" +
        hexBookId(book_id) +
        ".cbor";
}

bool CborBookmarkService::samePosition(
    const SemanticPosition& a,
    const SemanticPosition& b
) {
    return a.book_id == b.book_id &&
           a.section_id == b.section_id &&
           a.text_offset == b.text_offset;
}

BookmarkStatus CborBookmarkService::load(
    const BookId& book_id,
    std::vector<BookmarkRecord>& bookmarks
) const {
    bookmarks.clear();

    std::vector<std::uint8_t> bytes;

    const auto status =
        files_.read(
            pathFor(book_id),
            bytes
        );

    if (status == StateFileStatus::NotFound) {
        return BookmarkStatus::NotFound;
    }

    if (status != StateFileStatus::Ok) {
        return mapFileStatus(status);
    }

    if (!decodeRecord(bytes, bookmarks)) {
        bookmarks.clear();
        return BookmarkStatus::Corrupt;
    }

    for (const auto& bookmark : bookmarks) {
        if (bookmark.position.book_id !=
            book_id) {
            bookmarks.clear();
            return BookmarkStatus::Corrupt;
        }
    }

    std::sort(
        bookmarks.begin(),
        bookmarks.end(),
        [](const BookmarkRecord& a,
           const BookmarkRecord& b) {
            if (a.position.section_id !=
                b.position.section_id) {
                return a.position.section_id <
                    b.position.section_id;
            }

            return a.position.text_offset <
                b.position.text_offset;
        }
    );

    return BookmarkStatus::Ok;
}

BookmarkStatus CborBookmarkService::replace(
    const BookId& book_id,
    const std::vector<BookmarkRecord>& bookmarks
) {
    for (const auto& bookmark : bookmarks) {
        if (bookmark.position.book_id !=
            book_id) {
            return BookmarkStatus::Invalid;
        }
    }

    if (bookmarks.empty()) {
        return eraseBook(book_id);
    }

    return mapFileStatus(
        files_.write(
            pathFor(book_id),
            encodeRecord(bookmarks)
        )
    );
}

BookmarkStatus CborBookmarkService::add(
    const BookmarkRecord& bookmark
) {
    if (bookmark.position.book_id.empty()) {
        return BookmarkStatus::Invalid;
    }

    std::vector<BookmarkRecord> bookmarks;

    const auto load_status =
        load(
            bookmark.position.book_id,
            bookmarks
        );

    if (load_status != BookmarkStatus::Ok &&
        load_status != BookmarkStatus::NotFound) {
        return load_status;
    }

    const auto duplicate =
        std::find_if(
            bookmarks.begin(),
            bookmarks.end(),
            [&](const BookmarkRecord& existing) {
                return samePosition(
                    existing.position,
                    bookmark.position
                );
            }
        );

    if (duplicate != bookmarks.end()) {
        return BookmarkStatus::AlreadyExists;
    }

    bookmarks.push_back(bookmark);

    return replace(
        bookmark.position.book_id,
        bookmarks
    );
}

BookmarkStatus CborBookmarkService::remove(
    const SemanticPosition& position
) {
    std::vector<BookmarkRecord> bookmarks;

    const auto load_status =
        load(
            position.book_id,
            bookmarks
        );

    if (load_status != BookmarkStatus::Ok) {
        return load_status;
    }

    const auto before =
        bookmarks.size();

    bookmarks.erase(
        std::remove_if(
            bookmarks.begin(),
            bookmarks.end(),
            [&](const BookmarkRecord& bookmark) {
                return samePosition(
                    bookmark.position,
                    position
                );
            }
        ),
        bookmarks.end()
    );

    if (bookmarks.size() == before) {
        return BookmarkStatus::NotFound;
    }

    return replace(
        position.book_id,
        bookmarks
    );
}

BookmarkStatus CborBookmarkService::eraseBook(
    const BookId& book_id
) {
    const auto status =
        files_.remove(
            pathFor(book_id)
        );

    if (status == StateFileStatus::Ok ||
        status == StateFileStatus::NotFound) {
        return BookmarkStatus::Ok;
    }

    return mapFileStatus(status);
}

} // namespace enku
