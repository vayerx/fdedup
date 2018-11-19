#pragma once

#include "fs_accessor.h"
#include <stdint.h>
#include <tuple>

struct DupStat {
    uintmax_t   processed_size = 0;
    uintmax_t   processed_amount = 0;
    uintmax_t   skipped_size = 0;
    uintmax_t   skipped_amount = 0;
    uintmax_t   duplicate_size = 0;
    uintmax_t   duplicate_amount = 0;
};

struct Duplicate {
    std::filesystem::path   main_file;
    std::vector<std::filesystem::path> duplicates;
};

std::tuple<std::vector<Duplicate>, DupStat> FindDuplicates(IFsAccessor *a_fs, uintmax_t a_min_size, uintmax_t a_max_size);
