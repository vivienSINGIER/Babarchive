#ifndef BABARCHIVE_ARCHIVE_STRUCTS_H_DEFINED
#define BABARCHIVE_ARCHIVE_STRUCTS_H_DEFINED

#include <cstdint>
#include <string>

namespace Archive
{
    static constexpr uint32_t g_magicNumber = ('B' << 0) | ('A' << 8) | ('B' << 16) | ('A' << 24);
    static constexpr uint32_t g_currentVersion = 1;
    
    enum FileType : uint8_t
    {
        Raw, Binary, Text, UserStart
    };
    
    enum FileFlags : uint8_t
    {
        None = 0,
        Compressed = 0b00000001,
        Encrypted = 0b00000010,
    };
    
    struct BaseHeader
    {
        uint32_t magicNumber = g_magicNumber;
        uint32_t version = g_currentVersion;
    
        uint64_t dataSize;
        uint64_t descriptorListOffset;
    };

    struct FileDescriptor
    {
        uint32_t nameLength;
        std::string name;
        uint64_t offset;
        
        uint64_t DataSize() const
        {
            return sizeof(nameLength) + sizeof(offset) + sizeof(char) * nameLength;
        }
    };
    
#pragma pack(push, 1)
    struct FileHeaderFixed
    {
        uint64_t uncompressedSize;
        uint64_t compressedSize;
        
        uint8_t fileType;
        uint8_t flags;
        
        uint32_t crc32;
        
        uint16_t extensionLength;
    };
#pragma pack(pop)
    
    struct FileHeader
    {
        FileHeaderFixed fixed;
        std::string extension;
        
        uint32_t DataSize() const
        {
            return sizeof(FileHeaderFixed) + sizeof(char) * fixed.extensionLength;
        }
    };
    
    struct FileInfo
    {
        std::string name;
        uint64_t offset;
        
        uint64_t currentSize;
        uint64_t dataSize;
        
        bool isCompressed = false;
        bool isEncrypted = false;
        
        std::string extension;
    };
    
}
    
#endif
