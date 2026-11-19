# Babarchive — Guide d'utilisation de l'archive

Babarchive regroupe les assets d'un projet (textures, textes, données binaires)
dans un seul fichier `.babar`. Chaque fichier peut être compressé (zlib), puis relu
par son nom, en flux, ou extrait sur le disque.

```cpp
Archive::Packer packer;
packer.Open("Assets/assets.babar");

Archive::FileHeader header;
std::unique_ptr<Archive::FileStream> stream;
packer.ReadFile("BlueMetalPlate/arm", header, stream);
```

---

## 1. Utiliser l'archive : le `Packer`

Toute la manipulation passe par la classe `Packer` (`src/Archive/Packer.h`).

```cpp
namespace Archive
{
    class Packer
    {
    public:
        Error Create(fs::path _location, std::string _name);
        Error Open(fs::path _location);

        Error Add(fs::path _file, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary, std::string const& _key = "");
        Error Add(char* _data, uint64_t _count, std::string const& _name, uint8_t _flags, uint8_t _type = FileType::Binary, std::string const& _key = "");

        Error Remove(std::string_view _name);

        Error ReadFile(std::string const& _name, FileHeader& _header, std::unique_ptr<FileStream>& _stream, std::string const& _key = "");
        Error Unpack(fs::path _unpackLocation, std::unordered_map<std::string, std::string> const& _keyMap = {});
        Error ListInfo(std::vector<FileInfo>& _files);

        void     SetChunkSize(uint32_t _chunkSize);
        uint32_t GetChunkSize() const;
    };
}
```

### Gestion des erreurs

Toutes les méthodes renvoient une `Error` (`src/Core/Error.h`). Elle vaut `true` en cas
d'erreur, et `Format()` donne le message complet, avec le détail de chaque fichier en échec.

```cpp
if (Error e = packer.Add("Test/test.txt", "test", Archive::FileFlags::Compressed))
    std::cout << e.Format() << "\n";
```

