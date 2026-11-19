#include "Packer.h"

#include <fstream>
#include <sstream>

#include "Structs.h"

#include <zlib.h>

#include "Encryption.hpp"

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

     // Read helpers require an open archive with a valid location
     m_location = _location;
     m_isOpen = true;
     m_descriptors.clear();

     Error e = ReadHeader(src, m_header);
     if (!e && m_header.magicNumber != g_magicNumber)
          e = Error::Io("Read file doesn't fit the archive format, the file may be corrupted", _location);
     if (!e && m_header.version != g_currentVersion)
          Log::Log(LogType::Warning, "read archive version doesn't match the program");
     if (!e)
          e = ReadDescriptors(src, m_header.descriptorListOffset, m_descriptors);

     if (e)
     {
          m_isOpen = false;
          m_descriptors.clear();
     }

     return e;
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

Error Archive::Packer::Add(fs::path _file, std::string const& _name, uint8_t _flags, uint8_t _type, std::string const& _key)
{
     if (m_isOpen == false)
          return Error::Io("cannot write on non created archive file", m_location);
     
     if (!FileHelper::Exists(_file))
          return Error::Io("cannot add non existing file to archive", _file);
     
     bool isDir = FileHelper::IsDir(_file);

     // Check for duplicates
     if (CheckDuplicates(m_descriptors, _name))
          return Error::Io("file '" + _name + "' already exists in archive", m_location);

     Error err = Error::Io("Failed to add file/s to archive", _file);

     if (isDir)
     {
          std::vector<fs::path> files;
          if (Error e = FileHelper::ListDir(_file, files))
               err.details.push_back(e.Format());

          for (auto& f : files)
          {
               if (FileHelper::IsDir(f))
                    continue;
               
               // Stored names have no extension, it is kept in the file header and restored on unpack
               std::string name = (fs::path(_name) / FileHelper::RelativeTo(f, _file)).lexically_normal().replace_extension().generic_string();
               
               if (CheckDuplicates(m_descriptors, name))
                    return Error::Io("file '" + name + "' already exists in archive", m_location);
          }
     }
     
     std::ofstream out(m_location, std::ios::in | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     out.seekp(m_header.descriptorListOffset);

     // Write the files
     if (!isDir) // One single file given
     {
          std::ifstream in(_file, std::ios::in | std::ios::binary);
          if (in.is_open() == false)
               err.details.push_back(Error::Io("couldn't open targeted file", _file).Format());
          else if (Error e = WriteEntry(out, in, _name, FileHelper::ExtLower(_file), _flags, _type, _key))
               err.details.push_back(e.Format());
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

               fs::path fPath = (fs::path(_name) / FileHelper::RelativeTo(f, _file)).lexically_normal().replace_extension();

               std::ifstream in(f, std::ios::in | std::ios::binary);
               if (in.is_open() == false)
               {
                    err.details.push_back(Error::Io("couldn't open targeted file", f).Format());
                    continue;
               }

               if (Error e = WriteEntry(out, in, fPath.generic_string(), FileHelper::ExtLower(f), _flags, _type, _key))
                    err.details.push_back(e.Format());
          }
     }

     if (Error e = WriteIndex(out))
          err.details.push_back(e.Format());

     if (err.details.size() > 0)
          return err;
     return Error::Ok();
}

