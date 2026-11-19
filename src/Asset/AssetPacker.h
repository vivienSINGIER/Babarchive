#ifndef BABARCHIVE_ASSET_ASSETPACKER_H_DEFINED
#define BABARCHIVE_ASSET_ASSETPACKER_H_DEFINED
#include <string>

#include "Archive/Packer.h"
#include "Core/Error.h"

namespace Asset
{
    Error Pack(Archive::Packer& _packer, std::string const& _src, std::string const& _filename, uint8_t _flags = 0);
    
    Error ImportBmp(fs::path const& _src, std::vector<char>& _out);
    Error ImportObj(fs::path const& _src, std::vector<char>& _out);
    // Error ImportWav(fs::path const& _src, std::vector<char>& _out);
}

#endif
