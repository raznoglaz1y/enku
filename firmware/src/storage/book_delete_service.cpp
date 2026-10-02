#include "enku/storage/book_delete_service.hpp"

#include <optional>
#include <string>

namespace enku {

BookDeleteService::BookDeleteService(
    LibraryService& library,
    BookFileStore& book_files,
    CborReaderCheckpointService& checkpoint,
    AppContextService& context,
    CborBookmarkService* bookmarks
)
    : library_(library),
      book_files_(book_files),
      checkpoint_(checkpoint),
      context_(context),
      bookmarks_(bookmarks) {}

bool BookDeleteService::rollback(
    const BookRecord& record,
    const std::string& source_bytes,
    bool source_existed,
    const std::optional<ReaderCheckpoint>& checkpoint_backup,
    const std::optional<AppRestoreContext>& context_backup,
    const std::optional<std::vector<BookmarkRecord>>& bookmarks_backup
) {
    bool ok = true;

    if (source_existed) {
        ok =
            book_files_.write(
                record.source_path,
                source_bytes
            ) == BookFileStatus::Ok &&
            ok;
    }

    if (library_.upsert(record) != LibraryStatus::Ok) {
        ok = false;
    }

    if (checkpoint_backup.has_value()) {
        const auto& saved = *checkpoint_backup;
        if (checkpoint_.checkpoint(
                record.book_id,
                saved.position,
                saved.progress,
                saved.reading_state
            ) != PersistStatus::Ok) {
            ok = false;
        }
    }

    if (bookmarks_ != nullptr &&
        bookmarks_backup.has_value()) {
        if (bookmarks_->replace(
                record.book_id,
                *bookmarks_backup
            ) != BookmarkStatus::Ok) {
            ok = false;
        }
    }

    if (context_backup.has_value()) {
        if (context_.save(*context_backup) != PersistStatus::Ok) {
            ok = false;
        }
    }

    return ok;
}

BookDeleteStatus BookDeleteService::remove(
    const BookId& book_id
) {
    const auto record = library_.get(book_id);
    if (!record.has_value()) {
        return BookDeleteStatus::NotFound;
    }

    std::string source_bytes;
    bool source_existed = false;

    const auto source_status =
        book_files_.read(
            record->source_path,
            source_bytes
        );

    if (source_status == BookFileStatus::Ok) {
        source_existed = true;
    } else if (source_status != BookFileStatus::NotFound) {
        return BookDeleteStatus::SourceReadFailed;
    }

    std::optional<ReaderCheckpoint> checkpoint_backup;
    ReaderCheckpoint checkpoint_value;
    const auto checkpoint_status =
        checkpoint_.load(book_id, checkpoint_value);

    if (checkpoint_status == PersistStatus::Ok) {
        checkpoint_backup = checkpoint_value;
    }

    std::optional<std::vector<BookmarkRecord>>
        bookmarks_backup;

    if (bookmarks_ != nullptr) {
        std::vector<BookmarkRecord> stored_bookmarks;
        const auto bookmark_status =
            bookmarks_->load(
                book_id,
                stored_bookmarks
            );

        if (bookmark_status == BookmarkStatus::Ok) {
            bookmarks_backup =
                std::move(stored_bookmarks);
        } else if (
            bookmark_status != BookmarkStatus::NotFound) {
            return BookDeleteStatus::BookmarkLoadFailed;
        }
    }

    std::optional<AppRestoreContext> context_backup;
    AppRestoreContext context_value;
    const auto context_status =
        context_.load(context_value);

    if (context_status == PersistStatus::Ok) {
        context_backup = context_value;
    } else if (context_status != PersistStatus::NotFound) {
        return BookDeleteStatus::ContextLoadFailed;
    }

    const bool context_points_to_book =
        context_backup.has_value() &&
        context_backup->screen == Screen::Reading &&
        context_backup->current_book ==
            std::optional<BookId>{book_id};

    if (context_points_to_book) {
        const auto preserved_focus =
            context_backup->library_focused_book ==
                    std::optional<BookId>{book_id}
                ? std::optional<BookId>{}
                : context_backup->library_focused_book;

        if (context_.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    context_backup->library_offset,
                    preserved_focus,
                }
            ) != PersistStatus::Ok) {
            return BookDeleteStatus::ContextSaveFailed;
        }
    }

    if (library_.remove(book_id) != LibraryStatus::Ok) {
        if (context_points_to_book &&
            context_backup.has_value()) {
            if (context_.save(*context_backup) != PersistStatus::Ok) {
                return BookDeleteStatus::RollbackFailed;
            }
        }
        return BookDeleteStatus::LibraryRemoveFailed;
    }

    if (source_existed) {
        const auto remove_source =
            book_files_.remove(record->source_path);

        if (remove_source != BookFileStatus::Ok) {
            return rollback(
                *record,
                source_bytes,
                source_existed,
                checkpoint_backup,
                context_points_to_book
                    ? context_backup
                    : std::nullopt,
                bookmarks_backup
            )
                ? BookDeleteStatus::SourceRemoveFailed
                : BookDeleteStatus::RollbackFailed;
        }
    }

    if (bookmarks_ != nullptr &&
        bookmarks_->eraseBook(book_id) !=
            BookmarkStatus::Ok) {
        return rollback(
            *record,
            source_bytes,
            source_existed,
            checkpoint_backup,
            context_points_to_book
                ? context_backup
                : std::nullopt,
            bookmarks_backup
        )
            ? BookDeleteStatus::BookmarkRemoveFailed
            : BookDeleteStatus::RollbackFailed;
    }

    if (checkpoint_.erase(book_id) != PersistStatus::Ok) {
        return rollback(
            *record,
            source_bytes,
            source_existed,
            checkpoint_backup,
            context_points_to_book
                ? context_backup
                : std::nullopt
        )
            ? BookDeleteStatus::CheckpointRemoveFailed
            : BookDeleteStatus::RollbackFailed;
    }

    return BookDeleteStatus::Ok;
}

} // namespace enku
