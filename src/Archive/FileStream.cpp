#include "FileStream.h"

#include "Encryption.hpp"
#include <algorithm>

Error Archive::FileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    m_file.open(_path, std::ios::in | std::ios::binary);

    if (!m_file.is_open())
        return Error::Io("Failed to open file", _path);

    m_startOffset = _startOffset;
    m_count = _count;
    m_cursorOffset = 0;
    m_readCount = 0;
    m_readTotal = 0;
    m_decrypt = false;
    
    m_file.seekg(_startOffset);
    if (!m_file)
        return Error::Io("Failed to seek to file data", _path);
    
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
    m_file.clear();
    m_file.seekg(m_startOffset + m_cursorOffset);
}

void Archive::FileStream::SetDecryptionKey(std::string const& _key)
{
    m_decrypt = true;
    m_hashKeys = Encryption::GetHashes(_key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Error Archive::UncompressedFileStream::Read(char* _data, uint64_t _count)
{
    m_readCount = 0;

    if (IsAtEnd())
        return Error::Io("Failed to read file, cursor out of bounds");
    
    uint64_t toRead = std::min<uint64_t>(_count, m_count - m_cursorOffset);
    m_file.read(_data, (std::streamsize)toRead);
    
    if (m_decrypt)
        Encryption::Decrypt(m_hashKeys, _data, _count, m_cursorOffset);
    
    m_readCount = m_file.gcount();
    m_readTotal += m_readCount;
    m_cursorOffset += m_readCount;

    if (m_file.bad() || m_readCount != toRead)
        return Error::Io("Failed to read file, unexpected end of archive");

    return Error::Ok();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

Archive::CompressedFileStream::~CompressedFileStream()
{
    if (m_initialized)
        inflateEnd(&m_stream);
}

Error Archive::CompressedFileStream::Open(fs::path const& _path, uint64_t _startOffset, uint64_t _count)
{
    if (m_initialized)
        inflateEnd(&m_stream);

    m_stream = {};
    m_compressedLeft = _count;
    m_finished = false;

    if (inflateInit(&m_stream) != Z_OK)
        return Error::Io("couldn't initialize zlib inflate stream", _path);
    m_initialized = true;

    return FileStream::Open(_path, _startOffset, _count);
}

Error Archive::CompressedFileStream::Read(char* _data, uint64_t _count)
{
    m_readCount = 0;

    if (!m_initialized)
        return Error::Io("Compressed stream is not open");
    if (m_finished)
        return Error::Io("Failed to read file, cursor out of bounds");

    m_stream.next_out = reinterpret_cast<Bytef*>(_data);
    m_stream.avail_out = (uInt)std::min<uint64_t>(_count, UINT32_MAX);
    uInt const requested = m_stream.avail_out;

    while (m_stream.avail_out > 0 && !m_finished)
    {
        if (m_stream.avail_in == 0)
        {
            uint64_t toRead = std::min<uint64_t>(m_compressedLeft, STREAM_BUFFER_SIZE);
            if (toRead == 0)
                return Error::Io("Compressed data truncated");

            m_file.read(m_in, (std::streamsize)toRead);
            uint64_t got = (uint64_t)m_file.gcount();
            if (got == 0)
                return Error::Io("Failed to read compressed data");

            m_compressedLeft -= got;
            m_stream.next_in = reinterpret_cast<Bytef*>(m_in);
            m_stream.avail_in = (uInt)got;
        }

        int ret = inflate(&m_stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_END)
            m_finished = true;
        else if (ret != Z_OK)
            return Error::Io("Corrupted compressed data");
    }

    m_readCount = requested - m_stream.avail_out;
    m_readTotal += m_readCount;

    if (m_decrypt)
        Encryption::Decrypt(m_hashKeys, _data, m_readCount, m_cursorOffset);

    m_cursorOffset += m_readCount; // position in the decompressed data

    return Error::Ok();
}

bool Archive::CompressedFileStream::IsAtEnd()
{
    return m_finished;
}

bool Archive::CompressedFileStream::IsFinished()
{
    return m_finished;
}
