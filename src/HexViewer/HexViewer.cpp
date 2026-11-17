#include "HexViewer.h"

#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

void HexViewer::Init(std::string const& _path)
{
    file = std::fstream(_path, std::ios::in | std::ios::binary);
    
    if (!file)
    {
        std::cout << "Error opening file " << _path << std::endl;
    }
}

void HexViewer::Display(uint32_t _lineOffset)
{
    offset = _lineOffset;
    
    std::string buffer;
    buffer.resize(_lineOffset);
    
    bool canRead = true;
    while (canRead)
    {
        file.read(buffer.data(), buffer.size());
        uint32_t count = file.gcount();
        
        if (count < buffer.size())
        {
            canRead = false;
            buffer.resize(count);
        }
        
        std::cout << GetCurrentOffset() << "\033[32m | \033[0m" << GetHexLine(buffer) << "\033[32m | \033[0m" << GetAsciiLine(buffer) << std::endl;
        
        cursor += count;
    }
}

std::string HexViewer::GetCurrentOffset() const
{
    std::stringstream stream;
    stream << "\033[38;2;255;165;0m";
    
    stream << std::setfill ('0') << std::setw(8) 
       << std::hex << cursor * offset;
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


