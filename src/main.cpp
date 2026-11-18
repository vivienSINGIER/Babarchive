#include <iostream>

#include "Archive/Packer.h"

int main(int argc, char* argv[])
{
#ifdef BABARCHIVE_WORKING_DIR
    fs::current_path(BABARCHIVE_WORKING_DIR);
#endif
    
    if (argc < 2) 
    {
        std::cerr << "Erreur : arguments manquants. Commandes d'aides : --help / --h \n";
        return 1;
    }
    
    std::string mainCommand = argv[1];
    if (mainCommand == "--help" || mainCommand == "--h") 
    {
        std::cout << "Affichage de l'aide...\n";
        std::cout << "--create / --c <archive path> <archive name>\n";
        std::cout << "--pack / --p <archive path> <file path | directory path> <name>\n";
        std::cout << "--unpack / --u <archive path> <unpack location>\n";
    }
    else if (mainCommand == "--create" || mainCommand == "--c"  )
    {
        if (argc < 4)
        {
            std::cerr << "Arguments manquants" << std::endl;
            return 1;
        }
        
        if (argc > 4)
        {
            std::cerr << "Trop d'arguments " << std::endl;
            return 1;
        }
        
        std::string archivePath = argv[2];
        std::string archiveName = argv[3];
        
        Archive::Packer packer;
        
        if (Error e = packer.Create(archivePath, archiveName))
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
    }
    else if (mainCommand == "--pack" || mainCommand == "--p")
    {
        if (argc < 4)
        {
            std::cerr << "Arguments manquants" << std::endl;
            return 1;
        }
        
        std::string archivePath = argv[2];
        std::string elementPath = argv[3];
        std::string elementName = argv[4];
            
        Archive::Packer packer;
            
        Error e = packer.Open(archivePath);
            
        if (e)
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
            
        e = packer.Add(elementPath, elementName, Archive::FileFlags::None);
    }
    else if (mainCommand == "--unpack" || mainCommand == "--u")
    {
        if (argc < 4)
        {
            std::cerr << "Arguments manquants \n";
            return 1;
        }
        
        if (argc > 4)
        {
            std::cerr << "Trop d'arguments \n";
            return 1;
        }
        
        std::string archivePath = argv[2];
        std::string unpackLocation = argv[3];
        
        Archive::Packer packer;
        
        Error e = packer.Open(archivePath);
        
        if (e)
        {
            std::cout << e.Format() << "\n";
            return e.Exit();
        }
 
        e = packer.Unpack(unpackLocation);
    }
    else
    {
        std::cerr << "Commande : " << mainCommand <<" non reconnue \n";
    }
    
    return 0;
}
