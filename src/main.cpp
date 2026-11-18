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
        std::cout << "\n";
        std::cout << "<arg> = necessary (arg) = optional " << "\n";
        std::cout << "--create / --c <creation directory> <archive name>\n";
        std::cout << "--pack / --p <archive path> <file path | directory path> <name> <compressed -> y/n> (encryption key)  <>\n";
        std::cout << "--unpack / --u <archive path> <unpack directory>\n";
        std::cout << "--remove / --r <archive path> <name>\n";
        std::cout << "\n";
        std::cout << "Exemples : \n";
        std::cout << "\n";
        std::cout << "--create / --c project/archives archive1\n";
        std::cout << "--pack / --p project/archives/archive1.babar project/file1.txt file1 testKey y <>\n";
        std::cout << "--unpack / --u project/archives/archive1.babar project\n";
        std::cout << "--remove / --r project/archives/archive1.babar file1\n";
    }
    else if (mainCommand == "--create" || mainCommand == "--c"  )
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
            std::cerr << "Arguments manquants \n";
            return 1;
        }
        Archive::Packer packer;
        
        Error e = packer.Open(argv[2]);
        if (e)
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
        
        std::string elementPath = argv[3];
        std::string elementName = argv[4];
        
        // optionals
        std::string compressed;
        std::string encryptionKey;
        
        uint8_t flag = Archive::FileFlags::None;
        
        if (argc > 5)
            compressed = argv[5];
            
        if (argc > 6)
            encryptionKey = argv[6];
            
        if (compressed == "y")
        {
            flag = Archive::FileFlags::Encrypted;
            if (encryptionKey.empty() == false)
                flag |= Archive::FileFlags::Encrypted;
        }
        else if (compressed == "n")
        {
            if (encryptionKey.empty() == false)
                flag |= Archive::FileFlags::Encrypted;
        }
        else
        {
            std::cerr << "Argument de compression invalide \n";
            return 1;
        }
        
        e = packer.Add(elementPath, elementName, flag, Archive::FileType::Binary , encryptionKey);
        
        if (e)
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
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
        
        Archive::Packer packer;
        
        Error e = packer.Open(argv[2]);
        
        if (e)
        {
            std::cout << e.Format() << "\n";
            return e.Exit();
        }
        
        std::string unpackLocation = argv[3];
        
        std::unordered_map<std::string, std::string> keymap = {};
        
        std::vector<Archive::FileInfo> files;
        e = packer.ListInfo(files);
        
        for (auto& file : files)
        {
            if (file.isEncrypted)
            {
                std::string encryptionKey; 
                std::cout << file.name << " is encrypted. Input key : ";
                std::cin >> encryptionKey;
                
                keymap[file.name] = encryptionKey;
            }
        }
        
        e = packer.Unpack(unpackLocation, keymap);
        
        if (e)
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
    }
    else if (mainCommand == "--remove" || mainCommand == "--r")
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
        
        Archive::Packer packer;
        
        Error e = packer.Open(argv[2]);
        if (e)
        {
            std::cerr << e.Format() << "\n";
            return e.Exit();
        }
        
        std::string fileName = argv[3];
        
        
    }
    else
    {
        std::cerr << "Commande : " << mainCommand <<" non reconnue \n";
    }
    
    return 0;
}
