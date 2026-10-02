#include "enku/storage/cbor_reader_checkpoint.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;
constexpr std::uint64_t kRecordTypeBookState = 2;

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

void appendMap(
    std::vector<std::uint8_t>& out,
    std::uint64_t count
) {
    appendTypeValue(out, 5, count);
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

    bool map(std::uint64_t& count) {
        return typedValue(5, count);
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

std::vector<std::uint8_t> encodePayload(
    const ReaderCheckpoint& checkpoint
) {
    std::vector<std::uint8_t> out;

    appendMap(out, 5);

    appendUnsigned(out, 1);
    appendText(out, checkpoint.position.book_id);

    appendUnsigned(out, 2);
    appendText(out, checkpoint.position.section_id);

    appendUnsigned(out, 3);
    appendUnsigned(out, checkpoint.position.text_offset);

    appendUnsigned(out, 4);
    appendFloat32(out, checkpoint.progress);

    appendUnsigned(out, 5);
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(
            checkpoint.reading_state
        )
    );

    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    ReaderCheckpoint& checkpoint
) {
    CborReader reader(payload);

    std::uint64_t count = 0;
    if (!reader.map(count) || count != 5U) {
        return false;
    }

    bool have_book = false;
    bool have_section = false;
    bool have_offset = false;
    bool have_progress = false;
    bool have_state = false;

    for (std::uint64_t i = 0; i < count; ++i) {
        std::uint64_t key = 0;
        if (!reader.unsignedValue(key)) {
            return false;
        }

        switch (key) {
            case 1:
                if (!reader.text(checkpoint.position.book_id)) {
                    return false;
                }
                have_book = true;
                break;

            case 2:
                if (!reader.text(checkpoint.position.section_id)) {
                    return false;
                }
                have_section = true;
                break;

            case 3:
                if (!reader.unsignedValue(
                        checkpoint.position.text_offset
                    )) {
                    return false;
                }
                have_offset = true;
                break;

            case 4:
                if (!reader.float32(checkpoint.progress) ||
                    checkpoint.progress < 0.0F ||
                    checkpoint.progress > 1.0F) {
                    return false;
                }
                have_progress = true;
                break;

            case 5: {
                std::uint64_t state = 0;
                if (!reader.unsignedValue(state) ||
                    state >
                        static_cast<std::uint8_t>(
                            ReadingState::Finished
                        )) {
                    return false;
                }
                checkpoint.reading_state =
                    static_cast<ReadingState>(state);
                have_state = true;
                break;
            }

            default:
                return false;
        }
    }

    return reader.finished() &&
        have_book &&
        have_section &&
        have_offset &&
        have_progress &&
        have_state;
}

PersistStatus mapFileStatus(StateFileStatus status) {
    switch (status) {
        case StateFileStatus::Ok:
            return PersistStatus::Ok;
        case StateFileStatus::NotFound:
            return PersistStatus::NotFound;
        case StateFileStatus::NoSpace:
            return PersistStatus::NoSpace;
        case StateFileStatus::IoError:
        default:
            return PersistStatus::IoError;
    }
}

std::string hexBookId(const BookId& book_id) {
    static constexpr char kHex[] = "0123456789abcdef";

    std::string encoded;
    encoded.reserve(book_id.size() * 2U);

    for (const unsigned char c : book_id) {
        encoded.push_back(kHex[c >> 4U]);
        encoded.push_back(kHex[c & 0x0FU]);
    }

    return encoded;
}

} // namespace

CborReaderCheckpointService::CborReaderCheckpointService(
    StateFileStore& files
)
    : files_(files) {}

std::string CborReaderCheckpointService::slotPath(
    const BookId& book_id,
    char slot
) {
    return "/system/state/" +
        hexBookId(book_id) +
        "." +
        std::string(1, slot) +
        ".cbor";
}

std::vector<std::uint8_t>
CborReaderCheckpointService::encode(
    std::uint32_t generation,
    const ReaderCheckpoint& checkpoint
) {
    const auto payload = encodePayload(checkpoint);

    std::vector<std::uint8_t> out;

    // Envelope:
    // [schema_version, record_type, generation, payload_bstr, crc32]
    appendArray(out, 5);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeBookState);
    appendUnsigned(out, generation);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));

    return out;
}

