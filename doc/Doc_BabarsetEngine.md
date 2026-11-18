# BabarsetEngine — Guide d'utilisation de l'asset engine en lien avec Babarchive

BabarsetEngine est la surcouche par défaut de Babarchive.
Elle permet le stockage et l'extraction de données à l'aide de type prédéfinis.
Ce qui facilite la lecture des données et l'utilisation qui suit.

---

## Couplage avec l'archive

La surcouche ajoute de nouvelles méthodes pour pouvoir lire et écrire en fonction du type précisé.

### Ajouter un fichier

Avec un dossier ou fichier du disque

```cpp
Add("Textures/BlueMetalPlate/", FileType::Image, Flags::Compressed);   
       chemin du fichier      |      type      |      flags
```

Avec les données déjà en mémoire

```cpp
Add(headerData, headerSize);   // Etape 1 -> le header;
Add(data, dataSize);           // Etape 2 -> la data du fichier;
```

### Lire un fichier

Il est maintenant possible de lire la data directement dans un type.

```cpp
Image texture = Read<Image>("BlueMetalPlate/arm.png");
```

`Read<T>` s'appuie sur le **type de fichier** 

Ces méthodes servent à expliciter le type voulu et de rangé la data de la même manière.
Elles passent toutes par les méthodes par défaut de l'archive.
Un fichier ajouté avec un type peut toujours être lu en brut par l'archive sans passer obligatoirement par la surcouche.

## Exemple de data typée

### Image 

| Type   | Champ                 | Rôle                                  |
|--------|-----------------------|---------------------------------------|
| uint32 | width                 | largeur                               |
| uint32 | height                | hauteur                               |
| uint8  | mipCount              | nombre de niveaux de mipmap           |
| uint32 | mipOffsets[mipCount]  | position de chaque niveau de mipmap   |
