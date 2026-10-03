#include "enku/storage/book_import_service.hpp"
#include "enku/storage/book_fingerprint.hpp"
#include "enku/reader/zip_archive.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace enku {
namespace {

class BookFileTextRangeSource final
    : public TextRangeSource {
public:
    BookFileTextRangeSource(
        BookFileStore& files,
        std::string path,
        std::uint64_t size_bytes
    )
        : files_(files),
          path_(std::move(path)),
          size_bytes_(size_bytes) {}

    std::uint64_t size() const override {
        return size_bytes_;
    }

    bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const override {
        last_status_ =
            files_.readRange(
                path_,
                offset,
                length,
                out
            );

        return last_status_ ==
            BookFileStatus::Ok;
    }

    BookFileStatus lastStatus() const {
        return last_status_;
    }

private:
    BookFileStore& files_;
    std::string path_;
    std::uint64_t size_bytes_{0};
    mutable BookFileStatus last_status_{
        BookFileStatus::Ok
    };
};

class BookFileFb2RangeSource final
    : public Fb2RangeSource {
public:
    BookFileFb2RangeSource(
        BookFileStore& files,
        std::string path,
        std::uint64_t size_bytes
    )
        : files_(files),
          path_(std::move(path)),
          size_bytes_(size_bytes) {}

    std::uint64_t size() const override {
        return size_bytes_;
    }

    bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const override {
        last_status_ =
            files_.readRange(
                path_,
                offset,
                length,
                out
            );

        return last_status_ ==
            BookFileStatus::Ok;
    }

    BookFileStatus lastStatus() const {
        return last_status_;
    }

private:
    BookFileStore& files_;
    std::string path_;
    std::uint64_t size_bytes_{0};
    mutable BookFileStatus last_status_{
        BookFileStatus::Ok
    };
};

class BookFileZipRangeSource final
    : public ZipRangeSource {
public:
    BookFileZipRangeSource(
        BookFileStore& files,
        std::string path,
        std::uint64_t size_bytes
    )
        : files_(files),
          path_(std::move(path)),
          size_bytes_(size_bytes) {}

    std::uint64_t size() const override {
        return size_bytes_;
    }

    bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const override {
        last_status_ =
            files_.readRange(
                path_,
                offset,
                length,
                out
            );

        return last_status_ ==
            BookFileStatus::Ok;
    }

    BookFileStatus lastStatus() const {
        return last_status_;
    }

private:
    BookFileStore& files_;
    std::string path_;
    std::uint64_t size_bytes_{0};
    mutable BookFileStatus last_status_{
        BookFileStatus::Ok
    };
};

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

BookFormat BookImportService::detectFormat(
    const std::string& filename,
    bool& supported
) {
    const auto ext = lowerExtension(filename);

    if (ext == ".txt") {
        supported = true;
        return BookFormat::Txt;
    }

    if (ext == ".epub") {
        supported = true;
        return BookFormat::Epub;
    }

    if (ext == ".fb2") {
        supported = true;
        return BookFormat::Fb2;
    }

    supported = false;

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
        fingerprintBookBytes(source.bytes);

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
            parsed =
                epub_parser_.parseMetadata(
                    source.bytes,
                    parser_source
                );
            break;

        case BookFormat::Fb2:
            parsed =
                fb2_parser_.parseMetadata(
                    source.bytes,
                    parser_source
                );
            break;

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

PreparedBookImport BookImportService::prepareStored(
    BookFileStore& files,
    const std::string& source_path,
    const std::string& source_filename,
    std::uint64_t added_order
) {
    PreparedBookImport prepared;

    std::uint64_t file_size = 0;
    const auto size_status =
        files.size(
            source_path,
            file_size
        );

    if (size_status != BookFileStatus::Ok) {
        prepared.status =
            BookImportStatus::SourceReadFailed;
        return prepared;
    }

    if (file_size == 0U) {
        prepared.status =
            BookImportStatus::EmptySource;
        return prepared;
    }

    bool supported = false;
    const auto format =
        detectFormat(
            source_filename,
            supported
        );

    if (!supported) {
        prepared.status =
            BookImportStatus::UnsupportedFormat;
        return prepared;
    }

    const auto fingerprint_result =
        fingerprintStoredBook(
            files,
            source_path
        );

    if (!fingerprint_result.ok() ||
        fingerprint_result.file_size != file_size) {
        prepared.status =
            BookImportStatus::SourceReadFailed;
        return prepared;
    }

    const auto& content_fingerprint =
        fingerprint_result.fingerprint;

    if (library_.findByFingerprint(
            content_fingerprint
        ).has_value()) {
        prepared.status =
            BookImportStatus::Duplicate;
        return prepared;
    }

    const auto id_suffix =
        content_fingerprint.substr(
            content_fingerprint.find(':') + 1,
            16
        );
    const BookId book_id =
        "book-" + id_suffix;

    ParserSourceInfo parser_source{
        book_id,
        source_path,
        source_filename,
    };

    ParseResult parsed;

    if (format == BookFormat::Epub) {
        BookFileZipRangeSource ranged(
            files,
            source_path,
            file_size
        );

        parsed =
            epub_parser_.parseMetadata(
                ranged,
                parser_source
            );

        if (!parsed.ok() &&
            ranged.lastStatus() !=
                BookFileStatus::Ok) {
            prepared.status =
                BookImportStatus::SourceReadFailed;
            return prepared;
        }
    } else if (format == BookFormat::Fb2) {
        BookFileFb2RangeSource ranged(
            files,
            source_path,
            file_size
        );

        parsed =
            fb2_parser_.parseMetadata(
                ranged,
                parser_source
            );

        if (!parsed.ok() &&
            ranged.lastStatus() !=
                BookFileStatus::Ok) {
            prepared.status =
                BookImportStatus::SourceReadFailed;
            return prepared;
        }
    } else {
        BookFileTextRangeSource ranged(
            files,
            source_path,
            file_size
        );

        parsed =
            txt_parser_.parse(
                ranged,
                parser_source
            );

        if (!parsed.ok() &&
            ranged.lastStatus() !=
                BookFileStatus::Ok) {
            prepared.status =
                BookImportStatus::SourceReadFailed;
            return prepared;
        }
    }

    if (!parsed.ok()) {
        prepared.status =
            BookImportStatus::ParseFailed;
        return prepared;
    }

    BookRecord record;
    record.book_id = book_id;
    record.format = format;
    record.metadata =
        std::move(
            parsed.document.metadata
        );
    record.source_path = source_path;
    record.source_filename =
        source_filename;
    record.file_size = file_size;
    record.fingerprint =
        content_fingerprint;
    record.reading_state =
        ReadingState::New;
    record.progress = 0.0F;
    record.added_order = added_order;
    record.last_opened_order = 0;

    prepared.status =
        BookImportStatus::Ok;
    prepared.record =
        std::move(record);
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
