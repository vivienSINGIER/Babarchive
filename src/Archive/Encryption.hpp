#ifndef BABARCHIVE_ARCHIVE_ENCRYPTION_HPP_DEFINED
#define BABARCHIVE_ARCHIVE_ENCRYPTION_HPP_DEFINED
#include <array>
#include <string>

namespace Encryption
{
    inline std::array<std::string, 3> GetHashes(std::string const& _key)
    {
        std::array<std::string, 3> hashs = {"", "", ""};
        if (_key.empty())
            return hashs;
        
        int hashLengthDiff = 20;
        for (int i = 0; i < 3; i++)
        {
            std::string& s = hashs[i];
            
            uint64_t hashLength = _key.length() + hashLengthDiff * i;
            s.resize((size_t)hashLength);
            
            std::hash<std::string> hash;
            size_t sHash = (i == 0) ? hash(_key) : hash(hashs[i - 1]);
            for (uint64_t j = 0; j < hashLength; j++)
            {
                int index = (sHash * j) % _key.length();
                char value = _key[index] * sHash * j;
                s[(int)j] = value;
            }
        }
        
        return hashs;
    }
    
    inline void Encrypt(std::array<std::string, 3> const& _hashes, char* _data, uint64_t _dataLength, uint64_t _offset)
    {
        if (_hashes[0].empty())
            return;
        
        for (uint64_t i = 0; i < _dataLength; i++)
        {
            uint64_t index = _offset + i;
            for (int h = 0; h < 3; h++)
            {
                _data[i] = _data[i] + _hashes[h][index % _hashes[h].length()];
            }   
        }
    }
    
    inline void Decrypt(std::array<std::string, 3> const& _hashes, char* _data, uint64_t _dataLength, uint64_t _offset)
    {
        if (_hashes[0].empty())
            return;
        
        for (uint64_t i = 0; i < _dataLength; i++)
        {
            uint64_t index = _offset + i;
            for (int h = 2; h >= 0; h--)
            {
                _data[i] = _data[i] - _hashes[h][index % _hashes[h].length()];
            }
        }
    }
    
}

#endif