Quand une opération porte sur plusieurs fichiers (`Add` d'un dossier, `Unpack`), un fichier
en échec n'arrête pas les autres : les erreurs sont regroupées dans `Error::details`.

### Créer ou ouvrir une archive

```cpp
Archive::Packer packer;

Error e = packer.Create("Test", "archive");     // crée Test/archive.babar
if (e)
    e = packer.Open("Test/archive.babar");      // existe déjà donc on l'ouvre
```

- `Create(_location, _name)` crée le fichier `_location/_name.babar`, contenant seulement le header archive. Il est automatiquement ouvert apres sa création. Si le fichier existe déjà on renvoie une erreur. 
- `Open(_location)` lit le header et la liste des fichiers d'une archive existante. Renvoie
  une erreur si le fichier n'est pas une archive Babar (magic number invalide). Une version
  différente donne seulement un avertissement.

Toutes les autres méthodes demandent une archive ouverte au préalable.

### Ajouter des fichiers

```cpp
// Un fichier du disque
packer.Add("Test/babar.jpg", "image", Archive::FileFlags::Compressed, Archive::FileType::Binary);

// Un dossier du disque (récursif)
packer.Add("Textures/BlueMetalPlate", "BlueMetalPlate", Archive::FileFlags::Compressed);

// Des données déjà en mémoire
packer.Add(buffer, bufferSize, "level01", Archive::FileFlags::None);
```

- `Add(_file, _name, _flags, _type)` : ajoute un fichier ou un dossier du disque.
  - **Fichier** : il est enregistré sous le nom `_name`. L'extension du fichier d'origine
    est conservée à part dans son header, donc `_name` s'écrit **sans extension** !
  - **Dossier** : chaque fichier du dossier et de ses sous-dossiers est ajouté sous le nom `_name/<chemin relatif>`.
    **Exemple** : `Textures/BlueMetalPlate/sub/arm.png` ajouté avec `_name = "BlueMetalPlate"`
    devient `BlueMetalPlate/sub/arm`.
- `Add(_data, _count, _name, _flags, _type)` : ajoute `_count` octets lus depuis un buffer
  mémoire, sans extension. Les données sont copiées en mémoire le temps de l'écriture.

Un nom déjà présent dans l'archive est refusé, et pour un dossier rien n'est écrit si un de
ses noms existe déjà.

Chaque ajout réécrit la liste des fichiers et le header archive : l'archive est toujours
valide sur le disque après un `Add`.

### Retirer un fichier

```cpp
packer.Remove("image");
```

Le header et les données du fichier sont supprimés du fichier `.babar`, qui est compacté :
les fichiers suivants sont décalés et la liste des fichiers est réécrite. L'archive ne
garde pas de trou.

### Lire un fichier

```cpp
Archive::FileHeader header;
std::unique_ptr<Archive::FileStream> stream;

if (Error e = packer.ReadFile("image", header, stream))
    return e;
```

`ReadFile` cherche le fichier par son nom (sans extension), remplit `header` (tailles,
type, flags, CRC, extension) et renvoie un `FileStream` déjà positionné sur ses données.
Voir [Lire un flux](#2-lire-un-flux--filestream).

### Extraire toute l'archive

```cpp
packer.Unpack("Test/Unpack");
```

`Unpack` recrée chaque fichier sous `_unpackLocation/<nom><extension>`, en créant les
sous-dossiers nécessaires. Par exemple `BlueMetalPlate/sub/arm` avec l'extension `.png`
donne `_unpackLocation/BlueMetalPlate/sub/arm.png`. Un fichier sans extension (données
ajoutées depuis la mémoire) reçoit l'extension `.bin`.

Pour chaque fichier, `Unpack` :

- déchiffre les données si le fichier porte le flag `Encrypted`, avec la clé donnée pour son
  nom dans `_keyMap` ;
- décompresse les données si le fichier porte le flag `Compressed` ;
- vérifie la taille et le CRC32 : un fichier corrompu, ou déchiffré avec une mauvaise clé,
  est signalé dans l'erreur renvoyée ;
- ignore, avec une erreur, les noms qui sortiraient du dossier de destination (`../`,
  chemin absolu) et les fichiers chiffrés sans clé dans `_keyMap`.

### Lister le contenu

`ListInfo` remplit un `FileInfo` par fichier : nom, extension, position, taille d'origine
(`dataSize`), taille stockée (`currentSize`), et les booléens `isCompressed` et
`isEncrypted`. Pratique pour savoir quels fichiers demandent une clé avant un `Unpack`.

### Chiffrement

Un fichier ajouté avec une clé non vide (`_key`) est chiffré, avec les deux `Add`. C'est la
clé seule qui décide : le flag `Encrypted` passé à `Add` est ignoré, il est ajouté s'il y a
une clé et retiré sinon.
Il faut redonner la même clé pour le lire (`ReadFile`) ou l'extraire (`Unpack`).
Les données sont compressées puis chiffrées.

### Taille des blocs

`SetChunkSize` règle la taille des blocs utilisés pour écrire et extraire les fichiers
(16 384 octets par défaut). Les fichiers ne sont jamais chargés entièrement en mémoire.

---

## 2. Lire un flux : `FileStream`

`ReadFile` renvoie un `FileStream` (`src/Archive/FileStream.h`), qui se lit par blocs.

```cpp
class FileStream
{
public:
    virtual Error Read(char* _data, uint64_t _count) = 0;
    uint64_t      ReadCount() const;
    virtual bool  IsFinished();
    virtual bool  IsAtEnd();

    uint64_t CursorOffset() const;
    void     SetCursorOffset(uint64_t _cursorOffset);
};
```

| Méthode                 | Rôle                                                                  |
|-------------------------|-----------------------------------------------------------------------|
| `Read(_data, _count)`   | lit au plus `_count` octets (décompressés) dans `_data`, renvoie une `Error` |
| `ReadCount()`           | nombre d'octets réellement lus par le dernier `Read`                  |
| `IsFinished()`          | `true` quand tout le fichier a été lu                                 |
| `IsAtEnd()`             | `true` quand le curseur est à la fin des données                      |
| `CursorOffset()`        | position actuelle dans les données du fichier                         |
| `SetCursorOffset(pos)`  | déplace le curseur (fichiers non compressés uniquement)               |

Boucle de lecture type :

```cpp
Archive::FileHeader header;
std::unique_ptr<Archive::FileStream> stream;
if (Error e = packer.ReadFile("level01", header, stream))
    return e;

char buffer[STREAM_BUFFER_SIZE];
std::string content;

while (stream->IsFinished() == false)
{
    if (Error e = stream->Read(buffer, sizeof(buffer)))
        return e;
    content.append(buffer, stream->ReadCount());
}
```

Il existe deux implémentations de `FileStream`, choisies automatiquement par `ReadFile`
selon les flags du fichier :

- **`UncompressedFileStream`** : renvoie les octets tels qu'ils sont stockés. Ne lit jamais au-delà des données du fichier.
- **`CompressedFileStream`** : décompresse à la volée avec zlib (`inflate`). On ne peut pas se déplacer dans un flux compressé.

Le flux ne vérifie pas le CRC32 automatiquement, seul `Unpack` le fait. Pour le vérifier en lecture, il faut comparer le CRC des octets lus à `header.fixed.crc32`.

---

## 3. Options des fichiers

### Flags fichier (`Archive::FileFlags`)

Les flags se combinent avec `|` et sont stockés sur un octet.

| Flag           | Valeur | Effet                                                    |
|----------------|--------|----------------------------------------------------------|
| `None`         | `0x00` | fichier stocké tel quel                                  |
| `Compressed`   | `0x01` | fichier compressé avec zlib                              |
| `Encrypted`    | `0x02` | fichier chiffré                                          |

### Types de fichier (`Archive::FileType`)

Le type est seulement stocké dans le header du fichier. Il ne change pas la façon dont le
fichier est écrit ou lu. Il est `Binary` par défaut.

| Type     | Valeur | Contenu          |
|----------|--------|------------------|
| `Raw`    | `0`    | octets bruts     |
| `Binary` | `1`    | données binaires |
| `Text`   | `2`    | texte            |

---

## 4. Format du fichier d'archive

Tous les entiers sont en little-endian (ordre natif x86/x64). Les chaînes sont stockées
sans `\0` final, leur longueur est donnée par le champ juste avant.

Vue d'ensemble :

```text
[HEADER ARCHIVE]              <- 24 octets, position 0

[HEADER FICHIER 0]
[données fichier 0]

[HEADER FICHIER 1]
[données fichier 1]
...
[liste des FILE DESCRIPTOR]   <- position donnée par descriptorListOffset, jusqu'à la fin du fichier
```

Pour lire un fichier par son nom :

1. lire le header archive pour trouver `descriptorListOffset` ;
2. lire les File Descriptor les uns après les autres, de `descriptorListOffset` jusqu'à la fin du fichier, jusqu'à trouver `name` ;
3. aller à `offset` pour lire le header fichier, puis les `compressedSize` octets de données qui le suivent.

### Header archive (`BaseHeader`, 24 octets)

| Type   | Champ                | Rôle                                                       |
|--------|----------------------|------------------------------------------------------------|
| uint32 | magicNumber          | `B A B A` (`0x41424142`)                                   |
| uint32 | version              | version du format, actuellement `1`                        |
| uint64 | dataSize             | taille totale des fichiers (headers + données)             |
| uint64 | descriptorListOffset | position de la liste des File Descriptor                   |

### Header fichier (`FileHeader`)

Placé juste avant les données de chaque fichier. La partie fixe fait 24 octets, sans alignement.

| Type   | Champ            | Rôle                                                           |
|--------|------------------|----------------------------------------------------------------|
| uint64 | uncompressedSize | taille d'origine                                               |
| uint64 | compressedSize   | taille des données stockées (égale à `uncompressedSize` sans compression) |
| uint8  | fileType         | type de fichier (`FileType`)                                   |
| uint8  | flags            | flags du fichier (`FileFlags`)                                 |
| uint32 | crc32            | CRC32 des données **décompressées**                            |
| uint16 | extensionLength  | longueur de l'extension                                        |
| string | extension        | extension d'origine en minuscules, avec le point (`.png`, `.txt`), vide pour des données mémoire |

Les données suivent directement le header.
Si le flag `Compressed` est présent, elles forment un flux zlib complet (format zlib avec en-tête, comme produit par `deflateInit`), décodable avec `inflate`.

Avec le flag `Encrypted`, ce sont les données stockées qui sont chiffrées :
Le flux zlib pour un fichier compressé, les données d'origine sinon.
La position utilisée pour le chiffrement est celle de l'octet dans ces données stockées, et `compressedSize` reste leur taille.
Le CRC32 reste celui des données d'origine et non pas des données compressées et/ou chiffrées.

### File Descriptor (`FileDescriptor`)

Un par fichier, dans l'ordre où les fichiers sont rangés dans l'archive.

| Type   | Champ      | Rôle                                                   |
|--------|------------|--------------------------------------------------------|
| uint32 | nameLength | longueur du nom                                        |
| string | name       | nom du fichier dans l'archive, **sans extension**, séparateur `/` |
| uint64 | offset     | position du header fichier depuis le début de l'archive|

La liste n'a pas de nombre d'entrées. Elle s'arrête à la fin du fichier.

---

## 5. Ligne de commande

L'exécutable `Babarchive` est une interface console construite au-dessus de la lib.
La lib `BabarchiveLib` (`src/Archive`, `src/Core`) ne dépend pas de la console (`src/Console`) et peut être utilisée seule dans un autre projet.

```text
Babarchive <commande> <arguments>
```

`<arg>` est un argument obligatoire, `(arg)` est optionnel.

| Commande             | Arguments                                                   | Effet |
|----------------------|-------------------------------------------------------------|-------|
| `--help` / `--h`     | `(commande)`                                                | aide de toutes les commandes, ou d'une seule |
| `--create` / `--c`   | `<dossier> <nom>`                                           | crée l'archive vide `<dossier>/<nom>.babar` |
| `--pack` / `--p`     | `<archive> <fichier \| dossier> <nom> (compresse y/n) (clé)`| ajoute un fichier ou un dossier sous `<nom>` (sans extension). Compressé avec `y` (par défaut `n`), chiffré si une clé est donnée |
| `--unpack` / `--u`   | `<archive> <dossier>`                                       | extrait toute l'archive, demande la clé de chaque fichier chiffré |
| `--remove` / `--r`   | `<archive> <nom>`                                           | retire un fichier |
| `--list` / `--l`     | `<archive>`                                                 | tableau des fichiers : extension, taille, taille stockée, ratio, compressé, chiffré |
| `--read` / `--rd`    | `<archive> <nom> (fichier de sortie)`                       | lit un fichier vers un fichier de sortie, ou vers la sortie standard. Demande la clé s'il est chiffré, et vérifie le CRC32 |
| `--hex` / `--x`      | `<fichier> (octets par ligne)`                              | affiche n'importe quel fichier en hexadécimal (16 octets par ligne par défaut) |

Les clés sont demandées ligne par ligne.
`--read` vers la sortie standard ne contient que les données du fichier.
Si une clé contient des espaces il faut la mettre entre guillemets dans `--pack`.

Le code de retour vaut `0` en cas de succès, et une valeur non nulle sinon (commande inconnue, arguments manquants ou en trop, archive ou fichier introuvable, mauvaise clé…).

Les chemins relatifs partent du dossier du projet (celui du `CMakeLists.txt`), pas du dossier où la commande est lancée.

Exemple complet :

```text
Babarchive --create Test archive
Babarchive --pack Test/archive.babar Test/test.txt test y
Babarchive --pack Test/archive.babar Test/babar.jpg image n maCle
Babarchive --list Test/archive.babar
Babarchive --read Test/archive.babar image Test/image_copie.jpg     (demande la clé)
Babarchive --unpack Test/archive.babar Test/Unpack                  (demande la clé de image)
Babarchive --remove Test/archive.babar test
Babarchive --hex Test/archive.babar
```
