#pragma once

#include <filesystem>

class FilePath
{
public:
    static void SetExecutablePath(const std::string_view path);
    
    FilePath() = default;
    explicit FilePath(const std::string_view path);

    std::string GetAbsolute() const;
    std::string GetDirectory() const;
    std::string GetFileName() const;
    std::string GetExtension() const;
    
    bool Exists() const;
    
    friend std::ostream& operator<<(std::ostream& os, const FilePath& fp);
    
private:
    std::filesystem::path path;
};
