#include "Engine/FileSystem/FilePath.hpp"

namespace FilePathDetails
{
    static const std::filesystem::path* executableDir = nullptr;
}

void FilePath::SetExecutablePath(const std::string_view aPath)
{
    Assert(!FilePathDetails::executableDir);

    static std::filesystem::path path = std::filesystem::absolute(aPath).parent_path();
    FilePathDetails::executableDir = &path;
}

FilePath::FilePath(std::string_view aPath)
{
    Assert(FilePathDetails::executableDir);
    
    path = aPath.find("~/") == 0
        ? *FilePathDetails::executableDir / aPath.substr(2)
        : std::filesystem::path(aPath);

    path.make_preferred();
}

std::string FilePath::GetAbsolute() const
{
    return path.string();
}

std::string FilePath::GetDirectory() const
{
    return path.parent_path().string();
}

std::string FilePath::GetFileName() const
{
    return path.stem().string();
}

std::string FilePath::GetExtension() const
{
    return path.extension().string();
}

bool FilePath::Exists() const
{
    return std::filesystem::exists(path);
}

std::ostream& operator<<(std::ostream& os, const FilePath& fp)
{
    os << fp.path.string();
    return os;
}
