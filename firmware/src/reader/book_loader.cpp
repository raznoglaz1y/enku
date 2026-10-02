#include "enku/reader/book_loader.hpp"
#include "enku/reader/zip_archive.hpp"

#include <string>
#include <utility>

namespace enku {
namespace {

class BookZipRangeSource final
    : public ZipRangeSource {
public:
    BookZipRangeSource(
        BookSourceService& source,
        const BookRecord& record,
        std::uint64_t size_bytes
    )
        : source_(source),
          record_(record),
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
            source_.readSourceRange(
                record_,
                offset,
                length,
                out
            );

        return last_status_ ==
            BookSourceStatus::Ok;
    }

    BookSourceStatus lastStatus() const {
        return last_status_;
    }

private:
    BookSourceService& source_;
    const BookRecord& record_;
    std::uint64_t size_bytes_{0};
    mutable BookSourceStatus last_status_{
        BookSourceStatus::Ok
    };
};

BookLoadResult sourceFailure(
    BookSourceStatus status
) {
    return {
        status ==
                BookSourceStatus::Unavailable
            ? BookLoadStatus::SourceUnavailable
            : BookLoadStatus::SourceReadFailed,
        ReaderSessionStatus::Closed,
    };
}

} // namespace

ReaderBookLoader::ReaderBookLoader(
    LibraryService& library,
    BookSourceService& source,
    const TextMeasurer& measurer
)
    : library_(library),
      source_(source),
      measurer_(measurer) {}

void ReaderBookLoader::clearOwnedReader() {
    session_.reset();
    engine_.reset();
    document_.reset();
}

BookLoadResult ReaderBookLoader::open(
    const BookLoadRequest& request
) {
    clearOwnedReader();

    const auto record = library_.get(request.book_id);
    if (!record.has_value()) {
        return {
            BookLoadStatus::NotFound,
            ReaderSessionStatus::Closed,
        };
    }

    ParseResult parsed;

    const ParserSourceInfo source_info{
        record->book_id,
        record->source_path,
        record->source_filename,
    };

    if (record->format == BookFormat::Epub) {
        std::uint64_t size_bytes = 0;

        const auto size_status =
            source_.sourceSize(
                *record,
                size_bytes
            );

        if (size_status != BookSourceStatus::Ok) {
            return sourceFailure(size_status);
        }

        BookZipRangeSource ranged_source(
            source_,
            *record,
            size_bytes
        );

        parsed =
            epub_parser_.parse(
                ranged_source,
                source_info
            );

        if (!parsed.ok() &&
            ranged_source.lastStatus() !=
                BookSourceStatus::Ok) {
            return sourceFailure(
                ranged_source.lastStatus()
            );
        }
    } else {
        std::string bytes;

        const auto source_status =
            source_.readSource(
                *record,
                bytes
            );

        if (source_status !=
            BookSourceStatus::Ok) {
            return sourceFailure(
                source_status
            );
        }

        switch (record->format) {
            case BookFormat::Txt:
                parsed =
                    txt_parser_.parse(
                        bytes,
                        source_info
                    );
                break;

            case BookFormat::Fb2:
                parsed =
                    fb2_parser_.parse(
                        bytes,
                        source_info
                    );
                break;

            case BookFormat::Epub:
                break;

            default:
                return {
                    BookLoadStatus::UnsupportedFormat,
                    ReaderSessionStatus::Closed,
                };
        }
    }

    if (!parsed.ok()) {
        return {
            BookLoadStatus::ParseFailed,
            ReaderSessionStatus::Closed,
        };
    }

    document_ =
        std::move(parsed.document);

    engine_ = std::make_unique<DocumentReaderEngine>(
        *document_,
        measurer_
    );
    session_ = std::make_unique<ReaderSession>(*engine_);

    SemanticPosition anchor{
        request.book_id,
        "",
        0,
    };

    if (request.saved_position.has_value() &&
        request.saved_position->book_id == request.book_id) {
        anchor = *request.saved_position;
    }

    LayoutRequest layout_request{
        request.book_id,
        anchor,
        request.typography,
        request.viewport,
    };

    const auto session_status =
        session_->open(layout_request);

    if (session_status != ReaderSessionStatus::Ready) {
        clearOwnedReader();
        return {
            BookLoadStatus::SessionOpenFailed,
            session_status,
        };
    }

    return {
        BookLoadStatus::Ok,
        session_status,
    };
}

void ReaderBookLoader::close() {
    clearOwnedReader();
}

ReaderSession* ReaderBookLoader::session() {
    return session_.get();
}

const ReaderSession* ReaderBookLoader::session() const {
    return session_.get();
}

const BookDocument* ReaderBookLoader::document() const {
    if (!document_.has_value()) {
        return nullptr;
    }

    return &*document_;
}

} // namespace enku
