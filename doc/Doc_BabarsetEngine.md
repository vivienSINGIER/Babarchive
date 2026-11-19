# BabarsetEngine — Guide d'utilisation de l'asset engine

BabarsetEngine est une surcouche de Babarchive (voir `Doc_Babarchive.md`). Elle transforme
des fichiers sources (`.bmp`, `.obj`, scènes `.txt`) en données prêtes à l'emploi, les range
dans une archive `.babar`, puis les recharge en mémoire sans aucune conversion.

```cpp
Asset::AssetHandler assets;
Asset::Scene scene;
if (Error e = scene.Load(assets, "Test/DemoScene.babar", "scene"))
    std::cout << e.Format() << "\n";

Asset::MeshView mesh = Asset::GetMesh(scene.m_arena, scene.m_objects[1].mesh);
```

Le principe :

- **A l'import**, chaque fichier source est converti en un bloc binaire : un header fixe
  suivi de ses données (pixels RGBA, sommets et indices…).
- **Au chargement**, ce bloc est copié tel quel dans un buffer (l'*arena*), et on y accède
  par des vues qui pointent directement dans ce buffer.

La bibliothèque `BabarsetEngine` (`src/Asset/`) dépend de `BabarchiveLib`. Tout est dans le namespace `Asset`.

---

## 1. Types d'assets

### `AssetType`

Le type d'un asset est stocké dans le champ `fileType` du header fichier de l'archive.
Les valeurs commencent à `Archive::FileType::UserStart`, pour ne pas se mélanger avec les
types de base de l'archive (`Raw`, `Binary`, `Text`).

| Valeur  | Source   | Contenu stocké                              |
|---------|----------|---------------------------------------------|
| `Image` | `.bmp`   | `ImageHeader` + pixels RGBA8                |
| `Mesh`  | `.obj`   | `MeshHeader` + sommets + indices            |
| `Sound` | —        | `SoundHeader` + échantillons **(Pas implémentée)** |
| `Scene` | `.txt`   | le fichier texte tel quel (voir section 4)  |

Un asset reste un fichier normal de l'archive : on peut toujours le lire en brut avec
`Packer::ReadFile`, le lister avec `--list` ou l'extraire avec `--unpack`.

### Layouts stockés

Chaque asset binaire est un header suivi directement de ses données. Les structures sont
écrites telles qu'elles sont en mémoire (ordre des octets de la machine, pas de conversion).

**Image** (`ImageHeader`, 8 octets)

| Type     | Champ                       | Rôle                                         |
|----------|-----------------------------|----------------------------------------------|
| `uint32` | `width`                     | largeur en pixels                            |
| `uint32` | `height`                    | hauteur en pixels                            |
| `uint8`  | `pixels[width * height * 4]`| RGBA8, ligne par ligne du haut vers le bas  |

**Mesh** (`MeshHeader`, 8 octets)

| Type      | Champ                    | Rôle                                  |
|-----------|--------------------------|---------------------------------------|
| `uint32`  | `vertexCount`            | nombre de sommets uniques             |
| `uint32`  | `indexCount`             | nombre d'indices (3 par triangle)     |
| `Vertex`  | `vertices[vertexCount]`  | sommets, 32 octets chacun             |
| `uint32`  | `indices[indexCount]`    | triangles, indices dans `vertices`    |

```cpp
struct Vertex
{
    float pos[3];
    float normal[3];
    float uv[2];
};
```

**Sound** (`SoundHeader`, 12 octets) **(Pas implémenté)**

| Type     | Champ                                 | Rôle                          |
|----------|---------------------------------------|-------------------------------|
| `uint32` | `sampleRate`                          | fréquence d'échantillonnage   |
| `uint32` | `channelCount`                        | nombre de canaux              |
| `uint32` | `frameCount`                          | nombre de frames              |
| `float`  | `samples[frameCount * channelCount]`  | échantillons entrelacés       |

---

## 2. Ajouter des assets : `Asset::Pack`

```cpp
Error Pack(Archive::Packer& _packer, std::string const& _src, std::string const& _filename, uint8_t _flags = 0);
```

`Pack` choisit l'importeur d'après l'extension du fichier source, puis l'ajoute dans une
archive déjà ouverte sous le nom `_filename`. Les `_flags` sont ceux de l'archive (`Archive::FileFlags::Compressed`, par exemple).

```cpp
Archive::Packer packer;
packer.Create("Test", "DemoScene");

Asset::Pack(packer, "Test/chat.bmp",   "chat",  Archive::FileFlags::Compressed);
Asset::Pack(packer, "Test/singe.obj",  "monke", Archive::FileFlags::Compressed);
Asset::Pack(packer, "Test/scene.txt",  "scene");
```

| Extension | Traitement                                                         |
|-----------|--------------------------------------------------------------------|
| `.bmp`    | `ImportBmp`, puis `Add(data, size)` avec le type `Image`           |
| `.obj`    | `ImportObj`, puis `Add(data, size)` avec le type `Mesh`            |
| `.txt`    | ajouté tel quel avec `Add(path)` et le type `Scene`                |
| autre     | erreur `unsupported asset extension`                               |

`Pack` ne prend qu'un fichier à la fois (pas de dossier) et ne transmet pas de clé :
les assets ajoutés par `Pack` ne sont pas chiffrés.

### Importeurs

Les importeurs convertissent un fichier source en bloc binaire, sans toucher à l'archive.
Ils peuvent être appelés seuls :

```cpp
std::vector<char> data;
if (Error e = Asset::ImportObj("Test/singe.obj", data))
    return e;
packer.Add(data.data(), data.size(), "monke", 0, static_cast<uint8_t>(Asset::AssetType::Mesh));
```

**`ImportBmp`**

- BMP non compressé, 24 ou 32 bits par pixel uniquement ;
- les lignes sont remises de haut en bas;
- BGR(A) est converti en RGBA, l'alpha vaut 255 pour une image 24 bits.

**`ImportObj`**

- lit les lignes `v` (positions), `vt` (UV), `vn` (normales) et `f` (faces), le reste est ignoré ;
- les faces acceptent `v`, `v/vt` et `v/vt/vn`, avec des indices négatifs (`-1` = dernier défini) ;
- les faces de plus de 3 sommets sont découpées en triangles (en éventail depuis le premier sommet) ;
- un sommet est un triplet (position, UV, normale) unique : les triplets déjà vus sont réutilisés ;
- la coordonnée `v` des UV est inversée (`1 - v`), pour une origine en haut à gauche ;
- un fichier sans face renvoie l'erreur `obj has no geometry`.

---

## 3. Charger des assets : `AssetHandler`

```cpp
class AssetHandler
{
public:
    Error Load(std::string const& _archive, std::string const& _filename,
               std::vector<char>& _arena, Handle& _out, std::string const& _key = "");
};
```

`Load` ouvre l'archive `_archive` (une seule fois : les `Packer` ouverts sont gardés en
cache), lit le fichier `_filename` et le copie à la fin de `_arena`. Le fichier est
décompressé et déchiffré au passage par le `FileStream` de l'archive.

