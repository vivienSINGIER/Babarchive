#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include "Core/Error.h"

class HexViewer
{
public:
    Error Init(std::string const& _path);

    // Prints the whole file, _lineOffset bytes per line
    void Display(uint32_t _lineOffset);

private:
    std::fstream file;

    uint64_t cursor = 0;
    uint32_t offset = 0;

    std::string GetCurrentOffset() const;
    std::string GetHexLine(std::string const& _line);
    std::string GetAsciiLine(std::string const& _line);
};
