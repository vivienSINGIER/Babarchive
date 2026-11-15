#include "Packer.h"

#include <fstream>

#include "Structs.h"

#include <zlib.h>

Archive::Packer::Packer()
{
     m_isCreated = false;   
}

Error Archive::Packer::Create(fs::path _location, std::string _name)
{
     fs::path path = _location / (_name + ".babar");
     m_location = path;
     if (FileHelper::Exists(path))
     {
          Error e = Error::Io("file '" + _name + "' already exists", path);
          return e;
     }
     
     if (FileHelper::WriteFileAtomic(path, ""))
     {
          Error e = Error::Io("couldn't create file '" + _name + "'", path);
          return e;
     }
     m_isCreated = true;
     
     std::ofstream _out(path, std::ios::in | std::ios::binary);
     
     BaseHeader header {g_magicNumber, g_currentVersion, 0, sizeof(BaseHeader)};
     
     if (Error e = WriteHeader(_out, header))
          return e;
     
     return Error::Ok();
}

Error Archive::Packer::Add(fs::path _file, uint8_t _flags, std::string const& _name, FileType _type)
{
     if (m_isCreated == false)
          return Error::Io("cannot write on non created archive file", m_location);
     
     if (!FileHelper::Exists(_file))
          return Error::Io("cannot add non existing file to archive", _file);
     
     bool isDir = FileHelper::IsDir(_file);
     
     BaseHeader header;
     std::vector<FileDescriptor> descriptors;
     
     std::ifstream src(m_location, std::ios::in | std::ios::binary);
     if (src.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     if (Error e = ReadHeader(src, header))
          return e;
     if (header.magicNumber != g_magicNumber)
          return Error::Io("Read file doesn't fit the archive format, the file may be corrupted");
     if (header.version != g_currentVersion)
          Log::Log(LogType::Warning, "read archive version doesn't match the program");
     if (Error e = ReadDescriptors(src, header.descriptorListOffset, descriptors))
          return e;
     
     src.close();
     
     std::ofstream out(m_location, std::ios::in | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     out.seekp(header.descriptorListOffset);
     
     Error err = Error::Io("Failed to add file/s to archive", _file);
     
     // Check for duplicates
     if (CheckDuplicates(descriptors, _name))
          return Error::Io("file '" + _name + "' already exists in archive", m_location);
     if (isDir)
     {
          std::vector<fs::path> files;
          if (Error e = FileHelper::ListDir(_file, files))
               err.details.push_back(e.Format());
          
          for (auto& f : files)
          {
               if (FileHelper::IsDir(f))
                    continue;
               
               std::string name = FileHelper::NormalizePath(_name / FileHelper::RelativeTo(f, _file)).string();
               
               if (CheckDuplicates(descriptors, name))
                    return Error::Io("file '" + name + "' already exists in archive", m_location);
          }
     }
     
     // Write the files
     // TODO add compressed versions methods & calls
     if (!isDir) // One single file given
     {
          std::ifstream in(_file, std::ios::in | std::ios::binary);
          if (in.is_open() == false)
               err.details.push_back(Error::Io("couldn't open targeted file", _file).Format());
          
          FileHeader fHeader;
          fHeader.extension = FileHelper::ExtLower(_file);
          fHeader.fixed.extensionLength = fHeader.extension.length();
          fHeader.fixed.fileType = _type;
          fHeader.fixed.flags = _flags;
          
          FileDescriptor d;
          d.offset = out.tellp();
          d.nameLength = _name.length();
          d.name = _name;
          
          if (Error e = WriteFile(out, in, fHeader))
               err.details.push_back(e.Format());
          
          descriptors.push_back(d);
     }
     else // Loop over each file in given directory
     {
          std::vector<fs::path> files;
          if (Error e = FileHelper::ListDir(_file, files))
               err.details.push_back(e.Format());
          
          for (auto& f : files)
          {
               if (FileHelper::IsDir(f))
                    continue;
               
               fs::path fPath = FileHelper::NormalizePath(_name / FileHelper::RelativeTo(f, _file));
               
               std::ifstream in(f, std::ios::in | std::ios::binary);
               if (in.is_open() == false)
                    err.details.push_back(Error::Io("couldn't open targeted file", f).Format());
               
               FileHeader fHeader;
               fHeader.extension = FileHelper::ExtLower(f);
               fHeader.fixed.extensionLength = fHeader.extension.length();
               fHeader.fixed.fileType = _type;
               fHeader.fixed.flags = _flags;
          
               FileDescriptor d;
               d.offset = out.tellp();
               d.nameLength = fPath.string().length();
               d.name = fPath.string();
               
               if (Error e = WriteFile(out, in, fHeader))
                    err.details.push_back(e.Format());
          
               descriptors.push_back(d);
          }
     }
     
     header.dataSize += (uint64_t)out.tellp() - header.descriptorListOffset;
     header.descriptorListOffset = out.tellp();
     
     // Write descriptor list
     for (auto& desc : descriptors)
     {
          if (Error e = WriteDescriptor(out, desc))
               err.details.push_back(e.Format());
     }
     
     if (Error e = WriteHeader(out, header))
          e.details.push_back(e.Format());
     
     if (err.details.size() > 0)
          return err;
     return Error::Ok();
}

uint32_t Archive::Packer::GetChunkSize() const
{
     return m_chunkSize;
}

Error Archive::Packer::WriteHeader(std::ofstream& _out, BaseHeader const& _header)
{
     if (m_isCreated == false)
          return Error::Io("cannot write on non created archive file", m_location);
     
     _out.seekp(0);
     _out.write((char*)&_header, sizeof(Archive::BaseHeader));
     
     if (!_out)
          return Error::Io("couldn't write header", m_location); 
     
     return Error::Ok();
}

Error Archive::Packer::WriteDescriptor(std::ofstream& _out, FileDescriptor const& _desc)
{
     _out.write(reinterpret_cast<const char*>(&_desc), sizeof(_desc.nameLength));
     _out.write(_desc.name.data(), _desc.nameLength);
     _out.write(reinterpret_cast<const char*>(&_desc.offset), sizeof(_desc.offset));
     
     if (!_out)
          return Error::Io("couldn't write descriptor", m_location);
     
     return Error::Ok();
}

Error Archive::Packer::WriteFile(std::ofstream& _out, std::ifstream& _in, FileHeader& _fHeader)
{
     std::streampos pos = _out.tellp();
     std::streampos dataPos = pos + std::streampos(_fHeader.DataSize());
     
     _out.seekp(dataPos);
     
     std::string chunk;
     chunk.resize(m_chunkSize);
     
     uLong crc = crc32(0L, Z_NULL, 0);
     
     _fHeader.fixed.compressedSize = 0;
     _fHeader.fixed.uncompressedSize = 0;
     
     while (_in.read(chunk.data(), m_chunkSize) || _in.gcount() > 0)
     {
          crc = crc32(crc, reinterpret_cast<const Bytef*>(chunk.data()), _in.gcount());
          
          // TODO Add encryption step
          _fHeader.fixed.uncompressedSize += _in.gcount();
          
          _out.write(chunk.data(), _in.gcount());
     }
     
     _fHeader.fixed.compressedSize = _fHeader.fixed.uncompressedSize;
     _fHeader.fixed.crc32 = (uint32_t)crc;
     
     // Write header
     _out.seekp(pos);
     _out.write(reinterpret_cast<const char*>(&_fHeader), sizeof(FileHeaderFixed));
     _out.write(_fHeader.extension.data(), _fHeader.fixed.extensionLength);
     _out.seekp(dataPos + std::streampos(_fHeader.fixed.uncompressedSize)); // return cursor to end of file data to maintain loo^p integrity
     
     if (!_out)
          return Error::Io("couldn't write file in archive", m_location);
     
     return Error::Ok();
}

Error Archive::Packer::ReadHeader(std::ifstream& _in, BaseHeader& _header)
{
     if (m_isCreated == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     _in.seekg(0);
     _in.read(reinterpret_cast<char*>(&_header), sizeof(BaseHeader));
     
     if (!_in)
          return Error::Io("couldn't read header", m_location);
     
     return Error::Ok();
}

Error Archive::Packer::ReadDescriptors(std::ifstream& _in, uint64_t _offset, std::vector<FileDescriptor>& _out)
{
     if (m_isCreated == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     uint64_t size = GetFileSize();
     uint64_t offset = _offset;
     
     while (offset < size)
     {
          Error eob = Error::Io("cursor out of bounds, couldn't read file descriptor without going out of file bounds", m_location);
          
          _in.seekg(offset);
          
          FileDescriptor d;
          
          if (!_in.read(reinterpret_cast<char*>(&d.nameLength), sizeof(uint32_t)))
               return eob;
          
          offset += sizeof(uint32_t);
          offset += d.nameLength;
          if (offset > size)
               return eob;
          
          d.name.resize(d.nameLength);
          if (!_in.read(&d.name[0], d.nameLength))
               return eob;
          
          if (!_in.read(reinterpret_cast<char*>(&d.offset), sizeof(uint64_t)))
              return eob;
          offset += sizeof(uint64_t);
          
          if (offset > size)
               return eob;
          
          _out.push_back(d);
     }
     
     return Error::Ok();
}

Error Archive::Packer::ReadFileHeader(std::ifstream& _in, uint64_t _offset, FileHeader& _header)
{
     if (m_isCreated == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     uint64_t size = GetFileSize();
     if (_offset > size)
          return Error::Io("cursor out of bounds", m_location);
     
     _in.seekg(_offset);
     _in.read(reinterpret_cast<char*>(&_header), sizeof(FileHeaderFixed));
     
     _header.extension.resize(_header.fixed.extensionLength);
     if (!_in.read(&_header.extension[0], _header.fixed.extensionLength) || !_in)
          return Error::Io("couldn't read file header", m_location);
     
     return Error::Ok();
}

uint64_t Archive::Packer::GetFileSize()
{
     if (m_isCreated == false)
          return 0;
     
     return fs::file_size(m_location);
}

bool Archive::Packer::CheckDuplicates(std::vector<FileDescriptor> const& _desc, std::string const& _newName)
{
     for (FileDescriptor const& desc : _desc)
          if (desc.name == _newName)
               return true;
     
     return false;
}
