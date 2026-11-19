#include "Commands.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include <zlib.h>

#include "Archive/Packer.h"
#include "Core/Logger.h"
#include "HexViewer/HexViewer.h"

namespace
{
    int Report(Error const& _e)
    {
        Log::Log(LogType::Error, _e.Format() + "\n");
        return _e.Exit();
    }

    int UsageError(std::string const& _message)
    {
        std::cerr << _message << "\n";
        return 1;
    }

    Error OpenArchive(std::string const& _path, Archive::Packer& _packer)
    {
        return _packer.Open(_path);
    }

    std::string AskForKey(std::string const& _name)
    {
        std::cerr << "'" << _name << "' est chiffre. Cle : " << std::flush;
        std::string key;
        std::getline(std::cin, key);
        return key;
    }

    bool FindInfo(std::vector<Archive::FileInfo> const& _files, std::string const& _name, Archive::FileInfo& _out)
    {
        for (Archive::FileInfo const& info : _files)
        {
            if (info.name == _name)
            {
                _out = info;
                return true;
            }
        }
        return false;
    }

    std::string FormatSize(uint64_t _bytes)
    {
        std::ostringstream ss;
        if (_bytes < 1024)
            ss << _bytes << " o";
        else if (_bytes < 1024 * 1024)
            ss << std::fixed << std::setprecision(1) << _bytes / 1024.0 << " Ko";
        else
            ss << std::fixed << std::setprecision(1) << _bytes / (1024.0 * 1024.0) << " Mo";
        return ss.str();
    }
}

int Console::Commands::Create(Args const& _args)
{
    Archive::Packer packer;
    if (Error e = packer.Create(_args[0], _args[1]))
        return Report(e);

    std::cout << "Archive creee : " << (fs::path(_args[0]) / (_args[1] + ".babar")).string() << "\n";
    return 0;
}

int Console::Commands::Pack(Args const& _args)
{
    std::string const compressed = _args.size() > 3 ? _args[3] : "n";
    std::string const key = _args.size() > 4 ? _args[4] : "";

    uint8_t flags = Archive::FileFlags::None;
    if (compressed == "y")
        flags |= Archive::FileFlags::Compressed;
    else if (compressed != "n")
        return UsageError("Argument de compression invalide '" + compressed + "', attendu : y ou n");

    Archive::Packer packer;
    if (Error e = OpenArchive(_args[0], packer))
        return Report(e);

    if (Error e = packer.Add(_args[1], _args[2], flags, Archive::FileType::Binary, key))
        return Report(e);

    std::cout << "Ajoute : " << _args[2]
              << ((flags & Archive::FileFlags::Compressed) ? " (compresse)" : "")
              << (key.empty() ? "" : " (chiffre)") << "\n";
    return 0;
}

int Console::Commands::Unpack(Args const& _args)
{
    Archive::Packer packer;
    if (Error e = OpenArchive(_args[0], packer))
        return Report(e);

    std::vector<Archive::FileInfo> files;
    if (Error e = packer.ListInfo(files))
        return Report(e);

    std::unordered_map<std::string, std::string> keyMap;
    for (Archive::FileInfo const& file : files)
    {
        if (file.isEncrypted)
            keyMap[file.name] = AskForKey(file.name);
    }

    if (Error e = packer.Unpack(_args[1], keyMap))
        return Report(e);

    std::cout << files.size() << " fichier(s) extrait(s) dans " << _args[1] << "\n";
    return 0;
}

int Console::Commands::Remove(Args const& _args)
{
    Archive::Packer packer;
    if (Error e = OpenArchive(_args[0], packer))
        return Report(e);

    if (Error e = packer.Remove(_args[1]))
        return Report(e);

    std::cout << "Retire : " << _args[1] << "\n";
    return 0;
}

