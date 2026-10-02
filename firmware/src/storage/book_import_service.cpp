#include "enku/storage/book_import_service.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace enku {
namespace {

std::string lowerExtension(const std::string& filename) {
    const auto slash = filename.find_last_of("/\\");
    const auto base = slash == std::string::npos
        ? filename
        : filename.substr(slash + 1);

    const auto dot = base.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }

    std::string ext = base.substr(dot);
    std::transform(
        ext.begin(),
        ext.end(),
        ext.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );
    return ext;
}

} // namespace

BookImportService::BookImportService(
    LibraryService& library
)
    : library_(library) {}

std::string BookImportService::fingerprint(
    const std::string& bytes
) {
    // Reader v1 import MVP: deterministic content fingerprint.
    // This deliberately remains isolated behind the import service so it can
    // later be replaced by SHA-256 without changing Library identity APIs.
    std::uint64_t hash = 14695981039346656037ULL;

    for (const unsigned char ch : bytes) {
        hash ^= ch;
        hash *= 1099511628211ULL;
    }

    std::ostringstream out;
    out << "fnv1a64:"
        << std::hex
        << std::setfill('0')
        << std::setw(16)
        << hash
        << ":"
        << std::dec
        << bytes.size();

    return out.str();
}

BookFormat BookImportService::detectFormat(
    const std::string& filename,
    bool& supported
) {
    const auto ext = lowerExtension(filename);

    if (ext == ".txt") {
        supported = true;
        return BookFormat::Txt;
    }

    supported = false;

    if (ext == ".epub") {
        return BookFormat::Epub;
    }

    if (ext == ".fb2") {
        return BookFormat::Fb2;
    }

    return BookFormat::Txt;
}

BookImportResult BookImportService::import(
    const BookImportSource& source,
    std::uint64_t added_order
) {
    BookImportResult result;

    if (source.bytes.empty()) {
        result.status = BookImportStatus::EmptySource;
        return result;
    }

    bool supported = false;
    const auto format =
        detectFormat(source.source_filename, supported);

    if (!supported) {
        result.status = BookImportStatus::UnsupportedFormat;
        return result;
    }

    result.fingerprint = fingerprint(source.bytes);

    if (library_.findByFingerprint(
            result.fingerprint
        ).has_value()) {
        result.status = BookImportStatus::Duplicate;
        return result;
    }

    const auto id_suffix =
        result.fingerprint.substr(
            result.fingerprint.find(':') + 1,
            16
        );
    result.book_id = "book-" + id_suffix;

    ParserSourceInfo parser_source{
        result.book_id,
        source.source_path,
        source.source_filename,
    };

    ParseResult parsed;

    switch (format) {
        case BookFormat::Txt:
            parsed = txt_parser_.parse(
                source.bytes,
                parser_source
            );
            break;

        case BookFormat::Epub:
        case BookFormat::Fb2:
        default:
            result.status =
                BookImportStatus::UnsupportedFormat;
            return result;
    }

    if (!parsed.ok()) {
        result.status = BookImportStatus::ParseFailed;
        return result;
    }

    BookRecord record;
    record.book_id = result.book_id;
    record.format = format;
    record.metadata = std::move(parsed.document.metadata);
    record.source_path = source.source_path;
    record.source_filename = source.source_filename;
    record.file_size = source.bytes.size();
    record.fingerprint = result.fingerprint;
    record.reading_state = ReadingState::New;
    record.progress = 0.0F;
    record.added_order = added_order;
    record.last_opened_order = 0;

    const auto library_status =
        library_.upsert(record);

    if (library_status != LibraryStatus::Ok) {
        result.status =
            BookImportStatus::LibraryCommitFailed;
        return result;
    }

    result.status = BookImportStatus::Ok;
    return result;
}

} // namespace enku
