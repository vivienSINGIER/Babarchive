#include <iostream>

#include "Archive/Packer.h"
#include "Asset/AssetPacker.h"
#include "Asset/Scene.h"

int main(int argc, char* argv[])
{
#ifdef BABARCHIVE_WORKING_DIR
    fs::current_path(BABARCHIVE_WORKING_DIR);
#endif
    
    if (argc != 4)
    {
        Error e = Error::Io("Invalid argument count");
        std::cout << e.Format() << std::endl;
        std::cout << "Babarset - Command" << std::endl;
        std::cout << "<archive> <file> <name>" << std::endl;
        std::cout << "packs the file in the chosen archive under the given name" << std::endl;
        return e.Exit();
    }
    
    std::string archivePath = argv[1];
    std::string filePath = argv[2];
    std::string name = argv[3];
    
    Archive::Packer packer;
    if (Error e = packer.Open(archivePath))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }
    if (Error e = Asset::Pack(packer, filePath, name))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }
    
    return 0;
}
