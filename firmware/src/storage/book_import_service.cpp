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

PreparedBookImport BookImportService::prepare(
    const BookImportSource& source,
    std::uint64_t added_order
) {
    PreparedBookImport prepared;

    if (source.bytes.empty()) {
        prepared.status = BookImportStatus::EmptySource;
        return prepared;
    }

    bool supported = false;
    const auto format =
        detectFormat(source.source_filename, supported);

    if (!supported) {
        prepared.status = BookImportStatus::UnsupportedFormat;
        return prepared;
    }

    const auto content_fingerprint =
        fingerprint(source.bytes);

    if (library_.findByFingerprint(
            content_fingerprint
        ).has_value()) {
        prepared.status = BookImportStatus::Duplicate;
        return prepared;
    }

    const auto id_suffix =
        content_fingerprint.substr(
            content_fingerprint.find(':') + 1,
            16
        );
    const BookId book_id = "book-" + id_suffix;

    ParserSourceInfo parser_source{
        book_id,
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
            prepared.status =
                BookImportStatus::UnsupportedFormat;
            return prepared;
    }

    if (!parsed.ok()) {
        prepared.status = BookImportStatus::ParseFailed;
        return prepared;
    }

    BookRecord record;
    record.book_id = book_id;
    record.format = format;
    record.metadata = std::move(parsed.document.metadata);
    record.source_path = source.source_path;
    record.source_filename = source.source_filename;
    record.file_size = source.bytes.size();
    record.fingerprint = content_fingerprint;
    record.reading_state = ReadingState::New;
    record.progress = 0.0F;
    record.added_order = added_order;
    record.last_opened_order = 0;

    prepared.status = BookImportStatus::Ok;
    prepared.record = std::move(record);
    return prepared;
}

BookImportResult BookImportService::commit(
    const PreparedBookImport& prepared
) {
    BookImportResult result;
    result.status = prepared.status;

    if (!prepared.ok()) {
        return result;
    }

    result.book_id = prepared.record.book_id;
    result.fingerprint = prepared.record.fingerprint;

    if (library_.upsert(prepared.record) !=
        LibraryStatus::Ok) {
        result.status =
            BookImportStatus::LibraryCommitFailed;
        return result;
    }

    result.status = BookImportStatus::Ok;
    return result;
}

BookImportResult BookImportService::import(
    const BookImportSource& source,
    std::uint64_t added_order
) {
    return commit(prepare(source, added_order));
}

} // namespace enku
