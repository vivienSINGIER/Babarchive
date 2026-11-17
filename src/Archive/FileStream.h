#ifndef BABARCHIVE_ARCHIVE_FILESTREAM_H_DEFINED
#define BABARCHIVE_ARCHIVE_FILESTREAM_H_DEFINED

#include <array>
#include <fstream>
#include <iosfwd>

#include "Structs.h"
#include "zlib.h"
#include "../Core/Error.h"

#define STREAM_BUFFER_SIZE 4096

namespace Archive
{
    
class FileStream
{
public:
    FileStream() = default;
    virtual ~FileStream() = default;
    
    virtual Error Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count);
    virtual Error Read(const char* _data, uint64_t count);
    virtual bool IsAtEnd();
    virtual bool IsFinished();
    
    uint64_t CursorOffset() const;
    uint64_t ReadCount() const;
    
    void SetCursorOffset(uint64_t _cursorOffset);
    void SetDecryptionKey(std::string const& _key);
    
protected:
    std::ifstream m_file;
    uint64_t m_startOffset;
    uint64_t m_count;
    
    uint64_t m_cursorOffset;
    uint64_t m_readCount;
    uint64_t m_readTotal;
    
    std::array<std::string, 3> m_hashKeys;
    bool m_decrypt;
};
    
class UncompressedFileStream : public FileStream
{
public:
    Error Read(char* _data, uint64_t count);
};
    
class CompressedFileStream : public FileStream
{
public:
    CompressedFileStream() = default;
    ~CompressedFileStream();
    
    Error Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count) override;
    Error Read(char* _data, uint64_t _count);
    bool IsFinished() override;
private:
    z_stream m_stream;
    char m_in[STREAM_BUFFER_SIZE];
    uint64_t m_compressedLeft;
    bool m_finished;
};
    
}

#endif
