#pragma once

#include <cstdint>

#include "../services/services.hpp"
#include "book_file_store.hpp"
#include "cbor_reader_checkpoint.hpp"

namespace enku {

enum class BookDeleteStatus : std::uint8_t {
    Ok,
    NotFound,
    SourceReadFailed,
    ContextLoadFailed,
    ContextSaveFailed,
    LibraryRemoveFailed,
    SourceRemoveFailed,
    CheckpointRemoveFailed,
    RollbackFailed,
};

class BookDeleteService {
public:
    BookDeleteService(
        LibraryService& library,
        BookFileStore& book_files,
        CborReaderCheckpointService& checkpoint,
        AppContextService& context
    );

    BookDeleteStatus remove(
        const BookId& book_id
    );

private:
    LibraryService& library_;
    BookFileStore& book_files_;
    CborReaderCheckpointService& checkpoint_;
    AppContextService& context_;

    bool rollback(
        const BookRecord& record,
        const std::string& source_bytes,
        bool source_existed,
        const std::optional<ReaderCheckpoint>& checkpoint_backup,
        const std::optional<AppRestoreContext>& context_backup
    );
};

} // namespace enku
