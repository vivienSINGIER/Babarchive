#include "FileStream.h"

Error Archive::FileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    m_file.open(_path.string());
    
    if (!m_file.is_open())
        return Error::Io("Failed to open file", _path);
    
    m_startOffset = _startOffset;
    m_count = _count;
    m_cursorOffset = 0;
    m_readCount = 0;
    
    m_file.seekg(_startOffset);
    
    return Error::Ok();
}

uint64_t Archive::FileStream::CursorOffset() const
{
    return m_cursorOffset;
}

uint64_t Archive::FileStream::ReadCount() const
{
    return m_readCount;
}

void Archive::FileStream::SetCursorOffset(uint64_t _cursorOffset)
{
    m_cursorOffset = _cursorOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Error Archive::UncompressedFileStream::Read(char* _data, uint64_t count)
{
    m_file.read(_data, count);
    
    m_readCount = m_file.gcount();
    m_cursorOffset = m_file.tellg();
    
    if (m_file.bad())
        return Error::Io("Failed to read file");
    
    return Error::Ok();
}

Archive::CompressedFileStream::~CompressedFileStream()
{
    inflateEnd(&m_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
Error Archive::CompressedFileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;
    
    inflateInit(&m_stream);
    
    return FileStream::Open(_path, _startOffset, _count);
}

Error Archive::CompressedFileStream::Read(char* _data, uint64_t count)
{
    uint64_t remaining = count;
    
    std::st out();
    
    while (true)
    {
        uint64_t toRead = std::min<uint64_t>(remaining, STREAM_BUFFER_SIZE);
        m_file.read(m_in, STREAM_BUFFER_SIZE);
        remaining -= m_file.gcount();
        
        m_stream.avail_in = m_file.gcount();
        m_stream.next_in = (Bytef*)m_in;
        m_stream.avail_out = STREAM_BUFFER_SIZE;
        m_stream.next_out = (Bytef*)m_out;
        
        int ret = inflate(&m_stream, Z_NO_FLUSH);
        int produced = STREAM_BUFFER_SIZE - m_stream.avail_out;
        
        
        
        if (ret == Z_STREAM_ERROR)
            return Error::Io("Failed to decompress chunk");
        
        if (remaining > 0 || ret == Z_STREAM_END)
            break;
        
    }
}


