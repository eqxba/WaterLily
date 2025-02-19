#pragma once

#include <filesystem>

class FilePath
{
public:
    static void SetExecutablePath(std::string_view path);
    
    FilePath() = default;
    explicit FilePath(std::string_view path);

    std::string GetAbsolute() const;
    std::string GetRelativeTo(const FilePath& base) const;
    std::string GetDirectory() const;
    std::string GetFileNameWithExtension() const;
    std::string GetFileName() const;
    std::string GetExtension() const;

    bool Exists() const;
    bool IsDirectory() const;
    
    FilePath operator/(std::string_view suffix) const;

    friend std::ostream& operator<<(std::ostream& os, const FilePath& fp);
    
private:
    std::filesystem::path path;
};
