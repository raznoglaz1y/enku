#include "enku/storage/posix_state_file_store.hpp"

#include <fstream>
#include <system_error>

namespace enku {

PosixStateFileStore::PosixStateFileStore(
    std::filesystem::path root
)
    : root_(std::move(root)) {}

const std::filesystem::path&
PosixStateFileStore::root() const {
    return root_;
}

std::filesystem::path
PosixStateFileStore::resolve(
    const std::string& path
) const {
    std::filesystem::path relative(path);

    if (relative.is_absolute()) {
        relative = relative.relative_path();
    }

    return root_ / relative;
}

StateFileStatus PosixStateFileStore::read(
    const std::string& path,
    std::vector<std::uint8_t>& bytes
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    if (!std::filesystem::exists(full_path, ec)) {
        if (ec) {
            return StateFileStatus::IoError;
        }
        return StateFileStatus::NotFound;
    }

    std::ifstream input(
        full_path,
        std::ios::binary | std::ios::ate
    );

    if (!input) {
        return StateFileStatus::IoError;
    }

    const auto size = input.tellg();
    if (size < 0) {
        return StateFileStatus::IoError;
    }

    bytes.resize(static_cast<std::size_t>(size));

    input.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );

        if (!input) {
            return StateFileStatus::IoError;
        }
    }

    return StateFileStatus::Ok;
}

StateFileStatus PosixStateFileStore::write(
    const std::string& path,
    const std::vector<std::uint8_t>& bytes
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    std::filesystem::create_directories(
        full_path.parent_path(),
        ec
    );

    if (ec) {
        return StateFileStatus::IoError;
    }

    std::ofstream output(
        full_path,
        std::ios::binary |
        std::ios::trunc
    );

    if (!output) {
        return StateFileStatus::IoError;
    }

    if (!bytes.empty()) {
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
    }

    output.flush();

    if (!output) {
        return StateFileStatus::IoError;
    }

    return StateFileStatus::Ok;
}

} // namespace enku
