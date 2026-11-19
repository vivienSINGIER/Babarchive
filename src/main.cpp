#include <iostream>

#include "Archive/Packer.h"
#include "Asset/AssetPacker.h"
#include "Asset/Scene.h"

namespace
{
    void PrintFloats(char const* _label, float const* _v, int _count)
    {
        std::cout << "    " << _label << "(";
        for (int i = 0; i < _count; ++i)
            std::cout << (i ? ", " : "") << _v[i];
        std::cout << ")\n";
    }
}

int main()
{
#ifdef BABARCHIVE_WORKING_DIR
    fs::current_path(BABARCHIVE_WORKING_DIR);
#endif

    // Build a fresh archive holding the scene and its texture
    std::string const archivePath = "Test/scene.babar";
    fs::remove(archivePath);

    Archive::Packer packer;
    if (Error e = packer.Create("Test", "scene"))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }
    if (Error e = packer.Open(archivePath))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }
    if (Error e = Asset::Pack(packer, "Test/chat.bmp", "chat"))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }
    if (Error e = Asset::Pack(packer, "Test/scene.txt", "scene"))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }

    // Load the scene back and print its objects
    Asset::AssetHandler assets;
    Asset::Scene scene;
    if (Error e = scene.Load(assets, archivePath, "scene"))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }

    std::cout << "Scene loaded: " << scene.m_objects.size() << " objects, arena " << scene.m_arena.size() << " bytes\n";
    for (size_t i = 0; i < scene.m_objects.size(); ++i)
    {
        Asset::Object const& o = scene.m_objects[i];
        std::cout << "[" << i << "] " << o.name << "\n";
        std::cout << "    parent:   " << (o.parent >= 0 ? scene.m_objects[o.parent].name : "<none>") << "\n";
        PrintFloats("position: ", o.transform.position, 3);
        PrintFloats("rotation: ", o.transform.rotation, 4);
        PrintFloats("scale:    ", o.transform.scale, 3);
        std::cout << "    mesh:     " << (o.mesh.IsValid() ? "yes" : "<none>") << "\n";

        if (Asset::ImageView img = Asset::GetImage(scene.m_arena, o.texture); img.header)
            std::cout << "    texture:  " << img.header->width << "x" << img.header->height
                      << " (offset " << o.texture.offset << ", " << o.texture.size << " bytes)\n";
        else
            std::cout << "    texture:  <none>\n";
    }

    return 0;
}
