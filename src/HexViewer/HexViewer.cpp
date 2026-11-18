#include "HexViewer.h"

#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

Error HexViewer::Init(std::string const& _path)
{
    file = std::fstream(_path, std::ios::in | std::ios::binary);
    cursor = 0;

    if (!file)
        return Error::Io("couldn't open file", _path);

    return Error::Ok();
}

void HexViewer::Display(uint32_t _lineOffset)
{
    offset = _lineOffset;

    std::string buffer;
    buffer.resize(_lineOffset);

    while (true)
    {
        file.read(buffer.data(), buffer.size());
        uint32_t count = (uint32_t)file.gcount();

        // Nothing left: don't print an empty line when the size is a multiple of the width
        if (count == 0)
            break;

        std::string line = buffer.substr(0, count);

        // Pad the hex column of the last line so the ascii column stays aligned
        std::string hex = GetHexLine(line) + std::string((_lineOffset - count) * 3, ' ');

        std::cout << GetCurrentOffset() << "\033[32m | \033[0m" << hex << "\033[32m | \033[0m" << GetAsciiLine(line) << "\n";

        cursor += count;

        if (count < buffer.size())
            break;
    }
    std::cout.flush();
}

std::string HexViewer::GetCurrentOffset() const
{
    std::stringstream stream;
    stream << "\033[38;2;255;165;0m";

    // cursor already counts bytes
    stream << std::setfill ('0') << std::setw(8)
       << std::hex << cursor;
    stream << "\033[0m";
    std::string result( stream.str());
    return result;
}

std::string HexViewer::GetHexLine(std::string const& _line)
{
    std::stringstream ss;
    for(int i = 0; i < _line.length(); ++i)
    {
        if (i % 2 == 0)
            ss << "\033[34m";
        
        int t = (uint8_t)_line.c_str()[i];
        ss << std::setfill ('0') << std::setw(2) << std::hex << std::uppercase << t << " ";
        
        if (i % 2 == 0)
            ss << "\033[0m";
    }
    std::string mystr = ss.str();
    return mystr;
}

std::string HexViewer::GetAsciiLine(std::string const& _line)
{
    std::stringstream ss;
    for (int i = 0; i < _line.length(); ++i)
    {
        char v = _line[i];
        if (v < 31 || v >= 127)
            ss << '.';
        else
            ss << v;
    }
    return ss.str();
}


