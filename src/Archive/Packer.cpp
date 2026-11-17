#include "Packer.h"

#include <fstream>

#include "Structs.h"

#include <zlib.h>

Archive::Packer::Packer()
{
     m_isOpen = false;  
     m_header = {};
}

Error Archive::Packer::Open(fs::path _location)
{
     std::ifstream src(_location, std::ios::in | std::ios::binary);
     if (src.is_open() == false)
          return Error::Io("couldn't open archive file", _location);
     
     m_isOpen = true;
     m_location = _location;
     m_descriptors.clear();
     
     if (Error e = ReadHeader(src, m_header))
          return e;
     if (m_header.magicNumber != g_magicNumber)
          return Error::Io("Read file doesn't fit the archive format, the file may be corrupted");
     if (m_header.version != g_currentVersion)
          Log::Log(LogType::Warning, "read archive version doesn't match the program");
     if (Error e = ReadDescriptors(src, m_header.descriptorListOffset, m_descriptors))
          return e;
     
     
     return Error::Ok();
}

Error Archive::Packer::Create(fs::path _location, std::string _name)
{
     fs::path path = _location / (_name + ".babar");
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
     
     m_location = path;
     m_isOpen = true;
     
     std::ofstream _out(path, std::ios::in | std::ios::binary);
     
     m_header = { g_magicNumber, g_currentVersion, 0, sizeof(BaseHeader) };
     
     if (Error e = WriteHeader(_out, m_header))
          return e;
     
     return Error::Ok();
}

Error Archive::Packer::Add(fs::path _file, std::string const& _name, uint8_t _flags, uint8_t _type)
{
     if (m_isOpen == false)
          return Error::Io("cannot write on non created archive file", m_location);
     
     if (!FileHelper::Exists(_file))
          return Error::Io("cannot add non existing file to archive", _file);
     
     bool isDir = FileHelper::IsDir(_file);
     
     std::ofstream out(m_location, std::ios::in | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     out.seekp(m_header.descriptorListOffset);
     
     Error err = Error::Io("Failed to add file/s to archive", _file);
     
     // Check for duplicates
     if (CheckDuplicates(m_descriptors, _name))
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
               
               if (CheckDuplicates(m_descriptors, name))
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
          
          m_descriptors.push_back(d);
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
               
               m_descriptors.push_back(d);
          }
     }
     
     m_header.dataSize += (uint64_t)out.tellp() - m_header.descriptorListOffset;
     m_header.descriptorListOffset = out.tellp();
     
     // Write descriptor list
     for (auto& desc : m_descriptors)
     {
          if (Error e = WriteDescriptor(out, desc))
               err.details.push_back(e.Format());
     }
     
     if (Error e = WriteHeader(out, m_header))
          e.details.push_back(e.Format());
     
     if (err.details.size() > 0)
          return err;
     return Error::Ok();
}

