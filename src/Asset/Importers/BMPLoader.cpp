#include "../AssetPacker.h"
#include "Asset/AssetTypes.h"

namespace
{
#pragma pack(push, 1)
    struct BmpHeader
    {
        uint16_t type;
        uint32_t size;
        uint16_t reserved1;
        uint16_t reserved2;
        uint32_t dataOffset;
    };
    
    struct BmpInfo
    {
        uint32_t size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bitPerPixel;
        uint32_t compression;
        uint32_t imageSize;
        int32_t xPixPerMeter;
        int32_t yPixPerMeter;
        uint32_t colorsUsed;
        uint32_t colorsImportant;
    };
#pragma pack(pop)
}

Error Asset::ImportBmp(fs::path const& _src, std::vector<char>& _out)
{
    std::ifstream in(_src, std::ios::binary);
    if (!in)
        return Error::Io("could not open file", _src);
    
    uint64_t size = fs::file_size(_src);
    if (size < sizeof(BmpHeader) + sizeof(BmpInfo))
        return Error::Io("bmp is too small", _src);
    
    BmpHeader header;
    BmpInfo info;
    
    in.read((char*)&header, sizeof(BmpHeader));
    in.read((char*)&info, sizeof(BmpInfo));
    
    if (header.type != 0x4D42) // "BM"
        return Error::Io("not a BMP file", _src);
    if (info.bitPerPixel != 24 && info.bitPerPixel != 32)
        return Error::Io("only 24/32 bit bmp supported", _src);
    if (info.compression != 0)
        return Error::Io("compression not supported", _src);
    
    bool bottomUp = info.height > 0;
    uint64_t width = static_cast<uint64_t>(info.width);
    uint64_t height = static_cast<uint64_t>(bottomUp ? info.height : -info.height);
    uint16_t bpp = info.bitPerPixel / 8;
    uint64_t rowSize = ((width * bpp + 3) / 4) * 4; // rows are padded to 4 bytes // TODO use bit operations 
    uint64_t dataSize = rowSize * height;
    
    if (header.dataOffset > size || dataSize > size - header.dataOffset)
        return Error::Io("file too small for pixel data", _src);
    if (width <= 0 || height <= 0)
        return Error::Io("invalid bmp dimensions", _src);
    
    _out.clear();
    ImageHeader iH;
    iH.width = width;
    iH.height = height;
    Append(_out, iH);
    uint64_t pixelStart = sizeof(ImageHeader);
    _out.resize(pixelStart + (size_t)width * height * 4);
    
    std::vector<uint8_t> row(rowSize);
  
    bool anyAlpha = false;
    
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t srcRow = bottomUp ? height - 1 - y : y;
        
        in.seekg(header.dataOffset + uint64_t(srcRow) * rowSize);
        in.read(reinterpret_cast<char*>(row.data()), rowSize);
        if (!in)
            return Error::Io("failed to read pixel row", _src);
        
        uint8_t* dst = reinterpret_cast<uint8_t*>(_out.data() + pixelStart + size_t(y) * width * 4);
        for (uint32_t x = 0; x < width; ++x)
        {
            uint8_t const* src = row.data() + size_t(x) * bpp;
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = bpp == 4 ? src[3] : 255;
            if (bpp == 4 && dst[3] != 0)
                anyAlpha = true;
            dst += 4;
        }
    }
    
    if (bpp == 4 && anyAlpha)
    {
        uint8_t* p = reinterpret_cast<uint8_t*>(_out.data() + pixelStart);
        for (uint64_t i = 0; i < (size_t)width * height; ++i)
            p[i * 4 + 3] = 255;
    }
    
    return Error::Ok();
}
