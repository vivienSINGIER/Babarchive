#include "Console.h"

#include <iostream>

#include "Commands.h"

namespace
{
    Console::Command const* FindCommand(std::string const& _name)
    {
        for (Console::Command const& command : Console::GetCommands())
        {
            if (command.longName == _name || command.shortName == _name)
                return &command;
        }
        return nullptr;
    }

    void PrintUsage(Console::Command const& _command, std::ostream& _out)
    {
        _out << "  " << _command.longName << " / " << _command.shortName << " " << _command.usage << "\n";
    }
}

std::vector<Console::Command> const& Console::GetCommands()
{
    static std::vector<Command> const s_commands =
    {
        { "--help",   "--h",  "(commande)",
          "affiche l'aide de toutes les commandes, ou d'une seule",
          0, 1, &Commands::Help },
        { "--create", "--c",  "<dossier> <nom>",
          "cree l'archive vide <dossier>/<nom>.babar",
          2, 2, &Commands::Create },
        { "--pack",   "--p",  "<archive> <fichier | dossier> <nom> (compresse y/n) (cle)",
          "ajoute un fichier ou un dossier sous le nom <nom> (sans extension), compresse si y, chiffre si une cle est donnee",
          3, 5, &Commands::Pack },
        { "--unpack", "--u",  "<archive> <dossier>",
          "extrait toute l'archive dans <dossier>, demande la cle de chaque fichier chiffre",
          2, 2, &Commands::Unpack },
        { "--remove", "--r",  "<archive> <nom>",
          "retire le fichier <nom> de l'archive",
          2, 2, &Commands::Remove },
        { "--list",   "--l",  "<archive>",
          "liste les fichiers de l'archive",
          1, 1, &Commands::List },
        { "--read",   "--rd", "<archive> <nom> (fichier de sortie)",
          "lit le fichier <nom> vers un fichier, ou vers la sortie standard",
          2, 3, &Commands::Read },
        { "--hex",    "--x",  "<fichier> (octets par ligne, 16 par defaut)",
          "affiche un fichier (une archive par exemple) en hexadecimal",
          1, 2, &Commands::Hex },
    };
    return s_commands;
}

int Console::Run(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Erreur : arguments manquants. Commande d'aide : --help / --h\n";
        return 1;
    }

    std::string const name = argv[1];
    Command const* command = FindCommand(name);
    if (command == nullptr)
    {
        std::cerr << "Commande '" << name << "' non reconnue. Commande d'aide : --help / --h\n";
        return 1;
    }

    Args args(argv + 2, argv + argc);

    if (args.size() < command->minArgs || args.size() > command->maxArgs)
    {
        std::cerr << (args.size() < command->minArgs ? "Arguments manquants" : "Trop d'arguments") << ", usage :\n";
        PrintUsage(*command, std::cerr);
        return 1;
    }

    return command->handler(args);
}

int Console::Commands::Help(Args const& _args)
{
    if (_args.size() == 1)
    {
        Command const* command = FindCommand(_args[0]);
        if (command == nullptr)
        {
            std::cerr << "Commande '" << _args[0] << "' non reconnue\n";
            return 1;
        }

        PrintUsage(*command, std::cout);
        std::cout << "    " << command->description << "\n";
        return 0;
    }

    std::cout << "Babarchive - commandes :\n";
    std::cout << "<arg> = obligatoire, (arg) = optionnel\n\n";

    for (Command const& command : GetCommands())
    {
        PrintUsage(command, std::cout);
        std::cout << "    " << command.description << "\n";
    }

    std::cout << "\nLes chemins relatifs partent du dossier du projet (celui du CMakeLists.txt).\n";
    std::cout << "\nExemples :\n";
    std::cout << "  --create Test archive\n";
    std::cout << "  --pack Test/archive.babar Test/test.txt test y\n";
    std::cout << "  --pack Test/archive.babar Test/babar.jpg image n maCle\n";
    std::cout << "  --list Test/archive.babar\n";
    std::cout << "  --read Test/archive.babar test Test/test_copie.txt\n";
    std::cout << "  --unpack Test/archive.babar Test/Unpack\n";
    std::cout << "  --remove Test/archive.babar test\n";
    std::cout << "  --hex Test/archive.babar\n";

    return 0;
}
