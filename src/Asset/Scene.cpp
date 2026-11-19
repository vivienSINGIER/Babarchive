#include "Scene.h"

namespace
{
    std::string_view Trim(std::string_view _str)
    {
        while (!_str.empty() && (_str.front() == ' ' || _str.front() == '\r' || _str.front() == '\n'))
            _str.remove_prefix(1);
        while (!_str.empty() && (_str.back() == ' ' || _str.back() == '\r' || _str.back() == '\n'))
            _str.remove_suffix(1);
        return _str;
    }
    
    void SkipSpaces(std::string_view _str, size_t& _i, size_t& _end)
    {
        while (_i < _end && _str[_i] == ' ')
            _i += 1;
    }
    void SkipUntilSpace(std::string_view _str, size_t& _i, size_t& _end)
    {
        while (_i < _end && _str[_i] != ' ')
            _i += 1;
    }
    void SkipUntilEqual(std::string_view _str, size_t& _i, size_t& _end)
    {
        while (_i < _end && _str[_i] != '=')
            _i += 1;
    }
    void SkipUntilSpaceOrEqual(std::string_view _str, size_t& _i, size_t& _end)
    {
        while (_i < _end && _str[_i] != ' ' && _str[_i] != '=')
            _i += 1;
    }
    void SkipUntilQuote(std::string_view _str, size_t& _i, size_t& _end)
    {
        while (_i < _end && _str[_i] != '"')
            _i += 1;
    }
    
    using Attributes = std::unordered_map<std::string, std::string>;
    
    Error ParseSectionHeader(std::string_view _line, uint32_t _lineN, std::string& _kind, Attributes& _out)
    {
        size_t i = 1; // skip '['
        size_t end = _line.size() -1; // skip '['
        
        SkipSpaces(_line, i, end);
        size_t start = i;
        SkipUntilSpace(_line, i, end);
        
        _kind = std::string(_line.substr(start, i - start));
        
        while (true)
        {
            SkipSpaces(_line, i, end);
            if (i >= end)
                break;
            
            size_t keyStart = i;
            SkipUntilSpaceOrEqual(_line, i, end);
            std::string key = std::string(_line.substr(keyStart, i - keyStart));
            
            if (i >= end || _line[i] != '=')
                return Error::Syntax("expected '=' after '" + key + "'", _lineN, (uint32_t)(i + 1));
            i++;
            
            std::string value;
            if (i < end && _line[i] == '"')
            {
                size_t valueStart = ++i;
                SkipUntilQuote(_line, i, end);
                if (i >= end)
                    return Error::Syntax("unterminated string", _lineN, (uint32_t)(i + 1));
                value = std::string(_line.substr(valueStart, i - valueStart));
                i++;
            }
            else
            {
                size_t valueStart = i;
                SkipUntilSpace(_line, i, end);
                value = std::string(_line.substr(valueStart, i - valueStart));
            }
            
            _out[key] = value;
        }
        return Error::Ok();
    }
    
    bool CheckIdValid(Attributes& _attributes, int& _id)
    {
        std::string_view v = _attributes["id"];
        
        std::from_chars_result res = std::from_chars(v.data(), v.data() + v.size(), _id);
        
        return res.ec == std::errc();
    }
    
    bool ParseExtResource(std::string_view _value, int& _id)
    {
        std::string_view const prefix = "ExtResource(";
        if (_value.size() <= prefix.size() || _value.substr(0, prefix.size()) != prefix || _value.back() != ')')
            return false;
        std::string_view num = _value.substr(prefix.size(), _value.size() - prefix.size() - 1);
        std::from_chars_result res = std::from_chars(num.data(), num.data() + num.size(), _id);
        return res.ec == std::errc();
    }
    
    bool GetTypeFromString(std::string_view _type, Asset::AssetType& _out)
    {
        if (_type == "Image")
        {
            _out = Asset::AssetType::Image;
            return true;
        }
        if (_type == "Mesh")
        {
            _out = Asset::AssetType::Mesh;
            return true;
        }
        if (_type == "Sound")
        {
            _out = Asset::AssetType::Sound;
            return true;
        }
        return false;
    }
    
    bool ParseFloats(std::string_view _v, float* _out, int _count)
    {
        if (_v.size() < 2 || _v.front() != '(' || _v.back() != ')')
            return false;
        
        std::string s(_v.substr(1, _v.size() - 2));
        
        for (char& c : s)
        {
            if (c == ',' || c == 'f')
                c = ' ';   
        }
        
        std::istringstream ss(s);
        for (int i = 0; i < _count; ++i)
        {
            if (!(ss >> _out[i]))
                return false;
        }
        std::string rest;
        return !(ss >> rest);
    }
}

