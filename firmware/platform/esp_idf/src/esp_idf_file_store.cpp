#include "enku/platform/esp_idf/esp_idf_file_store.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace enku::platform::esp_idf {
namespace {

bool containsTraversal(const std::string& path) {
    return
        path == ".." ||
        path.find("../") != std::string::npos ||
        path.find("/..") != std::string::npos;
}

StateFileStatus stateStatusFromErrno() {
    if (errno == ENOENT) {
        return StateFileStatus::NotFound;
    }

    if (errno == ENOSPC) {
        return StateFileStatus::NoSpace;
    }

    return StateFileStatus::IoError;
}

BookFileStatus bookStatusFromErrno() {
    if (errno == ENOENT) {
        return BookFileStatus::NotFound;
    }

    if (errno == ENOSPC) {
        return BookFileStatus::NoSpace;
    }

    return BookFileStatus::IoError;
}

} // namespace

EspIdfFileStore::EspIdfFileStore(
    std::string mount_point
)
    : mount_point_(std::move(mount_point)) {}

bool EspIdfFileStore::resolve(
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

bool EspIdfFileStore::ensureParentDirectories(
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

StateFileStatus EspIdfFileStore::read(
    const std::string& path,
    std::vector<std::uint8_t>& bytes
) {
    bytes.clear();

    std::string physical;
    if (!resolve(path, physical)) {
        return StateFileStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "rb"
    );

    if (file == nullptr) {
        return stateStatusFromErrno();
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return StateFileStatus::IoError;
    }

    const long size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return StateFileStatus::IoError;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return StateFileStatus::IoError;
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
            return StateFileStatus::IoError;
        }
    }

    std::fclose(file);
    return StateFileStatus::Ok;
}

StateFileStatus EspIdfFileStore::write(
    const std::string& path,
    const std::vector<std::uint8_t>& bytes
) {
    std::string physical;
    if (!resolve(path, physical) ||
        !ensureParentDirectories(physical)) {
        return StateFileStatus::IoError;
    }

    FILE* file = std::fopen(
        physical.c_str(),
        "wb"
    );

    if (file == nullptr) {
        return stateStatusFromErrno();
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
                stateStatusFromErrno();
            std::fclose(file);
            return status;
        }
    }

    if (std::fflush(file) != 0) {
        std::fclose(file);
        return StateFileStatus::IoError;
    }

    std::fclose(file);
    return StateFileStatus::Ok;
}

StateFileStatus EspIdfFileStore::remove(
    const std::string& path
) {
    std::string physical;
    if (!resolve(path, physical)) {
        return StateFileStatus::IoError;
    }

    if (std::remove(physical.c_str()) == 0) {
        return StateFileStatus::Ok;
    }

    return stateStatusFromErrno();
}

BookFileStatus EspIdfFileStore::read(
    const std::string& path,
    std::string& bytes
) {
    bytes.clear();

    std::string physical;
    if (!resolve(path, physical)) {
        return BookFileStatus::IoError;
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
        return BookFileStatus::IoError;
    }

    const long size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return BookFileStatus::IoError;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return BookFileStatus::IoError;
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
            return BookFileStatus::IoError;
        }
    }

    std::fclose(file);
    return BookFileStatus::Ok;
}

BookFileStatus EspIdfFileStore::write(
    const std::string& path,
    const std::string& bytes
) {
    std::string physical;
    if (!resolve(path, physical) ||
        !ensureParentDirectories(physical)) {
        return BookFileStatus::IoError;
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
        return BookFileStatus::IoError;
    }

    std::fclose(file);
    return BookFileStatus::Ok;
}

BookFileStatus EspIdfFileStore::remove(
    const std::string& path
) {
    std::string physical;
    if (!resolve(path, physical)) {
        return BookFileStatus::IoError;
    }

    if (std::remove(physical.c_str()) == 0) {
        return BookFileStatus::Ok;
    }

    return bookStatusFromErrno();
}

BookFileStatus EspIdfFileStore::list(
    const std::string& directory,
    std::vector<std::string>& paths
) {
    paths.clear();

    std::string physical;
    if (!resolve(directory, physical)) {
        return BookFileStatus::IoError;
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
            return BookFileStatus::IoError;
        }

        if (S_ISREG(info.st_mode)) {
            paths.push_back(
                logical_prefix + entry->d_name
            );
        }
    }

    ::closedir(dir);
    std::sort(paths.begin(), paths.end());
    return BookFileStatus::Ok;
}

} // namespace enku::platform::esp_idf
