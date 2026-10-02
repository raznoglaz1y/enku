#include "enku/reader/book_loader.hpp"

#include <string>
#include <utility>

namespace enku {

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

    {
        std::string bytes;
        const auto source_status =
            source_.readSource(*record, bytes);

        if (source_status == BookSourceStatus::Unavailable) {
            return {
                BookLoadStatus::SourceUnavailable,
                ReaderSessionStatus::Closed,
            };
        }

        if (source_status != BookSourceStatus::Ok) {
            return {
                BookLoadStatus::SourceReadFailed,
                ReaderSessionStatus::Closed,
            };
        }

        ParseResult parsed;

        switch (record->format) {
            case BookFormat::Txt: {
                ParserSourceInfo source_info{
                    record->book_id,
                    record->source_path,
                    record->source_filename,
                };

                parsed = txt_parser_.parse(bytes, source_info);
                break;
            }

            case BookFormat::Epub: {
                ParserSourceInfo source_info{
                    record->book_id,
                    record->source_path,
                    record->source_filename,
                };

                parsed =
                    epub_parser_.parse(
                        bytes,
                        source_info
                    );
                break;
            }

            case BookFormat::Fb2: {
                ParserSourceInfo source_info{
                    record->book_id,
                    record->source_path,
                    record->source_filename,
                };

                parsed =
                    fb2_parser_.parse(
                        bytes,
                        source_info
                    );
                break;
            }

            default:
                return {
                    BookLoadStatus::UnsupportedFormat,
                    ReaderSessionStatus::Closed,
                };
        }

        if (!parsed.ok()) {
            return {
                BookLoadStatus::ParseFailed,
                ReaderSessionStatus::Closed,
            };
        }

        document_ = std::move(parsed.document);

    }

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