int Console::Commands::List(Args const& _args)
{
    Archive::Packer packer;
    if (Error e = OpenArchive(_args[0], packer))
        return Report(e);

    std::vector<Archive::FileInfo> files;
    if (Error e = packer.ListInfo(files))
        return Report(e);

    if (files.empty())
    {
        std::cout << "Archive vide\n";
        return 0;
    }

    size_t nameWidth = 3;
    for (Archive::FileInfo const& file : files)
        nameWidth = std::max(nameWidth, file.name.length());

    std::cout << std::left
              << std::setw(nameWidth + 2) << "Nom"
              << std::setw(8)  << "Ext"
              << std::setw(12) << "Taille"
              << std::setw(12) << "Stockee"
              << std::setw(8)  << "Ratio"
              << std::setw(10) << "Compresse"
              << "Chiffre\n";

    uint64_t total = 0, totalStored = 0;
    for (Archive::FileInfo const& file : files)
    {
        std::ostringstream ratio;
        if (file.dataSize > 0)
            ratio << (file.currentSize * 100 / file.dataSize) << " %";
        else
            ratio << "-";

        std::cout << std::setw(nameWidth + 2) << file.name
                  << std::setw(8)  << (file.extension.empty() ? "-" : file.extension)
                  << std::setw(12) << FormatSize(file.dataSize)
                  << std::setw(12) << FormatSize(file.currentSize)
                  << std::setw(8)  << ratio.str()
                  << std::setw(10) << (file.isCompressed ? "oui" : "non")
                  << (file.isEncrypted ? "oui" : "non") << "\n";

        total += file.dataSize;
        totalStored += file.currentSize;
    }

    std::cout << files.size() << " fichier(s), " << FormatSize(total) << " (" << FormatSize(totalStored) << " stockes)\n";
    return 0;
}

int Console::Commands::Read(Args const& _args)
{
    Archive::Packer packer;
    if (Error e = OpenArchive(_args[0], packer))
        return Report(e);

    std::vector<Archive::FileInfo> files;
    if (Error e = packer.ListInfo(files))
        return Report(e);

    Archive::FileInfo info;
    if (FindInfo(files, _args[1], info) == false)
        return Report(Error::Io("fichier '" + _args[1] + "' introuvable dans l'archive", _args[0]));

    std::string const key = info.isEncrypted ? AskForKey(info.name) : "";

    Archive::FileHeader header;
    std::unique_ptr<Archive::FileStream> stream;
    if (Error e = packer.ReadFile(_args[1], header, stream, key))
        return Report(e);

    std::ofstream out(_args[2], std::ios::out | std::ios::binary | std::ios::trunc);
    if (out.is_open() == false)
        return Report(Error::Io("impossible de creer le fichier de sortie", _args[2]));

    char buffer[STREAM_BUFFER_SIZE];
    uLong crc = crc32(0L, Z_NULL, 0);
    Error readError;

    while (stream->IsFinished() == false)
    {
        if (Error e = stream->Read(buffer, sizeof(buffer)))
        {
            readError = e;
            break;
        }
        crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer), (uInt)stream->ReadCount());
        out.write(buffer, (std::streamsize)stream->ReadCount());
    }

    out.flush();

    if (!readError && (uint32_t)crc != header.fixed.crc32)
        readError = Error::Io("CRC invalide pour '" + _args[1] + "' : mauvaise cle ou archive corrompue", _args[0]);

    if (!readError && !out)
        readError = Error::Io("erreur d'ecriture de la sortie");

    if (readError)
    {
        out.close();
        FileHelper::RemoveFile(_args[2]);
        return Report(readError);
    }

    std::cout << "Lu : " << _args[1] << " -> " << _args[2] << " (" << FormatSize(header.fixed.uncompressedSize) << ")\n";

    return 0;
}

int Console::Commands::Hex(Args const& _args)
{
    uint32_t width = 16;
    if (_args.size() > 1)
    {
        try
        {
            size_t used = 0;
            unsigned long value = std::stoul(_args[1], &used);

            if (used != _args[1].size() || value == 0 || value > 256)
                throw std::out_of_range("width");

            width = (uint32_t)value;
        }
        catch (std::exception const&)
        {
            return UsageError("Nombre d'octets par ligne invalide '" + _args[1] + "', attendu : 1 a 256");
        }
    }

    HexViewer viewer;

    if (Error e = viewer.Init(_args[0]))
        return Report(e);

    viewer.Display(width);
    return 0;
}