Error Asset::Scene::Load(AssetHandler& _assets, std::string const& _archive, std::string const& _name)
{
    Clear();
    
    // Read the scene txt file
    std::vector<char> sceneData;
    Handle txtHandle;
    if (Error e = _assets.Load(_archive, _name, sceneData, txtHandle))
        return e;
    
    if (txtHandle.type != AssetType::Scene)
        return Error::Io("'" + _name + "' is not a scene");
    
    std::istringstream in(std::string(sceneData.data() + txtHandle.offset, txtHandle.size));
    
    // Parse by line
    
    std::unordered_map<int, Handle> resources;
    std::unordered_map<std::string, int32_t> objectIndices;
    Object* curr = nullptr;
    
    std::string line;
    uint32_t lineN = 0;
    while (std::getline(in, line))
    {
        lineN++;
        std::string_view l = Trim(line);
        if (l.empty() || l[0] == '#')
            continue;
        
        if (l.front() == '[') // Section header detected
        {
            if (l.back() != ']')
                return Error::Syntax("expected missing ']'", lineN, (uint32_t)l.size());
            
            std::string kind;
            Attributes attributes;
            if (Error e = ParseSectionHeader(l, lineN, kind, attributes)) // Lexing step
                return e;
            
            if (kind == "ext_resource") // Load resources, they should be loaded before any usage by an object
            {
                int id;
                AssetType type;
                if (!CheckIdValid(attributes, id))
                    return Error::Syntax("ext_resource has no id", lineN, (uint32_t)l.size());
                if (!GetTypeFromString(attributes["type"], type))
                    return Error::Syntax("unknown resource type '" + attributes["type"] + "'", lineN, 0);
                
                std::string archive = attributes.contains("archive") ? attributes["archive"] : _archive; // default to current archive if none given
                Handle h;
                if (Error e = LoadResource(_assets, archive, attributes["name"], h))
                    return e;
                if (h.type != type)
                    return Error::Semantics("'" + attributes["name"] + "' is not a " + attributes["type"], lineN, 0);
                
                resources[id] = h;
                curr = nullptr;
            }
            else if (kind == "object")
            {
                Object object;
                object.name = attributes["name"];
                if (auto p = attributes.find("parent"); p != attributes.end())
                {
                    auto it = objectIndices.find(p->second);
                    if (it == objectIndices.end())
                        return Error::Semantics("unknown parent '" + p->second + "' (should be defined first)", lineN, 0);
                    object.parent = it->second;
                }
                
                objectIndices[object.name] = static_cast<uint32_t>(m_objects.size());
                m_objects.push_back(object);
                curr = &m_objects.back();
            }
            else 
                return Error::Syntax("unknown section '" + kind + "'", lineN, 2);
            continue;
        }
        
        // key = value 
        
        size_t eq = line.find('=');
        if (eq == std::string::npos)
            return Error::Syntax("expected 'key = value' expresion", lineN, 1);
        if (!curr)
            return Error::Syntax("property outside of an [object]", lineN, 1);
        
        // std::string_view lineView = l;
        std::string_view key = Trim(l.substr(0, eq));
        std::string_view value = Trim(l.substr(eq + 1));
        bool ok = true;
        
        if (key == "position")
            ok = ParseFloats(value, curr->transform.position, 3);
        else if (key == "rotation")
            ok = ParseFloats(value, curr->transform.rotation, 4);
        else if (key == "scale")
            ok = ParseFloats(value, curr->transform.scale, 3);
        else if (key == "mesh" || key == "texture")
        {
            int id;
            ok = ParseExtResource(value, id);
            if (ok)
            {
                if (key == "mesh")
                    curr->mesh = resources[id];
                else
                    curr->texture = resources[id];
            }
        }
        else
            return Error::Syntax("unknown property '" + std::string(key) + "'", lineN, 1);
        
        if (!ok)
            return Error::Syntax("invalid value for '" + std::string(key) + "'", lineN, (uint32_t)eq + 2);
        
    }
    return Error::Ok();
}

void Asset::Scene::Clear()
{
    m_arena.clear();
    m_objects.clear();
    m_loaded.clear();
}

Error Asset::Scene::LoadResource(AssetHandler& _assets, std::string const& _archive, std::string const& _name,
    Handle& _out)
{
    std::string key = _archive + "|" + _name;
    if (m_loaded.find(key) != m_loaded.end())
    {
        _out = m_loaded[key];
        return Error::Ok();
    }
    if (Error e = _assets.Load(_archive, _name, m_arena, _out))
        return e;
    m_loaded[key] = _out;
    return Error::Ok();
}