Error Archive::Packer::Add(const char* _data, uint64_t _count, std::string const& _name, uint8_t _flags, uint8_t _type, std::string const& _key)
{
     if (m_isOpen == false)
          return Error::Io("cannot write on non created archive file", m_location);

     if (CheckDuplicates(m_descriptors, _name))
          return Error::Io("file '" + _name + "' already exists in archive", m_location);

     std::ofstream out(m_location, std::ios::in | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     out.seekp(m_header.descriptorListOffset);

     Error err = Error::Io("Failed to add data to archive", m_location);

     // Wrap the data in a stream so it goes through the same write path as files
     std::istringstream in(std::string(_data, _count), std::ios::in | std::ios::binary);

     if (Error e = WriteEntry(out, in, _name, "", _flags, _type, _key))
          err.details.push_back(e.Format());

     if (Error e = WriteIndex(out))
          err.details.push_back(e.Format());

     if (err.details.size() > 0)
          return err;
     return Error::Ok();
}

Error Archive::Packer::WriteEntry(std::ofstream& _out, std::istream& _in, std::string const& _name, 
     std::string const& _extension, uint8_t _flags, uint8_t _type, std::string const& _key)
{
     // The key alone decides: same rule for every Add
     if (_key.empty())
          _flags &= (uint8_t)~FileFlags::Encrypted;
     else
          _flags |= FileFlags::Encrypted;

     FileHeader fHeader{};
     fHeader.extension = _extension;
     fHeader.fixed.extensionLength = (uint16_t)_extension.length();
     fHeader.fixed.fileType = _type;
     fHeader.fixed.flags = _flags;

     FileDescriptor d;
     d.offset = _out.tellp();
     d.nameLength = (uint32_t)_name.length();
     d.name = _name;

     Error e = (_flags & FileFlags::Compressed)
          ? WriteFileCompressed(_out, _in, fHeader, _key)
          : WriteFile(_out, _in, fHeader, _key);

     if (e)
     {
          // On laisse ça sera overwrite
          _out.clear();
          _out.seekp(d.offset);
          return e;
     }

     m_descriptors.push_back(d);
     return Error::Ok();
}

Error Archive::Packer::WriteIndex(std::ofstream& _out)
{
     // _out must be positioned right after the last written file
     uint64_t end = (uint64_t)_out.tellp();
     m_header.dataSize = end - sizeof(BaseHeader);
     m_header.descriptorListOffset = end;

     for (auto& desc : m_descriptors)
     {
          if (Error e = WriteDescriptor(_out, desc))
               return e;
     }
     uint64_t indexEnd = (uint64_t)_out.tellp();

     if (Error e = WriteHeader(_out, m_header))
          return e;

     _out.close();

     // If previous too long we cut
     std::error_code ec;
     if (fs::file_size(m_location, ec) > indexEnd && !ec)
          fs::resize_file(m_location, indexEnd, ec);
     if (ec)
          return Error::Io("couldn't resize archive file", m_location);

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
     
     in.close();

     if (Error e = FileHelper::EraseRange(m_location, toRemoveOffset, toRemoveLength))
          return e;

     // Only update the in-memory state once the data is really gone from the file
     m_header.dataSize -= toRemoveLength;
     m_header.descriptorListOffset -= toRemoveLength;
     m_descriptors.erase(m_descriptors.begin() + index);

     std::ofstream out(m_location, std::ios::in | std::ios::out | std::ios::binary);
     if (out.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     if (Error e = WriteHeader(out, m_header))
          return e;
     out.seekp(m_header.descriptorListOffset);
     for (int i = 0; i < m_descriptors.size(); i++)
     {
          if (i >= index)
               m_descriptors[i].offset -= toRemoveLength;
          if (Error e = WriteDescriptor(out, m_descriptors[i]))
               return e;
     }
     out.close();
     
     uint64_t size = fs::file_size(m_location);
     fs::resize_file(m_location, size - descLength);
     
     return Error::Ok();
}

Error Archive::Packer::ReadFile(std::string const& _name, FileHeader& _header, std::unique_ptr<FileStream>& _stream, std::string const& _key)
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
     
     if (_header.fixed.flags & FileFlags::Encrypted)
          ptr->SetDecryptionKey(_key);
     
     _stream = std::move(ptr);
     
     return Error::Ok();
}

Error Archive::Packer::ListInfo(std::vector<FileInfo>& _files)
{
     if (m_isOpen == false)
          return Error::Io("cannot read non created archive file", m_location);
     
     if (m_descriptors.size() == 0)
          return Error::Ok();
     
     std::ifstream src(m_location, std::ios::in | std::ios::binary);
     if (src.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     Error err = Error::Io("failed to list archive file", m_location);
     
     for (int i = 0; i < m_descriptors.size(); i++)
     {
          FileHeader fHeader;
          if (Error e = ReadFileHeader(src, m_descriptors[i].offset, fHeader))
               err.details.push_back(e.Format());
               
          FileInfo info;
          info.name = m_descriptors[i].name;
          info.offset = m_descriptors[i].offset;
          info.dataSize = fHeader.fixed.uncompressedSize;
          info.currentSize = fHeader.fixed.compressedSize;
          info.isEncrypted = fHeader.fixed.flags & FileFlags::Encrypted;
          info.isCompressed = fHeader.fixed.flags & FileFlags::Compressed;
          info.extension = fHeader.extension;
          
          _files.push_back(info);
     }
     
     return err.details.empty() ? Error::Ok() : err;
}

Error Archive::Packer::Unpack(fs::path _unpackLocation, std::unordered_map<std::string, std::string> const& _keyMap)
{
     if (m_isOpen == false)
          return Error::Io("cannot unpack archive file", m_location);
     
     std::ifstream src(m_location, std::ios::in | std::ios::binary);
     if (src.is_open() == false)
          return Error::Io("couldn't open archive file", m_location);
     
     if (Error e = FileHelper::CreateDir(_unpackLocation))
          return e;

     fs::path const root = _unpackLocation.lexically_normal();
     Error err = Error::Io("Failed to unpack file/s from archive", m_location);

     for (auto& desc : m_descriptors)
     {
          fs::path rel = fs::path(desc.name).relative_path().lexically_normal();
          fs::path dst = (root / rel).lexically_normal();
          fs::path check = FileHelper::RelativeTo(dst, root);

          if (rel.empty() || check.empty() || *check.begin() == "..")
          {
               err.details.push_back(Error::Io("invalid file name '" + desc.name + "' in archive, skipped", m_location).Format());
               continue;
          }

          FileHeader fHeader;
          if (Error e = ReadFileHeader(src, desc.offset, fHeader))
          {
               err.details.push_back(e.Format());
               continue;
          }
          
          if (fHeader.fixed.flags & FileFlags::Encrypted)
          {
               if (!_keyMap.contains(desc.name))
               {
                    err.details.push_back(Error::Io("no key given for encrypted file, skipped '" + desc.name + "'", m_location).Format());
                    continue;    
               }
          }
          
          dst += fHeader.extension.empty() ? ".bin" : fHeader.extension;
          
          if (Error e = FileHelper::CreateDir(dst.parent_path()))
          {
               err.details.push_back(e.Format());
               continue;
          }

          std::ofstream out(dst, std::ios::out | std::ios::binary | std::ios::trunc);
          if (out.is_open() == false)
          {
               err.details.push_back(Error::Io("couldn't create unpacked file", dst).Format());
               continue;
          }

          std::string key = _keyMap.contains(desc.name) ? _keyMap.at(desc.name) : "";
          if (Error e = ExtractFile(src, out, fHeader, key))
          {
               e.path = dst;
               err.details.push_back(e.Format());
          }
     }

     if (err.details.size() > 0)
          return err;
     return Error::Ok();
}

Error Archive::Packer::ExtractFile(std::ifstream& _in, std::ofstream& _out, FileHeader const& _fHeader, std::string const& _key)
{
     std::string chunk;
     chunk.resize(m_chunkSize);

     uLong crc = crc32(0L, Z_NULL, 0);
     uint64_t written = 0;
     uint64_t toRead = _fHeader.fixed.compressedSize;

     bool encrypted = (_fHeader.fixed.flags & FileFlags::Encrypted) != 0;
     
     std::array<std::string, 3> hashMap = Encryption::GetHashes(_key);
     
     if ((_fHeader.fixed.flags & FileFlags::Compressed) == 0)
     {
          while (toRead > 0)
          {
               std::streamsize count = (std::streamsize)std::min<uint64_t>(toRead, m_chunkSize);
               if (!_in.read(chunk.data(), count))
                    return Error::Io("unexpected end of archive while reading file data", m_location);

               Encryption::Decrypt(hashMap, chunk.data(), count, written);
               
               crc = crc32(crc, reinterpret_cast<const Bytef*>(chunk.data()), (uInt)count);
               _out.write(chunk.data(), count);

               toRead -= count;
               written += count;
          }
     }
     else
     {
          z_stream zs = {};
          if (inflateInit(&zs) != Z_OK)
               return Error::Io("couldn't initialize zlib inflate stream", m_location);

          std::string outChunk;
          outChunk.resize(m_chunkSize);

          int ret = Z_OK;
          while (ret != Z_STREAM_END && toRead > 0)
          {
               std::streamsize count = (std::streamsize)std::min<uint64_t>(toRead, m_chunkSize);
               uint64_t storedOffset = _fHeader.fixed.compressedSize - toRead; // compressed bytes already read
               if (!_in.read(chunk.data(), count))
               {
                    inflateEnd(&zs);
                    return Error::Io("unexpected end of archive while reading compressed data", m_location);
               }
               // The compressed bytes are encrypted: decrypt them before inflating
               Encryption::Decrypt(hashMap, chunk.data(), count, storedOffset);
               toRead -= count;

               zs.next_in = reinterpret_cast<Bytef*>(chunk.data());
               zs.avail_in = (uInt)count;

               do
               {
                    zs.next_out = reinterpret_cast<Bytef*>(outChunk.data());
                    zs.avail_out = m_chunkSize;

                    ret = inflate(&zs, Z_NO_FLUSH);
                    if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR || ret == Z_STREAM_ERROR)
                    {
                         inflateEnd(&zs);
                         return Error::Io(encrypted ? "corrupted compressed data (wrong key?)" : "corrupted compressed data", m_location);
                    }

                    uInt produced = m_chunkSize - zs.avail_out;
                    crc = crc32(crc, reinterpret_cast<const Bytef*>(outChunk.data()), produced);
                    _out.write(outChunk.data(), produced);
                    written += produced;
               } while (zs.avail_out == 0 && ret != Z_STREAM_END);
          }

          inflateEnd(&zs);
          if (ret != Z_STREAM_END)
               return Error::Io("compressed data stream is incomplete", m_location);
     }

     if (!_out)
          return Error::Io("couldn't write unpacked file");
     if (written != _fHeader.fixed.uncompressedSize)
          return Error::Io("unpacked size doesn't match the file header, the archive may be corrupted", m_location);
     if ((uint32_t)crc != _fHeader.fixed.crc32)
          return Error::Io("crc32 mismatch, the archive may be corrupted", m_location);

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

Error Archive::Packer::WriteFile(std::ofstream& _out, std::istream& _in, FileHeader& _fHeader, std::string const& _key)
{
     std::streampos pos = _out.tellp();
     std::streampos dataPos = pos + std::streampos(_fHeader.DataSize());
     
     _out.seekp(dataPos);
     
     std::string chunk;
     chunk.resize(m_chunkSize);
     
     uLong crc = crc32(0L, Z_NULL, 0);
     
     _fHeader.fixed.compressedSize = 0;
     _fHeader.fixed.uncompressedSize = 0;
     
     bool encrypt = _fHeader.fixed.flags & FileFlags::Encrypted;
     
     std::array<std::string, 3> hashes = Encryption::GetHashes(_key);
     
     while (_in.read(chunk.data(), m_chunkSize) || _in.gcount() > 0)
     {
          crc = crc32(crc, reinterpret_cast<const Bytef*>(chunk.data()), _in.gcount());
          
          if (encrypt)
               Encryption::Encrypt(hashes, chunk.data(), _in.gcount(), _fHeader.fixed.uncompressedSize);
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

Error Archive::Packer::WriteFileCompressed(std::ofstream& _out, std::istream& _in, FileHeader& _fHeader, std::string const& _key)
{
     z_stream zs{};
     if (deflateInit(&zs, 1) != Z_OK)
          return Error::Io("couldn't initialize zlib deflate stream", m_location);

     std::streampos pos = _out.tellp();
     std::streampos dataPos = pos + std::streampos(_fHeader.DataSize());

     _out.seekp(dataPos);

     std::string inChunk(m_chunkSize, '\0');
     std::string outChunk(m_chunkSize, '\0');

     uLong crc = crc32(0L, Z_NULL, 0);

     _fHeader.fixed.compressedSize = 0;
     _fHeader.fixed.uncompressedSize = 0;

     int flush = Z_NO_FLUSH;
     int ret = Z_OK;

     bool encrypt = _fHeader.fixed.flags & FileFlags::Encrypted;

     std::array<std::string, 3> hashes = Encryption::GetHashes(_key);
     
     // Read one chunk, compress it, and finish the stream once the input is exhausted
     do
     {
          _in.read(inChunk.data(), m_chunkSize);
          const auto n = static_cast<uInt>(_in.gcount());

          if (_in.bad())
          {
               deflateEnd(&zs);
               return Error::Io("couldn't read file to compress", m_location);
          }

          // CRC on the original data, then compress, then encrypt the compressed bytes
          crc = crc32(crc, reinterpret_cast<const Bytef*>(inChunk.data()), n);
          _fHeader.fixed.uncompressedSize += n;

          flush = _in.eof() ? Z_FINISH : Z_NO_FLUSH;
          zs.next_in  = reinterpret_cast<Bytef*>(inChunk.data());
          zs.avail_in = n;

          // Drain deflate output until it has consumed the whole input chunk
          do
          {
               zs.next_out  = reinterpret_cast<Bytef*>(outChunk.data());
               zs.avail_out = static_cast<uInt>(outChunk.size());

               ret = deflate(&zs, flush);
               if (ret == Z_STREAM_ERROR)
               {
                    deflateEnd(&zs);
                    return Error::Io("zlib compression failed", m_location);
               }

               const std::size_t produced = outChunk.size() - zs.avail_out;
               // Position = offset of these bytes in the stored (compressed) data
               if (encrypt)
                    Encryption::Encrypt(hashes, outChunk.data(), produced, _fHeader.fixed.compressedSize);
               _out.write(outChunk.data(), produced);
               _fHeader.fixed.compressedSize += produced;

          } while (zs.avail_out == 0);

     } while (flush != Z_FINISH);

     deflateEnd(&zs);

     if (ret != Z_STREAM_END)
          return Error::Io("zlib compression failed", m_location);
     
     _fHeader.fixed.crc32 = (uint32_t)crc;
     
     // Write header
     _out.seekp(pos);
     _out.write(reinterpret_cast<const char*>(&_fHeader), sizeof(FileHeaderFixed));
     _out.write(_fHeader.extension.data(), _fHeader.fixed.extensionLength);
     _out.seekp(dataPos + std::streampos(_fHeader.fixed.compressedSize)); // return cursor to end of file data to maintain loo^p integrity
     
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
