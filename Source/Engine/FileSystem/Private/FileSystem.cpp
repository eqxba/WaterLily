#include "Engine/FileSystem/FileSystem.hpp"

#include <portable-file-dialogs.h>

namespace FileSystem
{
    std::vector<char> ReadFile(const FilePath& path)
    {
        if (!path.Exists())
        {
            LogE << "Path does not exist: " << path.GetAbsolute() << '\n';
            Assert(false);
        }
        
        std::ifstream file(path.GetAbsolute(), std::ios::binary | std::ios::ate);

        const std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        
        const auto& result = file.read(buffer.data(), fileSize);
        Assert(result.good());

        return buffer;
    }

    void CreateDirectories(const FilePath& path)
    {
        std::filesystem::create_directories(path.GetDirectory());
    }

    FilePath ShowOpenFileDialog(const DialogDescription& description)
    {
        std::vector<std::string> filterStrings;

        for (const auto& filter : description.filters)
        {
            filterStrings.emplace_back(filter.description);
            filterStrings.emplace_back(filter.pattern);
        }

        auto selection = pfd::open_file(std::string(description.title),
            description.folder.GetAbsolute(), filterStrings, pfd::opt::none).result();

        return selection.empty() ? FilePath{} : FilePath(selection.front());
    }
}
