#include "enku/reader/book_loader.hpp"
#include "enku/reader/zip_archive.hpp"

#include <string>
#include <utility>

namespace enku {
namespace {

class BookTextRangeSource final
    : public TextRangeSource {
public:
    BookTextRangeSource(
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

class BookFb2RangeSource final
    : public Fb2RangeSource {
public:
    BookFb2RangeSource(
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

    bool source_matches = false;
    const auto validation_status =
        source_.validateSource(
            *record,
            source_matches
        );

    if (validation_status != BookSourceStatus::Ok) {
        return sourceFailure(validation_status);
    }

    if (!source_matches) {
        return {
            BookLoadStatus::SourceChanged,
            ReaderSessionStatus::Closed,
        };
    }

    ParseResult parsed;

    const ParserSourceInfo source_info{
        record->book_id,
        record->source_path,
        record->source_filename,
    };

    if (record->format == BookFormat::Epub ||
        record->format == BookFormat::Txt ||
        record->format == BookFormat::Fb2) {
        std::uint64_t size_bytes = 0;

        const auto size_status =
            source_.sourceSize(
                *record,
                size_bytes
            );

        if (size_status != BookSourceStatus::Ok) {
            return sourceFailure(size_status);
        }

        if (record->format == BookFormat::Epub) {
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
        } else if (
            record->format == BookFormat::Fb2
        ) {
            BookFb2RangeSource ranged_source(
                source_,
                *record,
                size_bytes
            );

            parsed =
                fb2_parser_.parse(
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
            BookTextRangeSource ranged_source(
                source_,
                *record,
                size_bytes
            );

            parsed =
                txt_parser_.parse(
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
        }
    } else {
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

    auto session_status =
        session_->open(layout_request);

    if (session_status != ReaderSessionStatus::Ready &&
        request.saved_position.has_value()) {
        layout_request.anchor =
            SemanticPosition{
                request.book_id,
                "",
                0,
            };

        session_status =
            session_->open(layout_request);
    }

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
