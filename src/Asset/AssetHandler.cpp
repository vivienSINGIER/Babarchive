#include "AssetHandler.h"

Error Asset::AssetHandler::Load(std::string const& _archive, std::string const& _filename, std::vector<char>& _arena,
    Handle& _out, std::string const& _key)
{
    Archive::Packer* packer = nullptr;
    if (Error e = GetPacker(_archive, packer))
        return e;

    Archive::FileHeader fileHeader;
    std::unique_ptr<Archive::FileStream> stream;
    if (Error e = packer->ReadFile(_filename, fileHeader, stream, _key))
        return e;
    
    uint64_t offset = ((_arena.size() + 15) / 16) * 16; // round size to next multiple of 16, keep addresses aligned
    uint64_t size = fileHeader.fixed.uncompressedSize;
    _arena.resize(offset + size);
    
    uint64_t written = 0;
    while (written < size)
    {
        Error e = stream->Read(_arena.data() + offset + written, size - written);
        if (!e && stream->ReadCount() == 0)
            e = Error::Io("unexpected end of '" + _filename + "'", _archive);
        
        if (e)
        {
            _arena.resize(offset); // erasee partly written file
            return e;
        }
        written += stream->ReadCount();
    }
    
    _out = { offset, size, static_cast<AssetType>(fileHeader.fixed.fileType) };
    return Error::Ok();
}

Error Asset::AssetHandler::GetPacker(std::string const& _archive, Archive::Packer*& _out)
{
    auto it = m_archive.find(_archive);
    if (it == m_archive.end())
    {
        Archive::Packer packer;
        if (Error e = packer.Open(_archive))
            return e;
        it = m_archive.emplace(_archive, std::move(packer)).first;
    }
    _out = &it->second;
    return Error::Ok();
}
