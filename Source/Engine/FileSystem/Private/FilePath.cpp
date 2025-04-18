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

std::string FilePath::GetRelativeTo(const FilePath& base) const
{
    return std::filesystem::relative(path, base.path).string();
}

std::string FilePath::GetDirectory() const
{
    return path.parent_path().string();
}

std::string FilePath::GetFileNameWithExtension() const
{
    return path.filename().string();
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

bool FilePath::IsDirectory() const
{
    return std::filesystem::is_directory(path);
}

FilePath FilePath::operator/(const std::string_view suffix) const
{
    return FilePath((path / suffix).string());
}

std::ostream& operator<<(std::ostream& os, const FilePath& fp)
{
    os << fp.path.string();
    return os;
}
