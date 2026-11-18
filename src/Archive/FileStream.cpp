#include "FileStream.h"

#include "Encryption.hpp"

Error Archive::FileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    m_file.open(_path.string());
    
    if (!m_file.is_open())
        return Error::Io("Failed to open file", _path);
    
    m_startOffset = _startOffset;
    m_count = _count;
    m_cursorOffset = 0;
    m_readCount = 0;
    m_readTotal = 0;
    m_decrypt = false;
    
    m_file.seekg(_startOffset);
    
    return Error::Ok();
}

Error Archive::FileStream::Read(const char* _data, uint64_t count)
{
    return Error::Ok();
}

bool Archive::FileStream::IsAtEnd()
{
    return m_cursorOffset >= m_count || m_file.eof();
}

bool Archive::FileStream::IsFinished()
{
    return m_file.eof() || m_readTotal >= m_count;
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
    m_file.seekg(m_startOffset + m_cursorOffset);
}

void Archive::FileStream::SetDecryptionKey(std::string const& _key)
{
    m_decrypt = true;
    m_hashKeys = Encryption::GetHashes(_key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Error Archive::UncompressedFileStream::Read(char* _data, uint64_t count)
{
    if (IsAtEnd())
        return Error::Io("Failed to read file, cursor out of bounds");
    
    m_file.read(_data, count);
    
    if (m_decrypt)
        Encryption::Decrypt(m_hashKeys, _data, count, m_cursorOffset);
    
    m_readCount = m_file.gcount();
    m_readTotal += m_readCount;
    m_cursorOffset = m_file.tellg();
    
    if (m_file.bad())
        return Error::Io("Failed to read file");
    
    return Error::Ok();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

Archive::CompressedFileStream::~CompressedFileStream()
{
    inflateEnd(&m_stream);
}

Error Archive::CompressedFileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;
    
    m_compressedLeft = _count;
    m_finished = false;
    
    inflateInit(&m_stream);
    
    return FileStream::Open(_path, _startOffset, _count);
}

Error Archive::CompressedFileStream::Read(char* _data, uint64_t _count)
{
    m_stream.next_out = reinterpret_cast<Bytef*>(_data);
    m_stream.avail_out = (uInt)_count;
    
    while (m_stream.avail_out > 0 && !m_finished)
    {
        if (m_stream.avail_in == 0)
        {
            uint64_t toRead = std::min<uint64_t>(m_compressedLeft, STREAM_BUFFER_SIZE);
            if (toRead == 0)
                return Error::Io("Compressed data truncated");
            
            m_file.read(m_in, toRead);
            uint64_t got = (uint64_t)m_file.gcount();
            if (got <= 0)
                return Error::Io("Failed to read compressed data");
            
            m_compressedLeft -= got;
            m_stream.next_in = reinterpret_cast<Bytef*>(m_in);
            m_stream.avail_in = (uInt)got;
        }
        
        int ret = inflate(&m_stream, Z_NO_FLUSH);
        
        if (ret == Z_STREAM_END)
            m_finished = true;
        if (ret == Z_BUF_ERROR)
            return Error::Io("Failed to read compressed data");
    }
    m_readCount = _count - m_stream.avail_out;
    if (m_decrypt)
        Encryption::Decrypt(m_hashKeys, _data, _count, m_cursorOffset);
    m_cursorOffset = m_file.tellg();
    return Error::Ok();
}

bool Archive::CompressedFileStream::IsFinished()
{
    return m_finished;
}


