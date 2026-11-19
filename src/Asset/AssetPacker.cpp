#include "AssetPacker.h"

#include "AssetTypes.h"

Error Asset::Pack(Archive::Packer& _packer, std::string const& _src, std::string const& _filename, uint8_t _flags)
{
    std::string ext = FileHelper::ExtLower(_src);
    
    if (ext == ".txt") // Scene files are stored in plain txt files for now
        return _packer.Add(_src, _filename, _flags, static_cast<uint8_t>(AssetType::Scene));
    
    std::vector<char> res;
    AssetType type;
    Error e;
    
    if (ext == ".bmp")
    {
        type = AssetType::Image;
        e = ImportBmp(_src, res);
    }
    else if (ext == ".obj")
    {
        type = AssetType::Mesh;
        e = ImportObj(_src, res);
    }
    else if (ext == ".wav")
    {
        type = AssetType::Sound;
        e = ImportWav(_src, res);
    }
    else 
        return Error::Io("unsupported asset extension '" + ext + "'", _src);
    
    if (e)
        return e;
    
    return _packer.Add(res.data(), res.size(), _filename, _flags, static_cast<uint8_t>(type));
}

Error Asset::ImportObj(fs::path const& _src, std::vector<char>& _out)
{
    return Error::Ok();
}

Error Asset::ImportWav(fs::path const& _src, std::vector<char>& _out)
{
    return Error::Ok();
}