bool CborReaderCheckpointService::decode(
    const std::vector<std::uint8_t>& bytes,
    DecodedRecord& record
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
        type != kRecordTypeBookState ||
        generation >
            std::numeric_limits<std::uint32_t>::max() ||
        stored_crc != crc32(payload)) {
        return false;
    }

    ReaderCheckpoint decoded;
    if (!decodePayload(payload, decoded)) {
        return false;
    }

    record.generation =
        static_cast<std::uint32_t>(generation);
    record.checkpoint = std::move(decoded);
    return true;
}

PersistStatus CborReaderCheckpointService::readSlot(
    const std::string& path,
    DecodedRecord& record
) {
    std::vector<std::uint8_t> bytes;
    const auto status = files_.read(path, bytes);

    if (status != StateFileStatus::Ok) {
        return mapFileStatus(status);
    }

    if (!decode(bytes, record)) {
        return PersistStatus::InvalidRecord;
    }

    return PersistStatus::Ok;
}

PersistStatus CborReaderCheckpointService::load(
    const BookId& book_id,
    ReaderCheckpoint& checkpoint
) {
    DecodedRecord a;
    DecodedRecord b;

    const auto a_status =
        readSlot(slotPath(book_id, 'a'), a);
    const auto b_status =
        readSlot(slotPath(book_id, 'b'), b);

    const bool a_ok = a_status == PersistStatus::Ok &&
        a.checkpoint.position.book_id == book_id;
    const bool b_ok = b_status == PersistStatus::Ok &&
        b.checkpoint.position.book_id == book_id;

    if (a_ok || b_ok) {
        if (a_ok && (!b_ok || a.generation >= b.generation)) {
            checkpoint = a.checkpoint;
        } else {
            checkpoint = b.checkpoint;
        }
        return PersistStatus::Ok;
    }

    if (a_status == PersistStatus::IoError ||
        b_status == PersistStatus::IoError) {
        return PersistStatus::IoError;
    }

    if (a_status == PersistStatus::NotFound &&
        b_status == PersistStatus::NotFound) {
        return PersistStatus::NotFound;
    }

    return PersistStatus::InvalidRecord;
}

PersistStatus CborReaderCheckpointService::checkpoint(
    const BookId& book_id,
    const SemanticPosition& position,
    float progress,
    ReadingState reading_state
) {
    if (book_id.empty() ||
        position.book_id != book_id ||
        !std::isfinite(progress) ||
        progress < 0.0F ||
        progress > 1.0F) {
        return PersistStatus::InvalidRecord;
    }

    DecodedRecord a;
    DecodedRecord b;

    const auto a_status =
        readSlot(slotPath(book_id, 'a'), a);
    const auto b_status =
        readSlot(slotPath(book_id, 'b'), b);

    if (a_status == PersistStatus::IoError ||
        b_status == PersistStatus::IoError) {
        return PersistStatus::IoError;
    }

    const bool a_ok = a_status == PersistStatus::Ok &&
        a.checkpoint.position.book_id == book_id;
    const bool b_ok = b_status == PersistStatus::Ok &&
        b.checkpoint.position.book_id == book_id;

    const std::uint32_t max_generation =
        std::max(
            a_ok ? a.generation : 0U,
            b_ok ? b.generation : 0U
        );

    if (max_generation ==
        std::numeric_limits<std::uint32_t>::max()) {
        return PersistStatus::InvalidRecord;
    }

    char target = 'a';

    if (!a_ok) {
        target = 'a';
    } else if (!b_ok) {
        target = 'b';
    } else {
        target =
            a.generation <= b.generation ? 'a' : 'b';
    }

    const ReaderCheckpoint record{
        position,
        progress,
        reading_state,
    };

    const auto bytes =
        encode(max_generation + 1U, record);

    return mapFileStatus(
        files_.write(slotPath(book_id, target), bytes)
    );
}

} // namespace enku
