#pragma once

#include <fstream>
#include <string>

class HexViewer
{
public:
    void Init(std::string const& _path);
    
    void Display(uint32_t _lineOffset);
    
private:
    std::fstream file;
    
    uint32_t cursor = 0;
    uint32_t offset = 0;
    
    std::string GetCurrentOffset() const;
    std::string GetHexLine(std::string const& _line);
    std::string GetAsciiLine(std::string const& _line);
};
