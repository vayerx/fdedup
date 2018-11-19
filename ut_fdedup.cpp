#define BOOST_TEST_MODULE fdedup
#include <boost/test/unit_test.hpp>

#include "fdedup.h"

#include <fstream>

namespace fs = std::filesystem;

struct TestFile
{
    uintmax_t size;
    size_t hash;
    uintmax_t inode;
    std::string contents;
};

class TestFilesystemAccessor
    : public IFsAccessor
{
public:
    TestFilesystemAccessor(const std::map<fs::path, TestFile> &a_files)
        : m_files(a_files)
        { }

    void IterateFiles(std::function<void (const fs::path &, uintmax_t)> a_handler) override {
        for (const auto &[path, file]: m_files) {
            // BOOST_TEST_MESSAGE("Checking " + path.string());
            a_handler(path, file.size);
        }
    }

    size_t GetFileHash(const fs::path &a_file) override {
        return m_files[a_file].hash;
    }

    size_t GetHardlinksCount(const std::filesystem::path &a_file) override {
        uintmax_t inode = m_files[a_file].inode;
        size_t count = 0;
        for (const auto &[path, file]: m_files) {
            if (file.inode == inode) {
                ++count;
            }
        }
        return count;
    }

    bool IsEqual(const std::filesystem::path &a_first, const std::filesystem::path &a_second) override {
        return m_files[a_first].inode == m_files[a_second].inode;
    }

    bool CompareFiles(const std::filesystem::path &a_first, const std::filesystem::path &a_second) override {
        return m_files[a_first].contents == m_files[a_second].contents;
    }

private:
    std::map<fs::path, TestFile> m_files;
};

BOOST_AUTO_TEST_SUITE(fdedup)

BOOST_AUTO_TEST_CASE(invariants)
{
    const fs::path dir = "_fdedup_utest";
    const fs::path file = dir / "file";
    const fs::path standalone = dir / "standalone";
    const fs::path symlink = dir / "symlink";
    const fs::path hardlink = dir / "hardlink";

    fs::remove_all(dir);
    fs::create_directory(dir);
    std::ofstream(file.string());
    std::ofstream(standalone.string());
    fs::create_symlink(fs::absolute(file), symlink);
    fs::create_hard_link(file, hardlink);

    BOOST_CHECK(fs::is_directory(dir));
    BOOST_CHECK(fs::is_regular_file(file));
    BOOST_CHECK(fs::is_regular_file(hardlink));
    BOOST_CHECK(fs::is_regular_file(symlink));
    BOOST_CHECK_EQUAL(1, fs::hard_link_count(standalone));
    BOOST_CHECK_EQUAL(2, fs::hard_link_count(file));
}

BOOST_AUTO_TEST_CASE(basic)
{
    TestFilesystemAccessor fs{{
        {"a", {10, 0xBEAF, 1, "abc"}},
        {"b", {10, 0xBEAF, 2, "abc"}},
        {"c", {20, 0xDEAD, 3, "DEF"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 1, 100);

    BOOST_CHECK_EQUAL(3, stat.processed_amount);
    BOOST_CHECK_EQUAL(40, stat.processed_size);
    BOOST_CHECK_EQUAL(1, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(10, stat.duplicate_size);
    BOOST_CHECK_EQUAL(0, stat.skipped_amount);
}

BOOST_AUTO_TEST_CASE(multiple)
{
    TestFilesystemAccessor fs{{
        {"a", {10, 0xBEAF, 1, "abc"}},
        {"b", {10, 0xBEAF, 2, "abc"}},
        {"c", {20, 0xDEAD, 3, "DEF"}},
        {"x", {30, 0xBABE, 4, "xyz"}},
        {"y", {30, 0xBABE, 5, "xyz"}},
        {"z", {30, 0xBABE, 6, "xyz"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 1, 100);

    BOOST_CHECK_EQUAL(6, stat.processed_amount);
    BOOST_CHECK_EQUAL(130, stat.processed_size);
    BOOST_CHECK_EQUAL(3, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(70, stat.duplicate_size);
    BOOST_CHECK_EQUAL(0, stat.skipped_amount);
}

BOOST_AUTO_TEST_CASE(collisions)
{
    TestFilesystemAccessor fs{{
        {"a", {10, 0xBEAF, 1, "abc"}},
        {"b", {10, 0xBEAF, 2, "XYZ"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 1, 100);

    BOOST_CHECK_EQUAL(2, stat.processed_amount);
    BOOST_CHECK_EQUAL(20, stat.processed_size);
    BOOST_CHECK_EQUAL(0, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(0, stat.duplicate_size);
    BOOST_CHECK_EQUAL(0, stat.skipped_amount);
}

BOOST_AUTO_TEST_CASE(duplicates_with_collisions)
{
    TestFilesystemAccessor fs{{
        {"aa", {10, 0xBEAF, 1, "abc"}},
        {"ab", {10, 0xBEAF, 2, "abc"}},
        {"xx", {10, 0xBEAF, 3, "XYZ"}},
        {"xy", {10, 0xBEAF, 4, "XYZ"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 1, 100);

    BOOST_CHECK_EQUAL(4, stat.processed_amount);
    BOOST_CHECK_EQUAL(40, stat.processed_size);
    BOOST_CHECK_EQUAL(2, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(20, stat.duplicate_size);
    BOOST_CHECK_EQUAL(0, stat.skipped_amount);
}

BOOST_AUTO_TEST_CASE(duplicates_partial_merged)
{
    TestFilesystemAccessor fs{{
        {"aa", {10, 0xBEAF, 1, "abc"}},
        {"ab", {10, 0xBEAF, 1, "abc"}},
        {"ac", {10, 0xBEAF, 2, "abc"}},
        {"xx", {10, 0xDEAD, 3, "XYZ"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 1, 100);

    BOOST_CHECK_EQUAL(4, stat.processed_amount);
    BOOST_CHECK_EQUAL(40, stat.processed_size);
    BOOST_CHECK_EQUAL(1, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(10, stat.duplicate_size);
    BOOST_CHECK_EQUAL(0, stat.skipped_amount);
}

BOOST_AUTO_TEST_CASE(skip_by_size)
{
    TestFilesystemAccessor fs{{
        {"a", {1, 0xBEAF, 1, "abc"}},
        {"b", {2, 0xBEAF, 2, "abc"}},
        {"c", {10, 0xDEAD, 3, "DEF"}},
        {"x", {100, 0xBABE, 4, "xyz"}},
        {"y", {200, 0xBABE, 5, "xyz"}},
        {"z", {300, 0xBABE, 6, "xyz"}},
    }};

    const auto [duplicates, stat] = FindDuplicates(&fs, 10, 100);

    BOOST_CHECK_EQUAL(2, stat.processed_amount);
    BOOST_CHECK_EQUAL(110, stat.processed_size);
    BOOST_CHECK_EQUAL(0, stat.duplicate_amount);
    BOOST_CHECK_EQUAL(0, stat.duplicate_size);
    BOOST_CHECK_EQUAL(4, stat.skipped_amount);
    BOOST_CHECK_EQUAL(503, stat.skipped_size);
}

BOOST_AUTO_TEST_SUITE_END()
