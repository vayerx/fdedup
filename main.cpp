#include <iostream>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <string_view>

#include "file_mapping.h"

namespace fs = std::filesystem;
namespace opt = boost::program_options;



struct DupStat {
    uintmax_t   processed_size = 0;
    uintmax_t   processed_amount = 0;
    uintmax_t   skipped_size = 0;
    uintmax_t   skipped_amount = 0;
    uintmax_t   duplicate_size = 0;
    uintmax_t   duplicate_amount = 0;
};

// void create_hard_link(const path& to, const path& new_hard_link);
void find_duplicates(const std::vector<fs::path> &a_files, uintmax_t a_files_size, DupStat &a_stat) {
    /// @todo precalculate only first bytes for large files
    std::unordered_multimap<size_t, fs::path> hash2files;
    std::hash<std::string_view> hash;
    for (const fs::path &path: a_files) {
        MapFile(path.string().c_str(), [&](const void *a_data, size_t a_size) {
            hash2files.emplace(hash(std::string_view(static_cast<const char *>(a_data), a_size)), path);
        });
    }

    for (auto isize_path = hash2files.cbegin(); isize_path != hash2files.cend(); ) {
        const auto &[hash, path] = *isize_path;
        const auto &[ibegin, iend] = hash2files.equal_range(hash);
        if (std::next(ibegin) != iend) {
            std::cout << "Maybe duplicate: " << path << " (" << std::hex << hash << std::dec << ")" << std::endl;
            a_stat.duplicate_size += (std::distance(ibegin, iend) - 1) * a_files_size;
            a_stat.duplicate_amount += (std::distance(ibegin, iend) - 1);
        }

        isize_path = iend;
    }
}

int main(int argc, char **argv) {
    opt::options_description desc("General options");
    fs::path start_path = fs::current_path();
    uintmax_t min_size = 0x1000, max_size = 0x40000000;
    desc.add_options()
        ("help,h", "Show help")
        ("root,r", opt::value(&start_path)/*->default_value(start_path)*/, "Root path")
        ("min_size,m", opt::value(&min_size)->default_value(min_size), "Minimum file size")
        ("max_size,M", opt::value(&max_size)->default_value(max_size), "Maximum file size")
    ;

    opt::variables_map vm;
    opt::parsed_options parser = opt::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    opt::store(parser, vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout << "File Deduplication\n" << desc << std::endl;
        return EXIT_SUCCESS;
    }

    DupStat stat;
    std::cout << "Loading tree..." << std::endl;
    std::unordered_multimap<uintmax_t, fs::path> size2files;
    for (fs::recursive_directory_iterator idir(start_path), iend; idir != iend; ++idir) {
        if (idir->is_regular_file()) {
            const uintmax_t size = idir->file_size();
            if (size >= min_size && size <= max_size) {
                size2files.emplace(size, idir->path());

                stat.processed_amount++;
                stat.processed_size += size;
            } else {
                stat.skipped_amount++;
                stat.skipped_size += size;
            }
        }
    }

    std::cout << "Processing..." << std::endl;
    for (auto isize_path = size2files.cbegin(); isize_path != size2files.cend(); ) {
        const auto &[size, path] = *isize_path;
        const auto &[ibegin, iend] = size2files.equal_range(size);
        if (std::next(ibegin) != iend) {
            std::cout << "size = " << size << ", amount = " << std::distance(ibegin, iend) << std::endl;
            std::vector<fs::path> files;
            for (auto i = ibegin; i != iend; ++i) {
                files.push_back(ibegin->second);
                std::cout << "\t" << i->second << ": " << i->first << std::endl;
            }

            find_duplicates(files, size, stat);
        }

        isize_path = iend;
    }

    std::cout
        << "Processed:  " << stat.processed_amount << " files (" << (stat.processed_size / 0x100000) << " Mb)" << std::endl
        << "Skipped:    " << stat.skipped_amount << " files (" << (stat.skipped_size/ 0x100000) << " Mb)" << std::endl
        << "Duplicates: " << stat.duplicate_amount << " files (" << (stat.duplicate_size / 0x100000) << " Mb)" << std::endl;
    return 0;
}
