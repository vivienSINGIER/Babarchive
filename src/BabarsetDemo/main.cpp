#include <iostream>

#include "raylib.h"
#include "Archive/Packer.h"
#include "Asset/AssetPacker.h"
#include "Asset/Scene.h"
#include "BabarsetDemo/RLibHelpers.hpp"

namespace
{
    void PrintFloats(char const* _label, float const* _v, int _count)
    {
        std::cout << "    " << _label << "(";
        for (int i = 0; i < _count; ++i)
            std::cout << (i ? ", " : "") << _v[i];
        std::cout << ")\n";
    }
    
    void PrintInfo(Asset::Scene const& scene, size_t _index)
    {
        Asset::Object const& o = scene.m_objects[_index];
        std::cout << "[" << _index << "] " << o.name << "\n";
        std::cout << "    parent:   " << (o.parent >= 0 ? scene.m_objects[o.parent].name : "<none>") << "\n";
        PrintFloats("position: ", o.transform.position, 3);
        PrintFloats("rotation: ", o.transform.rotation, 4);
        PrintFloats("scale:    ", o.transform.scale, 3);

        Asset::MeshView m = Asset::GetMesh(scene.m_arena, o.mesh);
        if (m.header)
            std::cout << "    mesh:  " << m.header->vertexCount<< " vertices | " << m.header->indexCount << " indices "
                      << " (offset " << o.mesh.offset << ", " << o.mesh.size << " bytes)\n";
        else
            std::cout << "    mesh:  <none>\n";
        
        Asset::ImageView img = Asset::GetImage(scene.m_arena, o.texture);
        if (img.header)
            std::cout << "    texture:  " << img.header->width << "x" << img.header->height
                      << " (offset " << o.texture.offset << ", " << o.texture.size << " bytes)\n";
        else
            std::cout << "    texture:  <none>\n";
    }
}

int main(int argc, char* argv[])
{
#ifdef BABARCHIVE_WORKING_DIR
    fs::current_path(BABARCHIVE_WORKING_DIR);
#endif
    
    if (argc != 3)
    {
        Error e = Error::Io("Invalid argument count");
        std::cout << e.Format() << std::endl;
        return e.Exit();
    }
    
    std::string archivePath = argv[1];
    std::string sceneName = argv[2];
    
    // Load the scene back and print its objects
    Asset::AssetHandler assets;
    Asset::Scene scene;
    if (Error e = scene.Load(assets, archivePath, sceneName))
    {
        std::cout << e.Format() << "\n";
        return e.Exit();
    }

    InitWindow(1280, 720, sceneName.c_str());
    SetTargetFPS(60);
    
    std::vector<Mesh> meshes(scene.m_objects.size());
    std::vector<Material> materials(scene.m_objects.size());
    std::vector<Matrix> transforms(scene.m_objects.size());
    Camera3D cam { { 7, 4, 7 }, { 0, 0, 0 },{ 0, 1, 0 }, 60, CAMERA_PERSPECTIVE };
    
    std::cout << "Scene loaded: " << scene.m_objects.size() << " objects, arena " << scene.m_arena.size() << " bytes\n";
    for (size_t i = 0; i < scene.m_objects.size(); ++i)
    {
        // DEBUG INFO
        PrintInfo(scene, i);

        Asset::Object& o = scene.m_objects[i];
        materials[i] = LoadMaterialDefault();
        
        Asset::MeshView m = Asset::GetMesh(scene.m_arena, o.mesh);
        if (m.header)
            meshes[i] = BabarsetDemo::ToMesh(m);
        
        Asset::ImageView img = Asset::GetImage(scene.m_arena, o.texture);
        if (img.header)
            materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = BabarsetDemo::ToTexture(img);
        else
            materials[i].maps[MATERIAL_MAP_ALBEDO].color = { o.color[0], o.color[1], o.color[2], o.color[3] };
        
        Matrix local = BabarsetDemo::ToMatrix(o.transform);
        int32_t p = o.parent;
        transforms[i] = p >= 0 ? MatrixMultiply(local, transforms[p]) : local;
    }
    
    // WINDOW LOOP
    while (!WindowShouldClose())
    {
        UpdateCamera(&cam, CAMERA_ORBITAL);
        BeginDrawing();
        ClearBackground(DARKGRAY);
        BeginMode3D(cam);
        DrawGrid(15, 1);
        
        for (size_t i = 0; i < meshes.size(); ++i)
            if (meshes[i].vertexCount > 0)
                DrawMesh(meshes[i], materials[i], transforms[i]);
        
        EndMode3D();
        DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
    
    return 0;
}
