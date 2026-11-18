#ifndef BABARCHIVE_ARCHIVE_PACKER_H_DEFINED
#define BABARCHIVE_ARCHIVE_PACKER_H_DEFINED

#include <unordered_map>

#include "FileStream.h"
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
        
        Error Add(fs::path _file, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary, std::string const& _key = "");
        Error Add(char* _data, uint64_t _count, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary, std::string const& _key = "");
        
        Error Remove(std::string_view _name);
        
        Error Unpack(fs::path _unpackLocation, std::unordered_map<std::string, std::string> const& _keyMap = {});
        
        Error ReadFile(std::string const& _name, FileHeader& _header, std::unique_ptr<FileStream>& _stream, std::string const& _key = "");
        
        Error ListInfo(std::vector<FileInfo>& _files);
        
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
        
        // Writes one file entry (header + data) at the current position and registers its descriptor
        Error WriteEntry(std::ofstream& _out, std::istream& _in, std::string const& _name, std::string const& _extension, uint8_t _flags, uint8_t _type, std::string const& _key);
        // Writes the descriptor list after the last entry and updates the base header
        Error WriteIndex(std::ofstream& _out);

        Error WriteFile(std::ofstream& _out, std::istream& _in, FileHeader& _fHeader, std::string const& _key);
        Error WriteFileCompressed(std::ofstream& _out, std::istream& _in, FileHeader& _fHeader, std::string const& _key);
        
        Error ReadHeader(std::ifstream& _in, BaseHeader& _header);
        Error ReadDescriptors(std::ifstream& _in, uint64_t _offset, std::vector<FileDescriptor>& _out);
        Error ReadFileHeader(std::ifstream& _in, uint64_t _offset, FileHeader& _header);
        Error ExtractFile(std::ifstream& _in, std::ofstream& _out, FileHeader const& _fHeader, std::string const& _key = "");
        
        // Helpers
        uint64_t GetFileSize();
        bool CheckDuplicates(std::vector<FileDescriptor> const& _desc, std::string const& _newName);
    };
}

#endif