Error Archive::Packer::Add(const char* _data, uint64_t _count, std::string const& _name, uint8_t _flags, uint8_t _type)
{
     if (m_isOpen == false)
          return Error::Io("cannot write on non created archive file", m_location);
     
     std::ofstream out(m_location, std::ios::in | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     out.seekp(m_header.descriptorListOffset);
          
     Error err = Error::Io("Failed to add file/s to archive");
     
     FileHeader fHeader;
     fHeader.extension = "";
     fHeader.fixed.extensionLength = 0;
     fHeader.fixed.fileType = _type;
     fHeader.fixed.flags = _flags;
          
     FileDescriptor d;
     d.offset = out.tellp();
     d.nameLength = _name.length();
     d.name = _name;
          
     if (Error e = WriteRawFile(out, _data, _count, fHeader))
          err.details.push_back(e.Format());
          
     m_descriptors.push_back(d);
     
     return Error::Ok();
}

Error Archive::Packer::Remove(std::string_view _name)
{
     if (m_isOpen == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     bool found = false;
     uint64_t toRemoveOffset = 0;
     int index = 0;
     uint64_t descLength = 0;
     for (int i = 0; i < m_descriptors.size(); i++)
     {
          if (m_descriptors[i].name == _name)
          {
               index = i;
               found = true;
               toRemoveOffset = m_descriptors[i].offset;
               descLength = m_descriptors[i].DataSize();
               break;
          }
     }
     
     if (!found)
          return Error::Io("couldn't find file in archive", m_location);
     std::ifstream in(m_location, std::ios::in | std::ios::binary);
     if (in.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     FileHeader fHeader;
     if (ReadFileHeader(in, toRemoveOffset, fHeader))
          return Error::Io("couldn't read file header", m_location);
     
     uint64_t toRemoveLength = fHeader.DataSize();
     if (fHeader.fixed.flags & FileFlags::Compressed)
          toRemoveLength += fHeader.fixed.compressedSize;
     else
          toRemoveLength += fHeader.fixed.uncompressedSize;
     
     m_header.descriptorListOffset -= toRemoveLength;
     m_descriptors.erase(m_descriptors.begin() + index);
     
     in.close();
     
     if (Error e = FileHelper::EraseRange(m_location, toRemoveOffset, toRemoveLength))
          return e;
     
     std::ofstream out(m_location, std::ios::out | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     if (Error e = WriteHeader(out, m_header))
          return e;
     out.seekp(m_header.descriptorListOffset);
     for (FileDescriptor& d : m_descriptors)
     {
          if (Error e = WriteDescriptor(out, d))
               return e;
     }
     out.close();
     
     uint64_t size = fs::file_size(m_location);
     fs::resize_file(m_location, size - descLength);
     
     return Error::Ok();
}

Error Archive::Packer::ReadFile(std::string const& _name, FileHeader& _header, std::unique_ptr<FileStream>& _stream)
{
     if (m_isOpen == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     bool found = false;
     uint64_t offset = 0;
     for (int i = 0; i < m_descriptors.size(); i++)
     {
          if (m_descriptors[i].name == _name)
          {
               found = true;
               offset = m_descriptors[i].offset;
               break;
          }
     }
     
     if (!found)
          return Error::Io("couldn't find file in archive", m_location);
     std::ifstream in(m_location, std::ios::in | std::ios::binary);
     
     if (in.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     FileHeader fHeader;
     if (ReadFileHeader(in, offset, fHeader))
          return Error::Io("couldn't read file header", m_location);
     
     _header = fHeader;
     
     std::unique_ptr<FileStream> ptr;
     uint64_t strOffset = offset + fHeader.DataSize();
     uint64_t dataSize;
      
     if (fHeader.fixed.flags & FileFlags::Compressed)
     {
          ptr = std::make_unique<CompressedFileStream>();
          dataSize = fHeader.fixed.compressedSize;
     }
     else
     {
          ptr = std::make_unique<UncompressedFileStream>();
          dataSize = fHeader.fixed.uncompressedSize;
     }
     
     if (Error e = ptr->Open(m_location, strOffset, dataSize))
          return e;
     
     _stream = std::move(ptr);
     
     return Error::Ok();
}

void Archive::Packer::SetChunkSize(uint32_t _chunkSize)
{
     m_chunkSize = _chunkSize;
}

uint32_t Archive::Packer::GetChunkSize() const
{
     return m_chunkSize;
}

Error Archive::Packer::WriteHeader(std::ofstream& _out, BaseHeader const& _header)
{
     if (m_isOpen == false)
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

Error Archive::Packer::WriteRawFile(std::ofstream& _out, const char* _data, uint64_t _count, FileHeader& _fHeader)
{
     std::streampos pos = _out.tellp();
     std::streampos dataPos = pos + std::streampos(_fHeader.DataSize());
     
     _out.seekp(dataPos);
     
     std::string chunk;
     chunk.resize(m_chunkSize);
     
     uLong crc = crc32(0L, Z_NULL, 0);
     
     _fHeader.fixed.compressedSize = 0;
     _fHeader.fixed.uncompressedSize = 0;
     
     uint32_t toWrite = _count;
     uint32_t dataCursor = 0;
     
     while (toWrite > 0)
     {
          uint32_t count = std::min(toWrite, m_chunkSize);
          
          crc = crc32(crc, reinterpret_cast<const Bytef*>(_data + dataCursor), count);
          
          _out.write(_data + dataCursor, count);
          toWrite -= count;
          dataCursor += count;
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
     if (m_isOpen == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     _in.seekg(0);
     _in.read(reinterpret_cast<char*>(&_header), sizeof(BaseHeader));
     
     if (!_in)
          return Error::Io("couldn't read header", m_location);
     
     return Error::Ok();
}

Error Archive::Packer::ReadDescriptors(std::ifstream& _in, uint64_t _offset, std::vector<FileDescriptor>& _out)
{
     if (m_isOpen == false)
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
     if (m_isOpen == false)
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
     if (m_isOpen == false)
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
