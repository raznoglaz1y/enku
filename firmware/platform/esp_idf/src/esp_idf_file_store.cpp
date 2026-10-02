#include "enku/platform/esp_idf/esp_idf_file_store.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <utility>

namespace enku::platform::esp_idf {
namespace {

bool containsTraversal(const std::string& path) {
    return
        path == ".." ||
        path.find("../") != std::string::npos ||
        path.find("/..") != std::string::npos;
}

EspIdfFsStatus fsStatusFromErrno() {
    if (errno == ENOENT) {
        return EspIdfFsStatus::NotFound;
    }

    if (errno == ENOSPC) {
        return EspIdfFsStatus::NoSpace;
    }

    return EspIdfFsStatus::IoError;
}

EspIdfFsStatus bookStatusFromErrno() {
    if (errno == ENOENT) {
        return EspIdfFsStatus::NotFound;
    }

    if (errno == ENOSPC) {
        return EspIdfFsStatus::NoSpace;
    }

    return EspIdfFsStatus::IoError;
}

} // namespace

EspIdfFilesystem::EspIdfFilesystem(
    std::string mount_point
)
    : mount_point_(std::move(mount_point)) {}

bool EspIdfFilesystem::resolve(
    const std::string& logical_path,
    std::string& physical_path
) const {
    if (logical_path.empty() ||
        containsTraversal(logical_path)) {
        return false;
    }

    if (logical_path.front() == '/') {
        physical_path =
            mount_point_ + logical_path;
    } else {
        physical_path =
            mount_point_ + "/" + logical_path;
    }

    return true;
}

bool EspIdfFilesystem::ensureParentDirectories(
    const std::string& physical_path
) const {
    std::size_t pos =
        physical_path.find('/', 1);

    while (pos != std::string::npos) {
        const auto dir =
            physical_path.substr(0, pos);

        if (!dir.empty() &&
            ::mkdir(dir.c_str(), 0775) != 0 &&
            errno != EEXIST) {
            return false;
        }

        pos = physical_path.find('/', pos + 1);
    }

    return true;
}

EspIdfFsStatus EspIdfFilesystem::readBytes(
    const std::string& path,
    std::vector<std::uint8_t>& bytes
) {
    bytes.clear();

    std::string physical;
    if (!resolve(path, physical)) {
        return EspIdfFsStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "rb"
    );

    if (file == nullptr) {
        return fsStatusFromErrno();
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    const long size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    bytes.resize(static_cast<std::size_t>(size));

    if (!bytes.empty()) {
        const auto read_count =
            std::fread(
                bytes.data(),
                1,
                bytes.size(),
                file
            );

        if (read_count != bytes.size()) {
            std::fclose(file);
            bytes.clear();
            return EspIdfFsStatus::IoError;
        }
    }

    std::fclose(file);
    return EspIdfFsStatus::Ok;
}

EspIdfFsStatus EspIdfFilesystem::writeBytes(
    const std::string& path,
    const std::vector<std::uint8_t>& bytes
) {
    std::string physical;
    if (!resolve(path, physical) ||
        !ensureParentDirectories(physical)) {
        return EspIdfFsStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "wb"
    );

    if (file == nullptr) {
        return fsStatusFromErrno();
    }

    if (!bytes.empty()) {
        const auto written =
            std::fwrite(
                bytes.data(),
                1,
                bytes.size(),
                file
            );

        if (written != bytes.size()) {
            const auto status =
                fsStatusFromErrno();
            std::fclose(file);
            return status;
        }
    }

    if (std::fflush(file) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    std::fclose(file);
    return EspIdfFsStatus::Ok;
}

EspIdfFsStatus EspIdfFilesystem::appendText(
    const std::string& path,
    const std::string& bytes
) {
    std::string physical;
    if (!resolve(path, physical) ||
        !ensureParentDirectories(physical)) {
        return EspIdfFsStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "ab"
    );

    if (file == nullptr) {
        return bookStatusFromErrno();
    }

    if (!bytes.empty()) {
        const auto written =
            std::fwrite(
                bytes.data(),
                1,
                bytes.size(),
                file
            );

        if (written != bytes.size()) {
            const auto status =
                bookStatusFromErrno();
            std::fclose(file);
            return status;
        }
    }

    if (std::fflush(file) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    std::fclose(file);
    return EspIdfFsStatus::Ok;
}

EspIdfFsStatus EspIdfFilesystem::remove(
    const std::string& path
) {
    std::string physical;
    if (!resolve(path, physical)) {
        return EspIdfFsStatus::IoError;
    }

    if (std::remove(physical.c_str()) == 0) {
        return EspIdfFsStatus::Ok;
    }

    return fsStatusFromErrno();
}

EspIdfFsStatus EspIdfFilesystem::readText(
    const std::string& path,
    std::string& bytes
) {
    bytes.clear();

    std::string physical;
    if (!resolve(path, physical)) {
        return EspIdfFsStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "rb"
    );

    if (file == nullptr) {
        return bookStatusFromErrno();
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    const long size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    bytes.resize(static_cast<std::size_t>(size));

    if (!bytes.empty()) {
        const auto read_count =
            std::fread(
                bytes.data(),
                1,
                bytes.size(),
                file
            );

        if (read_count != bytes.size()) {
            std::fclose(file);
            bytes.clear();
            return EspIdfFsStatus::IoError;
        }
    }

    std::fclose(file);
    return EspIdfFsStatus::Ok;
}

EspIdfFsStatus EspIdfFilesystem::writeText(
    const std::string& path,
    const std::string& bytes
) {
    std::string physical;
    if (!resolve(path, physical) ||
        !ensureParentDirectories(physical)) {
        return EspIdfFsStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "wb"
    );

    if (file == nullptr) {
        return bookStatusFromErrno();
    }

    if (!bytes.empty()) {
        const auto written =
            std::fwrite(
                bytes.data(),
                1,
                bytes.size(),
                file
            );

        if (written != bytes.size()) {
            const auto status =
                bookStatusFromErrno();
            std::fclose(file);
            return status;
        }
    }

    if (std::fflush(file) != 0) {
        std::fclose(file);
        return EspIdfFsStatus::IoError;
    }

    std::fclose(file);
    return EspIdfFsStatus::Ok;
}

EspIdfFsStatus EspIdfFilesystem::list(
    const std::string& directory,
    std::vector<std::string>& paths
) {
    paths.clear();

    std::string physical;
    if (!resolve(directory, physical)) {
        return EspIdfFsStatus::IoError;
    }

    DIR* dir = ::opendir(physical.c_str());
    if (dir == nullptr) {
        return bookStatusFromErrno();
    }

    const std::string logical_prefix =
        directory.empty() || directory.back() == '/'
            ? directory
            : directory + "/";

    while (const dirent* entry = ::readdir(dir)) {
        if (std::strcmp(entry->d_name, ".") == 0 ||
            std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string child_physical =
            physical + "/" + entry->d_name;

        struct stat info = {};
        if (::stat(child_physical.c_str(), &info) != 0) {
            ::closedir(dir);
            return EspIdfFsStatus::IoError;
        }

        if (S_ISREG(info.st_mode)) {
            paths.push_back(
                logical_prefix + entry->d_name
            );
        }
    }

    ::closedir(dir);
    std::sort(paths.begin(), paths.end());
    return EspIdfFsStatus::Ok;
}

namespace {

StateFileStatus toStateStatus(EspIdfFsStatus status) {
    switch (status) {
        case EspIdfFsStatus::Ok:
            return StateFileStatus::Ok;
        case EspIdfFsStatus::NotFound:
            return StateFileStatus::NotFound;
        case EspIdfFsStatus::NoSpace:
            return StateFileStatus::NoSpace;
        case EspIdfFsStatus::IoError:
        default:
            return StateFileStatus::IoError;
    }
}

BookFileStatus toBookStatus(EspIdfFsStatus status) {
    switch (status) {
        case EspIdfFsStatus::Ok:
            return BookFileStatus::Ok;
        case EspIdfFsStatus::NotFound:
            return BookFileStatus::NotFound;
        case EspIdfFsStatus::NoSpace:
            return BookFileStatus::NoSpace;
        case EspIdfFsStatus::IoError:
        default:
            return BookFileStatus::IoError;
    }
}

} // namespace

EspIdfStateFileStore::EspIdfStateFileStore(
    EspIdfFilesystem& filesystem
)
    : filesystem_(filesystem) {}

StateFileStatus EspIdfStateFileStore::read(
    const std::string& path,
    std::vector<std::uint8_t>& bytes
) {
    return toStateStatus(
        filesystem_.readBytes(path, bytes)
    );
}

StateFileStatus EspIdfStateFileStore::write(
    const std::string& path,
    const std::vector<std::uint8_t>& bytes
) {
    return toStateStatus(
        filesystem_.writeBytes(path, bytes)
    );
}

StateFileStatus EspIdfStateFileStore::remove(
    const std::string& path
) {
    return toStateStatus(
        filesystem_.remove(path)
    );
}

EspIdfBookFileStore::EspIdfBookFileStore(
    EspIdfFilesystem& filesystem
)
    : filesystem_(filesystem) {}

BookFileStatus EspIdfBookFileStore::read(
    const std::string& path,
    std::string& bytes
) {
    return toBookStatus(
        filesystem_.readText(path, bytes)
    );
}

BookFileStatus EspIdfBookFileStore::write(
    const std::string& path,
    const std::string& bytes
) {
    return toBookStatus(
        filesystem_.writeText(path, bytes)
    );
}

BookFileStatus EspIdfBookFileStore::append(
    const std::string& path,
    const std::string& bytes
) {
    return toBookStatus(
        filesystem_.appendText(path, bytes)
    );
}

BookFileStatus EspIdfBookFileStore::remove(
    const std::string& path
) {
    return toBookStatus(
        filesystem_.remove(path)
    );
}

BookFileStatus EspIdfBookFileStore::list(
    const std::string& directory,
    std::vector<std::string>& paths
) {
    return toBookStatus(
        filesystem_.list(directory, paths)
    );
}

} // namespace enku::platform::esp_idf
