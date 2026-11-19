#ifndef BABARCHIVE_BABARSETDEMO_RLIBHELPERS_HPP_DEFINED
#define BABARCHIVE_BABARSETDEMO_RLIBHELPERS_HPP_DEFINED

#include "raylib.h"
#include "raymath.h"
#include "Asset/AssetTypes.h"
#include "Asset/Scene.h"

namespace BabarsetDemo
{
    inline Texture2D ToTexture(Asset::ImageView image)
    {
        Image img{};
        img.data = const_cast<uint8_t*>(image.pixels);
        img.width = (int)image.header->width;
        img.height = (int)image.header->height;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        return LoadTextureFromImage(img);
    }
    
    inline Mesh ToMesh(Asset::MeshView _m)
    {
        Mesh m{};
        uint32_t n = _m.header->indexCount;
        m.vertexCount = (int)n;
        m.triangleCount = (int)n / 3;
        m.vertices = (float*)MemAlloc(n * 3 * sizeof(float));
        m.normals = (float*)MemAlloc(n * 3 * sizeof(float));
        m.texcoords = (float*)MemAlloc(n * 2 * sizeof(float));
        
        for (uint32_t i = 0; i < n; i++)
        {
            Asset::Vertex const& v = _m.vertices[_m.indices[i]];
            memcpy(m.vertices + 3 * i, &v.pos, sizeof(float) * 3);
            memcpy(m.normals + 3 * i, &v.normal, sizeof(float) * 3);
            memcpy(m.texcoords + 2 * i, &v.uv, sizeof(float) * 2);
        }
        UploadMesh(&m, false);
        return m;
    }
    
    inline Matrix ToMatrix(Asset::Transform _t)
    {
        Matrix s = MatrixScale(_t.scale[0], _t.scale[1], _t.scale[2]);
        Matrix r = QuaternionToMatrix({_t.rotation[0], _t.rotation[1], _t.rotation[2], _t.rotation[3]});
        Matrix t = MatrixTranslate(_t.position[0], _t.position[1], _t.position[2]);
        
        return MatrixMultiply(MatrixMultiply(s, r), t);
    }
    
}

#endif
