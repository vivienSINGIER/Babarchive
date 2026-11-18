#ifndef BABARCHIVE_CONSOLE_CONSOLE_H_DEFINED
#define BABARCHIVE_CONSOLE_CONSOLE_H_DEFINED

#include <cstddef>
#include <string>
#include <vector>

// Command line interface of Babarchive.
// Only depends on the public API of the Archive library, the library knows nothing about it.
namespace Console
{
    // Arguments of a command, without the program name and the command itself
    using Args = std::vector<std::string>;
    using Handler = int (*)(Args const& _args);

    struct Command
    {
        std::string longName;     // --create
        std::string shortName;    // --c
        std::string usage;        // <directory> <name>
        std::string description;
        size_t minArgs;
        size_t maxArgs;
        Handler handler;
    };

    std::vector<Command> const& GetCommands();

    // Parses argv, checks the argument count and runs the matching command. Returns the process exit code.
    int Run(int argc, char* argv[]);
}

#endif
