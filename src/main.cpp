#include "fdedup.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;
namespace opt = boost::program_options;


int main(int argc, char **argv) try {
    fs::path start_path = ".";
    uintmax_t min_size = 0x1000, max_size = 0x40000000;
    std::string command_str;

    opt::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show help")
        ("min_size,m", opt::value(&min_size)->default_value(min_size), "Minimum file size")
        ("max_size,M", opt::value(&max_size)->default_value(max_size), "Maximum file size")
        ("apply,a", opt::value(&command_str)->default_value("print"), "Process duplicates: merge, print, shell");
    ;

    opt::positional_options_description pos_desc;
    pos_desc.add("root", 1);

    opt::options_description hidden;
    hidden.add_options()
        ("root", opt::value(&start_path)->default_value(start_path), "Root")
    ;
    opt::options_description all_opts;
    all_opts.add(desc);
    all_opts.add(hidden);

    opt::variables_map vm;
    opt::parsed_options parser = opt::command_line_parser(argc, argv).options(all_opts).positional(pos_desc).run();

    opt::store(parser, vm);
    opt::notify(vm);

    if (vm.count("help")) {
        std::cout
            << "File Deduplication\n"
            << "Usage: fdedup [options] [PATH]\n"
            << desc
            << std::endl;
        return EXIT_SUCCESS;
    }

    enum class Command {
        Print,
        Merge,
        // Remove, -- too dangerous ;-)
        Shell,
    } command;
    if (command_str == "print") {
        command = Command::Print;
    } else if (command_str == "merge") {
        command = Command::Merge;
    } else if (command_str == "shell") {
        command = Command::Shell;
    } else {
        throw std::invalid_argument("unknown command " + command_str);
    }

    FilesystemAccessor fs(start_path);
    const auto [duplicates, stat] = FindDuplicates(&fs, min_size, max_size);
    switch(command) {
        case Command::Print:
            for (const auto &[main, dups]: duplicates) {
                std::cout << main.string() << std::endl;
                std::copy(dups.begin(), dups.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
                std::cout << std::endl << std::endl;
            }
            break;

        case Command::Shell:
            for (const auto &[main, dups]: duplicates) {
                for (const auto &dup: dups) {
                    const fs::path temp = dup.string() + ".bak";    // unique_path() wasn't included to std::filesystem
                    std::cout
                        << "mv " << dup << " " << temp << " && "
                        << "cp -l " << main << " " << dup << " && "
                        << "rm -f " << temp
                        << std::endl;
                }
            }
            if (!duplicates.empty()) {
                std::cout << std::endl;
            }
            break;

        case Command::Merge:
            for (const auto &[main, dups]: duplicates) {
                for (const auto &dup: dups) {
                    std::cout << main << " --> " << dup << std::endl;

                    const fs::path temp = dup.string() + ".fdup-bak";   /// @todo ::mkstemp
                    if (!fs::exists(temp)) {
                        fs::rename(dup, temp);
                        try {
                            fs::create_hard_link(main, dup);
                        } catch(...) {
                            fs::rename(temp, dup);  // try to rename back
                            throw;
                        }
                        fs::remove(temp);
                    } else {
                        std::cerr << "Can't create temporary file for " << dup << std::endl;
                    }
                }
            }
            break;
    }

    std::cerr
        << "Processed:  " << stat.processed_amount << " files (" << (stat.processed_size / 0x100000) << " Mb)" << std::endl
        << "Skipped:    " << stat.skipped_amount << " files (" << (stat.skipped_size/ 0x100000) << " Mb)" << std::endl
        << "Duplicates: " << stat.duplicate_amount << " files (" << (stat.duplicate_size / 0x100000) << " Mb)" << std::endl;

    return EXIT_SUCCESS;
} catch (std::exception &x) {
    std::cerr << "Error: " << x.what() << std::endl;
    return EXIT_FAILURE;
}
