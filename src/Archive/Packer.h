#ifndef BABARCHIVE_ARCHIVE_PACKER_H_DEFINED
#define BABARCHIVE_ARCHIVE_PACKER_H_DEFINED

#include "Packer.h"
#include "Structs.h"

#include "../Core/FileHelper.h"
#include "../Core/Error.h"

namespace Archive
{
    class Packer
    {
    public:
        Packer();
        
        Error Create(fs::path _location, std::string _name);
        
        Error Add(fs::path _file, uint8_t _flags, std::string const& _name, FileType _type = FileType::Binary);
        Error Add(const char* _data, uint32_t _count);
        
        Error Remove(std::string_view _name);
        
        // Add Reads
        
        void SetChunkSize(uint32_t _chunkSize);
        uint32_t GetChunkSize() const;
    private:
        uint32_t m_chunkSize = 16384;
        fs::path m_location;
        bool m_isCreated;
        
        Error WriteChunk(std::ofstream& _out, unsigned char* _outBuff);
        Error WriteCompressedChunk(std::ofstream& _out, unsigned char* _outBuff);
        
        Error WriteHeader(std::ofstream& _out, BaseHeader const& _header);
        Error WriteDescriptor(std::ofstream& _out, FileDescriptor const& _desc);
        Error WriteFile(std::ofstream& _out, std::ifstream& _in, FileHeader& _fHeader);
        // Error WriteFileCompressed(std::ofstream& _out, std::ifstream& _in, FileHeader& _fHeader); Use zstream
        
        Error ReadHeader(std::ifstream& _in, BaseHeader& _header);
        Error ReadDescriptors(std::ifstream& _in, uint64_t _offset, std::vector<FileDescriptor>& _out);
        Error ReadFileHeader(std::ifstream& _in, uint64_t _offset, FileHeader& _header);
        
        // Helpers
        uint64_t GetFileSize();
        bool CheckDuplicates(std::vector<FileDescriptor> const& _desc, std::string const& _newName);
    };
}

#endif

