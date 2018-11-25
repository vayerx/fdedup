#include "fdedup.h"
#include <unordered_map>
#include <filesystem>
#include <cassert>

#ifdef DEBUG
#include <iostream>
#endif

using Path = std::filesystem::path;


// Find duplicates amoung files with same size and same hash
static void dedup_by_hash(
    IFsAccessor *a_fs,
    const std::vector<Path> &a_files,
    uintmax_t a_files_size,
    std::vector<Duplicate> &a_duplicates,
    DupStat &a_stat
) {
    assert(a_files.size() > 1);

    size_t max_hardlinks = 0;
    Path reachest_file;
    for (const Path &path: a_files) {
        const size_t hardlinks = a_fs->GetHardlinksCount(path);
        if (hardlinks > max_hardlinks) {
            max_hardlinks = hardlinks;
            reachest_file = path;
        }
    }
    assert(max_hardlinks > 0);

    std::vector<Path> duplicates;   // duplicates of reachest_file (excluding hardlinks)
    std::vector<Path> mismatched;   // remaining files
    for (const Path &path: a_files) {
        if (path == reachest_file) {
            continue;
        }

        if (!a_fs->IsEqual(path, reachest_file)) {
            if (a_fs->CompareFiles(path, reachest_file)) {
                duplicates.push_back(path);

                a_stat.duplicate_amount++;
                a_stat.duplicate_size += a_files_size;
            } else {
                mismatched.push_back(path);
            }
        }
    }

    if (!duplicates.empty()) {
        Duplicate duplicate{std::move(reachest_file), std::move(duplicates)};
        a_duplicates.push_back(std::move(duplicate));

#ifdef DEBUG
        std::cout << "Found " << duplicates.size() << " duplicates of " << reachest_file << ":" << std::endl;
        for (const Path &path: duplicates) {
            std::cout << "\t" << path << std::endl;
        }
#endif
    }

    if (mismatched.size() > 1) {
        dedup_by_hash(a_fs, mismatched, a_files_size, a_duplicates, a_stat);
    }
}

// Find duplicates amoung files with same size
static void dedup_by_size(
    IFsAccessor *a_fs,
    const std::vector<Path> &a_files,
    uintmax_t a_files_size,
    std::vector<Duplicate> &a_duplicates,
    DupStat &a_stat)
{
    /// @todo precalculate only first bytes for large files
    std::unordered_multimap<size_t, Path> hash2files;
    for (const Path &path: a_files) {
        hash2files.emplace(a_fs->GetFileHash(path), path);
    }

    for (auto isize_path = hash2files.cbegin(); isize_path != hash2files.cend(); ) {
        const auto &[hash, path] = *isize_path;
        const auto &[ibegin, iend] = hash2files.equal_range(hash);
        if (std::next(ibegin) != iend) {
            std::vector<Path> files;
            for (auto ifile = ibegin; ifile != iend; ++ifile) {
                files.push_back(ifile->second);
            }

            dedup_by_hash(a_fs, files, a_files_size, a_duplicates, a_stat);
        }

        isize_path = iend;
    }
}


// DupStat Deduplicate(IFsAccessor *a_fs, uintmax_t a_min_size, uintmax_t a_max_size)
std::tuple<std::vector<Duplicate>, DupStat> FindDuplicates(IFsAccessor *a_fs, uintmax_t a_min_size, uintmax_t a_max_size)
{
    DupStat stat;

    std::unordered_multimap<uintmax_t, Path> size2files;
    a_fs->IterateFiles([&](const Path &path, uintmax_t a_size) {
        if (a_size >= a_min_size && a_size <= a_max_size) {
            size2files.emplace(a_size, path);

            stat.processed_amount++;
            stat.processed_size += a_size;
        } else {
            stat.skipped_amount++;
            stat.skipped_size += a_size;
        }
    });

    std::vector<Duplicate> duplicates;
    for (auto isize_path = size2files.cbegin(); isize_path != size2files.cend(); ) {
        const auto &[size, path] = *isize_path;
        const auto &[ibegin, iend] = size2files.equal_range(size);
        if (std::next(ibegin) != iend) {
            std::vector<Path> files;
            for (auto i = ibegin; i != iend; ++i) {
                files.push_back(i->second);
            }

            dedup_by_size(a_fs, files, size, duplicates, stat);
        }

        isize_path = iend;
    }

    return {duplicates, stat};
}
