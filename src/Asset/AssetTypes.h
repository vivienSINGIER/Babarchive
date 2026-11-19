#ifndef BABARCHIVE_ASSET_ASSETTYPES_H_DEFINED
#define BABARCHIVE_ASSET_ASSETTYPES_H_DEFINED

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../Archive/Structs.h"

namespace Asset
{
    // Stored in FileHeader.fileType
    enum class AssetType : uint8_t
    {
        Image = Archive::FileType::UserStart, Mesh, Sound, Scene
    };

    // Location of an asset inside its arena
    struct Handle
    {
        uint64_t offset = 0;
        uint64_t size = 0;
        AssetType type = {};

        bool IsValid() const { return size != 0; }
    };

    // Stored layouts: headers directly followed by thgeir payload

#define MAX_MIP_COUNT 10
    
    // payload: RGBA8 pixels[width * height], top-left -> bottom-right
    struct ImageHeader
    {
        static constexpr AssetType kType = AssetType::Image;
        uint32_t width;
        uint32_t height;
    };

    struct Vertex
    {
        float pos[3];
        float normal[3];
        float uv[2];
    };

    // payload: Vertex vertices[vertexCount] & uint32 indices[indexCount]
    struct MeshHeader
    {
        static constexpr AssetType kType = AssetType::Mesh;
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    // payload: float samples[frameCount * channelCount]
    struct SoundHeader
    {
        static constexpr AssetType kType = AssetType::Sound;
        uint32_t sampleRate;
        uint32_t channelCount;
        uint32_t frameCount;
    };

    // Views: pointers into the arena
    // they stay valid until the arena grows again
    // keep  handles
    struct ImageView
    {
        ImageHeader const* header = nullptr;
        uint8_t const* pixels = nullptr;
    };

    struct MeshView
    {
        MeshHeader const* header = nullptr;
        Vertex const* vertices = nullptr;
        uint32_t const* indices = nullptr;
    };

    struct SoundView
    {
        SoundHeader const* header = nullptr;
        float const* samples = nullptr;
    };

    // Returns nullptr if the handle has the wrong type or doesn't fit in the arena
    template<class H>
    H const* HeaderAt(std::vector<char> const& _arena, Handle _h)
    {
        if (_h.type != H::kType || _h.size < sizeof(H) || _h.offset + _h.size > _arena.size())
            return nullptr;
        return reinterpret_cast<H const*>(_arena.data() + _h.offset);
    }

    inline ImageView GetImage(std::vector<char> const& _arena, Handle _h)
    {
        ImageHeader const* h = HeaderAt<ImageHeader>(_arena, _h);
        if (!h)
            return {};
        return { h, reinterpret_cast<uint8_t const*>(h + 1) };
    }

    inline MeshView GetMesh(std::vector<char> const& _arena, Handle _h)
    {
        MeshHeader const* h = HeaderAt<MeshHeader>(_arena, _h);
        if (!h)
            return {};
        Vertex const* v = reinterpret_cast<Vertex const*>(h + 1);
        return { h, v, reinterpret_cast<uint32_t const*>(v + h->vertexCount) };
    }

    inline SoundView GetSound(std::vector<char> const& _arena, Handle _h)
    {
        SoundHeader const* h = HeaderAt<SoundHeader>(_arena, _h);
        if (!h)
            return {};
        return { h, reinterpret_cast<float const*>(h + 1) };
    }

    // helpers (used by the importers)

    template<class T>
    void Append(std::vector<char>& _out, T const& _value)
    {
        char const* p = reinterpret_cast<char const*>(&_value);
        _out.insert(_out.end(), p, p + sizeof(T));
    }

    inline void AppendBytes(std::vector<char>& _out, void const* _data, size_t _count)
    {
        char const* p = static_cast<char const*>(_data);
        _out.insert(_out.end(), p, p + _count);
    }
}

#endif
