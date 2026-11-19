#ifndef BABARCHIVE_ASSET_ASSETHANDLER_H_DEFINED
#define BABARCHIVE_ASSET_ASSETHANDLER_H_DEFINED
#include <string>

#include "AssetTypes.h"
#include "Archive/Packer.h"
#include "Core/Error.h"

namespace Asset
{
    class AssetHandler
    {
    public:
        Error Load(std::string const& _archive, std::string const& _filename, std::vector<char>& _arena, Handle& _out, std::string const& _key = "");
        
    private:
        Error GetPacker(std::string const& _archive, Archive::Packer*& _out);
        
        std::unordered_map<std::string, Archive::Packer> m_archive; // cached archives to avoid reopening them 
    };   
}

#endif
