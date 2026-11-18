#include <filesystem>

#include "Console/Console.h"

int main(int argc, char* argv[])
{
#ifdef BABARCHIVE_WORKING_DIR
    std::filesystem::current_path(BABARCHIVE_WORKING_DIR);
#endif

    return Console::Run(argc, argv);
}