```cpp
Asset::AssetHandler assets;
std::vector<char> arena;
Asset::Handle h;
if (Error e = assets.Load("Test/DemoScene.babar", "chat", arena, h))
    return e;
```

- Chaque asset commence à une position multiple de 16 dans l'arena, pour que les headers et les tableaux de `float` soient bien alignés.
- En cas d'erreur, l'arena est remise à sa taille d'avant l'appel.

### `Handle`

```cpp
struct Handle
{
    uint64_t  offset;   // position dans l'arena
    uint64_t  size;     // taille des données
    AssetType type;

    bool IsValid() const;   // size != 0
};
```

Un `Handle` est une position, pas un pointeur : il reste valide même quand l'arena grandit. C'est lui qu'il faut garder.

### Vues

Les vues donnent un accès typé aux données d'un `Handle` :

```cpp
ImageView GetImage(std::vector<char> const& _arena, Handle _h);   // header, pixels
MeshView  GetMesh (std::vector<char> const& _arena, Handle _h);   // header, vertices, indices
SoundView GetSound(std::vector<char> const& _arena, Handle _h);   // header, samples
```

```cpp
Asset::ImageView img = Asset::GetImage(arena, h);
if (img.header)
    std::cout << img.header->width << "x" << img.header->height << "\n";
```

- Si le `Handle` n'a pas le bon type ou ne tient pas dans l'arena, la vue renvoyée est vide (`header == nullptr`)
- Une vue contient des pointeurs dans l'arena. Elle devient donc **invalide dès que l'arena grandit** (nouveau `Load`).
Il faut la redemander à partir du `Handle` après chaque chargement.

---

## 4. Scènes : `Scene`

Une scène est un fichier texte qui liste des ressources et des objets. Elle est stockée dans l'archive avec le type `Scene`.

```cpp
class Scene
{
public:
    Error Load(AssetHandler& _assets, std::string const& _archive, std::string const& _name);
    void  Clear();

    std::vector<char>   m_arena;     // données de toutes les ressources de la scène
    std::vector<Object> m_objects;   // objets, dans l'ordre du fichier
};
```

`Load` vide la scène, lit le fichier `_name` dans `_archive`, le parse, et charge chaque
ressource dans `m_arena`. Une ressource utilisée plusieurs fois (même archive, même nom) n'est chargée qu'une fois.

