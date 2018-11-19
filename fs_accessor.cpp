#include "fs_accessor.h"
#include "file_mapping.h"

#include <stdexcept>
#include <cstring>

namespace fs = std::filesystem;

void FilesystemAccessor::IterateFiles(std::function<void (const fs::path &, uintmax_t)> a_handler)
{
    for (fs::recursive_directory_iterator idir(m_root), iend; idir != iend; ++idir) {
        if (idir->is_regular_file() && !idir->is_symlink()) {
            a_handler(idir->path(), idir->file_size());
        }
    }
}

size_t FilesystemAccessor::GetFileHash(const fs::path &a_file)
{
    return MapFile(a_file.string().c_str(), [](const void *a_data, size_t a_size) {
       return std::hash<std::string_view>()(std::string_view(static_cast<const char *>(a_data), a_size));
    });
}

size_t FilesystemAccessor::GetHardlinksCount(const fs::path &a_file)
{
    return fs::hard_link_count(a_file);
}

bool FilesystemAccessor::IsEqual(const fs::path &a_first, const fs::path &a_second)
{
    return fs::equivalent(a_first, a_second);
}

bool FilesystemAccessor::CompareFiles(const fs::path &a_first, const fs::path &a_second)
{
    return MapFile(a_first.string().c_str(), [=](const void *a_first_data, size_t a_first_size) {
        return MapFile(a_second.string().c_str(), [=](const void *a_second_data, size_t a_second_size) {
            return a_first_size == a_second_size && std::memcmp(a_first_data, a_second_data, a_first_size) == 0;
        });
    });
}
