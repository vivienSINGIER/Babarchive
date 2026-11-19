#include <map>

#include "Asset/AssetPacker.h"
#include "Asset/AssetTypes.h"

namespace
{
    bool ToInt(std::string_view _str, int& _out)
    {
        std::from_chars_result res = std::from_chars(_str.data(), _str.data() + _str.size(), _out);
        return res.ec == std::errc();
    }
    
    bool ParseFaceToken(std::string_view _token, int& _v, int& _uv, int& _n)
    {
        _v = 0;
        _uv = 0;
        _n = 0;
        
        size_t s1 = _token.find('/');
        std::string_view str1 =  _token.substr(0, s1);
        if (!ToInt(str1, _v))
            return false;
        if (s1 == std::string_view::npos)
            return true;
        
        size_t s2 = _token.find('/', s1 + 1);
        std::string_view str2 =  _token.substr(s1 + 1, s2 - s1 - 1);
        if (!ToInt(str2, _uv))
            return false;
        if (s2 == std::string_view::npos)
            return true;
        
        std::string_view str3 =  _token.substr(s2 + 1);
        if (!ToInt(str3, _n))
            return false;
        
        return true;
    }
    
    // Indices can be negative, -1 represents the latest defined vertex
    // returns -1 if the resolved index is incorrect
    int Resolve(int _index, size_t _currCount)
    {
        if (_index < 0)
            _index = static_cast<int>(_currCount) + _index + 1;
        
        if (_index >= 1 && _index <= static_cast<int>(_currCount))
            return _index - 1;
        
        return -1;
    }
}

Error Asset::ImportObj(fs::path const& _src, std::vector<char>& _out)
{
    std::ifstream in(_src, std::ios::binary);
    if (!in)
        return Error::Io("Could not open file", _src);
    
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::array<float, 2>> uvs;
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    std::map<std::tuple<int, int, int>, uint32_t> uniqueIndices;
    
    std::string line;
    uint32_t lineN = 0;
    
    while (std::getline(in, line))
    {
        lineN++;
        std::istringstream ss(line);
        std::string header;
        ss >> header;
        
        if (header == "v") // positions
        {
            std::array<float, 3> p{};
            ss >> p[0] >> p[1] >> p[2];
            positions.push_back(p);
        }
        else if (header == "vt") // uvs
        {
            std::array<float, 2> uv{};
            ss >> uv[0] >> uv[1];
            uvs.push_back(uv);
        }
        else if (header == "vn") // normals
        {
            std::array<float, 3> normal{};
            ss >> normal[0] >> normal[1] >> normal[2];
            normals.push_back(normal);
        }
        else if (header == "f")
        {
            std::vector<uint32_t> face;
            std::string faceToken;
            while (ss >> faceToken)
            {
                int v;
                int uv;
                int n;
                if (!ParseFaceToken(faceToken, v, uv, n))
                    return Error::Syntax("invalid face index '" + faceToken + "'", lineN, 0);
                
                int vIndex = Resolve(v, positions.size());
                int uvIndex = uv ? Resolve(uv, uvs.size()) : -1;
                int nIndex = n ? Resolve(n, normals.size()) : -1;
                
                if (vIndex < 0 || (uv && uvIndex < 0) || (n && nIndex < 0))
                    return Error::Syntax("face index out of range '" + faceToken + "'", lineN, 0);
                
                std::tuple<int, int, int> key = std::make_tuple(vIndex, uvIndex, nIndex);
                auto it = uniqueIndices.find(key);
                if (it == uniqueIndices.end()) // Skip if vertex already exists 
                {
                    Vertex vert;
                    std::memcpy(&vert.pos, positions[vIndex].data(), sizeof(float) * 3);
                    if (uvIndex >= 0)
                        std::memcpy(&vert.uv, uvs[uvIndex].data(), sizeof(float) * 2);
                    if (nIndex >= 0)
                        std::memcpy(&vert.normal, normals[nIndex].data(), sizeof(float) * 3);
                    it = uniqueIndices.emplace(key, static_cast<uint32_t>(uniqueIndices.size())).first;
                    vertices.push_back(vert);
                }
                face.push_back(it->second);
            }
            if (face.size() < 3)
                return Error::Syntax("face with less than 3 vertices", lineN, 0);
            
            for (size_t i = 1; i + 1 < face.size(); i++) // fan triangulation
            {
                indices.push_back(face[0]);
                indices.push_back(face[i]);
                indices.push_back(face[i + 1]);
            }
        }
    }
    
    if (indices.empty())
        return Error::Io("obj has no geometry", _src);
    
    _out.clear();
    MeshHeader header { static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(indices.size()) };
    Append(_out, header);
    AppendBytes(_out, vertices.data(), vertices.size() * sizeof(Vertex));
    AppendBytes(_out, indices.data(), indices.size() * sizeof(uint32_t));
    return Error::Ok();
}
