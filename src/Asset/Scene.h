#ifndef BABARCHIVE_ASSET_SCENE_H_DEFINED
#define BABARCHIVE_ASSET_SCENE_H_DEFINED
#include "AssetHandler.h"
#include "Core/Error.h"

namespace Asset
{
 
    struct Transform
    {
        float position[3] = { 0.0f, 0.0f, 0.0f };
        float rotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float scale[3] = { 1.0f, 1.0f, 1.0f };
    };
    
    struct Object
    {
        std::string name;
        int32_t parent = -1;
        Transform transform;
        Handle mesh;
        Handle texture;
    };
    
    class Scene
    {
    public:
        Error Load(AssetHandler& _assets, std::string const& _archive, std::string const& _name);
        void Clear();
        
        std::vector<char> m_arena;
        std::vector<Object> m_objects;
        
    private:
        Error LoadResource(AssetHandler& _assets, std::string const& _archive, std::string const& _name, Handle& _out);
        
        std::unordered_map<std::string, Handle> m_loaded;
    };
    
}

#endif
