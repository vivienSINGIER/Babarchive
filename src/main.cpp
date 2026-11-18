#include <iostream>

#include "Archive/Packer.h"

int main()
{
#ifdef BABARCHIVE_WORKING_DIR
    fs::current_path(BABARCHIVE_WORKING_DIR);
#endif
    
    Archive::Packer packer;
    
    Error e = packer.Create("Test", "archive");
    
    if (e)
    {
        e = packer.Open("Test/archive.babar");
        if (e)
        {
            std::cout << e.Format() << "\n";
            return e.Exit();
        }
    }
    
    std::string key = "ceci est une clé de test";
    
    // e = packer.Add("Test/test.txt", "Patate/test", Archive::FileFlags::Compressed, Archive::FileType::Text);
    // std::cout << e.Format() << "\n";
    //
    // e = packer.Add("Test/testVE2.txt", "test2", Archive::FileFlags::None, Archive::FileType::Text, key);
    // std::cout << e.Format() << "\n";
    //
    // e = packer.Add("Test/babar.jpg", "image", Archive::FileFlags::None, Archive::FileType::Binary);
    // std::cout << e.Format() << "\n";
    
    e = packer.Add("Test/ToPack", "test", Archive::FileFlags::None);
    
    // e = packer.Remove("test");
    // std::cout << e.Format() << "\n";
    
    std::vector<Archive::FileInfo> fileInfo;
    e = packer.ListInfo(fileInfo);
    
    // for (Archive::FileInfo& f : fileInfo)
    // {
    //     std::cout << "FileInfo:\n"
    //           << "  name:         " << f.name << '\n'
    //           << "  offset:       " << f.offset << '\n'
    //           << "  currentSize:  " << f.currentSize << '\n'
    //           << "  dataSize:     " << f.dataSize << '\n'
    //           << "  isCompressed: " << std::boolalpha << f.isCompressed << '\n'
    //           << "  isEncrypted:  " << std::boolalpha << f.isEncrypted << '\n'
    //           << "  extension:    " << f.extension << '\n';
    // }
    
    // std::unordered_map<std::string, std::string> keyMap;
    // keyMap["test2"] = key;
    
    e = packer.Unpack("Test/Unpack");
    std::cout << e.Format() << "\n";
    
    return 0;
}
