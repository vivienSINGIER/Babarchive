#ifndef BABARCHIVE_CONSOLE_COMMANDS_H_DEFINED
#define BABARCHIVE_CONSOLE_COMMANDS_H_DEFINED

#include "Console.h"

namespace Console::Commands
{
    int Help(Args const& _args);
    int Create(Args const& _args);
    int Pack(Args const& _args);
    int Unpack(Args const& _args);
    int Remove(Args const& _args);
    int List(Args const& _args);
    int Read(Args const& _args);
    int Hex(Args const& _args);
}

#endif
