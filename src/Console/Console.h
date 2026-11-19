#ifndef BABARCHIVE_CONSOLE_CONSOLE_H_DEFINED
#define BABARCHIVE_CONSOLE_CONSOLE_H_DEFINED

#include <cstddef>
#include <string>
#include <vector>

namespace Console
{
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

    int Run(int argc, char* argv[]);
}

#endif
