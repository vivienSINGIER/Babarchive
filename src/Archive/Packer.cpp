#ifndef BABARCHIVE_ARCHIVE_PACKER_H_DEFINED
#define BABARCHIVE_ARCHIVE_PACKER_H_DEFINED

#include "Packer.h"

#include "../Core/FileHelper.h"
#include "../Core/Error.h"

namespace Archive
{
    class Packer
    {
        Error Create(fs::path _location);
        
        Error Add(fs::path _file, uint8_t _flags);
        Error Add(const char* _data, uint32_t _count);
        
        Error Remove(std::string_view _name);
        
        // Add Reads
    };
}

#endif

