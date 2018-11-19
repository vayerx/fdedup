#pragma once

#include <functional>
#include <filesystem>

class IFsAccessor
{
public:
    virtual ~IFsAccessor() = default;
    virtual void IterateFiles(std::function<void (const std::filesystem::path &, uintmax_t)> a_handler) = 0;
    virtual size_t GetFileHash(const std::filesystem::path &a_file) = 0;
    virtual size_t GetHardlinksCount(const std::filesystem::path &a_file) = 0;
    virtual bool IsEqual(const std::filesystem::path &a_first, const std::filesystem::path &a_second) = 0;
    virtual bool CompareFiles(const std::filesystem::path &a_first, const std::filesystem::path &a_second) = 0;
};

class FilesystemAccessor
    : public IFsAccessor
{
public:
    FilesystemAccessor(const std::filesystem::path &a_root)
        : m_root(a_root)
        { }

    void IterateFiles(std::function<void (const std::filesystem::path &, uintmax_t)> a_handler) override;
    size_t GetFileHash(const std::filesystem::path &a_file) override;
    size_t GetHardlinksCount(const std::filesystem::path &a_file) override;
    bool IsEqual(const std::filesystem::path &a_first, const std::filesystem::path &a_second) override;
    bool CompareFiles(const std::filesystem::path &a_first, const std::filesystem::path &a_second) override;

private:
    std::filesystem::path m_root;
};
