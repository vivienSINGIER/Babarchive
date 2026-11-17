#ifndef BABARCHIVE_ARCHIVE_FILESTREAM_H_DEFINED
#define BABARCHIVE_ARCHIVE_FILESTREAM_H_DEFINED

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
    // Reads up to _count bytes of (decompressed) file data into _data, ReadCount() gives how many were read
    virtual Error Read(char* _data, uint64_t _count) = 0;
    virtual bool IsAtEnd();
    virtual bool IsFinished();
    
    uint64_t CursorOffset() const;
    uint64_t ReadCount() const;
    
    void SetCursorOffset(uint64_t _cursorOffset);
    
protected:
    std::ifstream m_file;
    uint64_t m_startOffset = 0;
    uint64_t m_count = 0;

    uint64_t m_cursorOffset = 0;
    uint64_t m_readCount = 0;
    uint64_t m_readTotal = 0;
};

class UncompressedFileStream : public FileStream
{
public:
    Error Read(char* _data, uint64_t _count) override;
};

class CompressedFileStream : public FileStream
{
public:
    CompressedFileStream() = default;
    ~CompressedFileStream() override;

    Error Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count) override;
    Error Read(char* _data, uint64_t _count) override;
    bool IsAtEnd() override;
    bool IsFinished() override;
private:
    z_stream m_stream{};
    bool m_initialized = false;
    char m_in[STREAM_BUFFER_SIZE];
    uint64_t m_compressedLeft = 0;
    bool m_finished = false;
};
    
}

#endif
