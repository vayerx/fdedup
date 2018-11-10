#pragma once

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <filesystem>
#include <cstddef>

struct FileMappingError: std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<typename Handler>
void MapFile(
    const char *a_filename,
    Handler a_handler,
    boost::interprocess::mode_t a_mode = boost::interprocess::read_only,
    boost::interprocess::mapped_region::advice_types a_advice = boost::interprocess::mapped_region::advice_sequential
) {
    if (!std::filesystem::is_regular_file(a_filename)) {
        throw FileMappingError(std::string("File is missing or is not a regular file: ") + a_filename);
    }

    try {
        if (std::filesystem::file_size(a_filename)) {
            boost::interprocess::file_mapping mapping(a_filename, a_mode);
            boost::interprocess::mapped_region region(mapping, a_mode);
            region.advise(a_advice);

            a_handler(region.get_address(), region.get_size());
        } else {
            // call handler for empty files too
            a_handler(nullptr, 0);
        }
    } catch (boost::interprocess::interprocess_exception &x) {
        throw FileMappingError(std::string("Can't map file \"") + a_filename + "\": " + x.what());
    }
}
