# Babarchive — Guide d'utilisation de l'archive

Babarchive regroupe les assets d'un projet (textures, textes, données binaires)
dans un seul fichier. Les fichiers peuvent être compressés, puis relus par leur nom,
sous forme brute.

```text
Babarchive Read BlueMetalPlate/arm.png
```

---

## 1. Utiliser l'archive : le Packer

Toute la manipulation passe par la classe `Archive`.

```cpp
class Archive
{
    Create(path /* dossier ou fichier */, flags);

    Add(path /* dossier ou fichier */, flags);
    Add(const char* _data, uint32 _count);

    Remove(fileName);

    FileStream Read(name);
};
```

### Créer une archive

```cpp
Archive archive = Archive::Create("Assets/", Flags::Compressed);
```

`Create` construit une archive à partir d'un **dossier** (tout son contenu est ajouté) ou d'un
**fichier** unique. Les `flags` choisissent le traitement appliqué aux fichiers (voir
[Flags](#flags-fichier)).

### Ajouter des fichiers

```cpp
archive.Add("Textures/BlueMetalPlate/", Flags::Compressed);   // dossier ou fichier du disque
archive.Add(buffer, bufferSize);                              // données déjà en mémoire
```

- `Add(path, flags)` : ajoute un fichier ou un dossier du disque, avec les flags voulus.
- `Add(const char* _data, uint32 _count)` : ajoute `_count` octets lus depuis un buffer mémoire.

### Retirer un fichier

```cpp
archive.Remove("BlueMetalPlate/arm.png");
```

### Lire un fichier

Deux façons de lire, selon le besoin :

```cpp
// Lecture brute 
FileStream stream = archive.Read("BlueMetalPlate/arm.png");
```

---

## 2. Lire un flux : `FileStream`

`Read` renvoie un `FileStream`, qui se lit par blocs.

```cpp
class FileStream
{
    Error  Read(char* buff, _buffSize);
    uint32 gcount();
    bool   IsAtEnd();

    uint32 m_lastReadSize;
};
```

| Méthode                   | Rôle                                                       |
|---------------------------|------------------------------------------------------------|
| `Read(buff, _buffSize)`   | lit au plus `_buffSize` octets dans `buff`, renvoie une `Error` |
| `gcount()`                | nombre d'octets réellement lus par le dernier `Read` (`m_lastReadSize`) |
| `IsAtEnd()`               | `true` quand tout le fichier a été lu                      |

Boucle de lecture type :

```cpp
FileStream stream = archive.Read("Data/level01.bin");
char buffer[4096];

while (stream.IsAtEnd() == false)
{
    Error err = stream.Read(buffer, sizeof(buffer));
    // vérifier err, puis traiter stream.gcount() octets de buffer
}
```

Il existe deux implémentations de `FileStream`, choisies automatiquement selon les flags du fichier :

- **Uncompressed File Stream** : renvoie les octets tels qu'ils sont stockés.
- **Compressed File Stream** : décompresse à la volée (*avec zstream*).

---

## 3. Options des fichiers

### Flags fichier

| Flag         | Effet                                     |
|--------------|-------------------------------------------|
| `None`       | fichier stocké en raw                     |
| `Compressed` | fichier compressé dans l'archive          |

### Types de fichier 

| Type     | Contenu                                           |
|----------|---------------------------------------------------|
| `Raw`    | octets brutes                                     |
| `Binary` | données binaires                                  |
| `Text`   | texte                                             |

---

## 4. Format du fichier d'archive

Vue d'ensemble :

```text
[HEADER ARCHIVE]

[HEADER FICHIER 0]
[données fichier 0]

[HEADER FICHIER 1]
[données fichier 1]
...
[liste des FILE DESCRIPTOR]   <- position donnée par descriptorListOffset
```

Pour lire un fichier par son nom :

1. lire le header archive pour trouver `descriptorListOffset` ;
2. parcourir les File Descriptor jusqu'à trouver `name` ;
3. aller à `offset` pour lire le header fichier, puis les données.

### Header archive

| Type   | Champ                | Rôle                                   |
|--------|----------------------|----------------------------------------|
| uint16 | magicNumber          | `0xBABA` : identifie une archive Babar |
| uint16 | version              | version du format                      |
| uint64 | archiveSize          | taille totale de l'archive             |
| uint64 | descriptorListOffset | position de la liste des File Descriptor |

### File Descriptor

Un par fichier.

| Type   | Champ      | Rôle                                   |
|--------|------------|----------------------------------------|
| uint32 | nameLength | longueur du nom                        |
| string | name       | nom du fichier dans l'archive          |
| uint32 | offset     | position du header fichier             |

### Header fichier

| Type   | Champ            | Rôle                                   |
|--------|------------------|----------------------------------------|
| uint64 | uncompressedSize | taille d'origine                       |
| uint64 | compressedSize   | taille stockée dans l'archive          |
| uint8  | fileType         | type de fichier                        |
| uint16 | extensionLength  | longueur de l'extension                |
| string | extension        | extension d'origine (`png`, `txt`…)    |
| uint16 | flags            | flags du fichier                       |
| uint64 | crc32            | vérification corruption                |
