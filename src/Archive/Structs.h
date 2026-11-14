#ifndef BABARCHIVE_ARCHIVE_STRUCTS_H_DEFINED
#define BABARCHIVE_ARCHIVE_STRUCTS_H_DEFINED

#include <cstdint>
#include <string>

namespace Archive
{
    enum FileType : uint8_t
    {
        Raw, Binary, Text
    };
    
    enum FileFlags : uint8_t
    {
        None = 0,
        Compressed = 0b00000001,
        Encrypted = 0b00000010,
    };
    
    struct BaseHeader
    {
        uint32_t magicNumber = 0xBABA;
        uint32_t version = 1;
    
        uint32_t archiveSize;
        uint8_t descriptorListOffset;
    };

    struct FileDescriptor
    {
        uint32_t nameLength;
        const char* name;
        uint32_t offset;
    };
    
    struct FileHeader
    {
        uint32_t uncompressedSize;
        uint32_t compressedSize;
        uint8_t fileType;
        
        uint16_t extensionLength;
        std::string extension;
        
        uint8_t flags;
        uint32_t crc32;
    };
    
}
    
#endif