### Objets

```cpp
struct Transform
{
    float position[3] = { 0, 0, 0 };
    float rotation[4] = { 0, 0, 0, 1 };   // quaternion (x, y, z, w)
    float scale[3]    = { 1, 1, 1 };
};

struct Object
{
    std::string name;
    int32_t     parent = -1;              // index dans m_objects, -1 sans parent
    Transform   transform;                // relatif au parent
    Handle      mesh;
    Handle      texture;
    uint8_t     color[4] = { 255, 255, 255, 255 };
};
```

Un parent est toujours placé avant ses enfants dans `m_objects` : on peut calculer les
transformations globales en un seul parcours.

### Format du fichier scène

```ini
# Commentaire

[ext_resource id=1 type="Image" name="chat"]
[ext_resource id=2 type="Mesh" name="monke"]
[ext_resource id=3 type="Mesh" name="rocher" archive="Test/Autre.babar"]

[object name="Root"]
position = (0.0, 0.0, 0.0)

[object name="Cat" parent="Root"]
position = (1.0, 0.0, 0.0)
rotation = (0.0, 0.7071, 0.0, 0.7071)
scale = (2.0, 2.0, 2.0)
color = (200, 200, 10, 255)
texture = ExtResource(1)
mesh = ExtResource(2)
```

- Une ligne vide ou qui commence par `#` est ignorée.
- Une section est entre `[ ]` : son genre, puis des attributs `cle=valeur` ou `cle="valeur"`.
- Les propriétés `cle = valeur` qui suivent s'appliquent au dernier `[object]`.

**Section `[ext_resource]`** : déclare une ressource, avant tout objet qui l'utilise.

| Attribut  | Obligatoire | Rôle                                                         |
|-----------|-------------|--------------------------------------------------------------|
| `id`      | oui         | entier, utilisé par `ExtResource(id)`                        |
| `type`    | oui         | `Image`, `Mesh` ou `Sound`, vérifié avec le type stocké      |
| `name`    | oui         | nom du fichier dans l'archive                                |
| `archive` | non         | autre archive à utiliser (par défaut, celle de la scène)     |

**Section `[object]`**

| Attribut | Obligatoire | Rôle                                                    |
|----------|-------------|---------------------------------------------------------|
| `name`   | oui         | nom de l'objet                                          |
| `parent` | non         | nom d'un objet déjà défini plus haut                    |

**Propriétés d'un objet**

| Propriété  | Valeur                     | Exemple                         |
|------------|----------------------------|---------------------------------|
| `position` | 3 flottants                | `(1.0, 0.5f, 0.0)`              |
| `rotation` | 4 flottants (quaternion)   | `(0.0, 0.7071, 0.0, 0.7071)`    |
| `scale`    | 3 flottants                | `(2.0, 2.0, 2.0)`               |
| `color`    | 4 entiers de 0 à 255       | `(200, 200, 10, 255)`           |
| `mesh`     | une ressource              | `ExtResource(2)`                |
| `texture`  | une ressource              | `ExtResource(1)`                |

Les nombres sont séparés par des virgules ou des espaces, et le suffixe `f` est accepté.

### Erreurs

Une erreur de scène donne la ligne (et parfois la colonne) en cause :

- `Error::Syntax` : section ou propriété inconnue, `=` manquant, `]` manquant, chaîne non fermée, valeur mal écrite, propriété en dehors d'un `[object]`.
- `Error::Semantics` : ressource qui n'a pas le type annoncé, parent inconnu ;
- `Error::Io` : fichier de scène ou ressource introuvable, fichier qui n'est pas une scène.

---

## 5. Exécutables

### `BabarsetPacker`

```
BabarsetPacker <archive> <fichier> <nom>
```

Ajoute un fichier source dans une archive existante avec `Asset::Pack` (sans flags).
L'archive se crée avec l'outil Babarchive (`--create`).

```
BabarchiveTool --create Test DemoScene
BabarsetPacker Test/DemoScene.babar Test/chat.bmp chat
BabarsetPacker Test/DemoScene.babar Test/singe.obj monke
BabarsetPacker Test/DemoScene.babar Test/scene.txt scene
```

### `BabarsetDemo`

```
BabarsetDemo <archive> <scene>
```

Charge la scène, affiche chaque objet dans la console (parent, transformation, mesh,
texture), puis l'affiche en 3D avec raylib, caméra en orbite.

```
BabarsetDemo Test/DemoScene.babar scene
```

Les fonctions de conversion vers raylib sont dans `src/BabarsetDemo/RLibHelpers.hpp` (`ToTexture`, `ToMesh`, `ToMatrix`)

Les deux exécutables se lancent depuis le dossier du `CMakeLists.txt`, comme `BabarchiveTool`.
