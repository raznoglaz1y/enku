#pragma once

#include <memory>
#include <optional>

#include "../core/library.hpp"
#include "../services/services.hpp"
#include "document_reader_engine.hpp"
#include "reader_session.hpp"
#include "txt_parser.hpp"
#include "epub_parser.hpp"
#include "fb2_parser.hpp"

namespace enku {

enum class BookLoadStatus : std::uint8_t {
    Ok,
    NotFound,
    SourceUnavailable,
    SourceReadFailed,
    SourceChanged,
    UnsupportedFormat,
    ParseFailed,
    SessionOpenFailed,
};

struct BookLoadRequest {
    BookId book_id;
    std::optional<SemanticPosition> saved_position;
    TypographySettings typography;
    Viewport viewport;
};

struct BookLoadResult {
    BookLoadStatus status{BookLoadStatus::SessionOpenFailed};
    ReaderSessionStatus session_status{ReaderSessionStatus::Closed};

    bool ok() const {
        return status == BookLoadStatus::Ok;
    }
};

class ReaderBookLoader {
public:
    ReaderBookLoader(
        LibraryService& library,
        BookSourceService& source,
        const TextMeasurer& measurer
    );

    BookLoadResult open(const BookLoadRequest& request);
    void close();

    ReaderSession* session();
    const ReaderSession* session() const;

    const BookDocument* document() const;

private:
    LibraryService& library_;
    BookSourceService& source_;
    const TextMeasurer& measurer_;

    TxtParser txt_parser_;
    EpubParser epub_parser_;
    Fb2Parser fb2_parser_;

    std::optional<BookDocument> document_;
    std::unique_ptr<DocumentReaderEngine> engine_;
    std::unique_ptr<ReaderSession> session_;

    void clearOwnedReader();
};

} // namespace enku
