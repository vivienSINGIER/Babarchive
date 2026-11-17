#ifndef BABARCHIVE_ARCHIVE_PACKER_H_DEFINED
#define BABARCHIVE_ARCHIVE_PACKER_H_DEFINED

#include "Structs.h"

#include "../Core/FileHelper.h"
#include "../Core/Error.h"

namespace Archive
{
    class Packer
    {
    public:
        Packer();
        
        Error Open(fs::path _location);
        Error Create(fs::path _location, std::string _name);
        
        Error Add(fs::path _file, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary);
        Error Add(const char* _data, uint64_t _count, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary);
        
        // Error Remove(std::string_view _name);
        
        Error Unpack(fs::path _unpackLocation);
        
        // FileStream Read(std::string const& _name);
        
        // Add Reads
        
        void SetChunkSize(uint32_t _chunkSize);
        uint32_t GetChunkSize() const;
    private:
        uint32_t m_chunkSize = 16384;
        fs::path m_location;
        bool m_isOpen;
        
        BaseHeader m_header;
        std::vector<FileDescriptor> m_descriptors;
        
        Error WriteHeader(std::ofstream& _out, BaseHeader const& _header);
        Error WriteDescriptor(std::ofstream& _out, FileDescriptor const& _desc);
        
        Error WriteFile(std::ofstream& _out, std::ifstream& _in, FileHeader& _fHeader);
        Error WriteRawFile(std::ofstream& _out, const char* _data, uint32_t _count, FileHeader& _fHeader);
        
        // Error WriteFileCompressed(std::ofstream& _out, std::ifstream& _in, FileHeader& _fHeader); // Use zstream
        
        Error ReadHeader(std::ifstream& _in, BaseHeader& _header);
        Error ReadDescriptors(std::ifstream& _in, uint64_t _offset, std::vector<FileDescriptor>& _out);
        Error ReadFileHeader(std::ifstream& _in, uint64_t _offset, FileHeader& _header);
        Error ExtractFile(std::ifstream& _in, std::ofstream& _out, FileHeader const& _fHeader);
        
        // Helpers
        uint64_t GetFileSize();
        bool CheckDuplicates(std::vector<FileDescriptor> const& _desc, std::string const& _newName);
    };
}

#endif

