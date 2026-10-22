# My_Malloc

Implémentation d'un allocateur mémoire en C99 (`malloc`, `free`, `calloc`, `realloc`), livrée sous forme de bibliothèque partagée `libmalloc.so` qui remplace celle de la libc via `LD_PRELOAD`.

Projet réalisé pour comprendre de l'intérieur comment fonctionne la gestion de la mémoire dynamique : appels système, métadonnées, fragmentation, alignement.

## Points clés

- **Algorithme** : first fit sur listes chaînées, avec découpe (*split*) et fusion (*merge*) des blocs.
- **Mémoire obtenue du noyau par pages** (`mmap`), et rendue (`munmap`) dès qu'une page ne contient plus aucun bloc utilisé.
- **Alignement sur 16 octets** (`sizeof(long double)`) pour chaque pointeur renvoyé.
- **Détection des overflows** avec les builtins GCC (`__builtin_mul_overflow`, `__builtin_add_overflow`).
- **Seuls 4 symboles exportés** (`malloc`, `free`, `calloc`, `realloc`), le reste est masqué avec `-fvisibility=hidden`.
- **`realloc` étendu** : agrandissement sur place quand le bloc suivant est libre, sans copie.
- **Testé sur de vrais programmes** (`ls`, `cat`, `factor`, `git`, `ip`) via `LD_PRELOAD`.

## Compilation et utilisation

```sh
make library        # produit libmalloc.so
make check          # tests unitaires + tests sur des programmes réels
make clean
```

Utilisation sur un programme existant, sans le recompiler :

```sh
LD_PRELOAD=./libmalloc.so ls -la
LD_PRELOAD=./libmalloc.so cat Makefile
```

Ou lié à un programme que l'on compile soi-même :

```sh
gcc main.c -L. -lmalloc -o main
LD_LIBRARY_PATH=. ./main
```

Flags de compilation : `-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -fvisibility=hidden -fPIC`, édition de liens avec `-shared -Wl,--no-undefined`.

Vérifier les symboles exportés :

```sh
nm -C -D libmalloc.so | grep ' T '
```

## Architecture

```
malloc/
├── Makefile
├── src/
│   ├── alignment/
│   │   └── alignment.c      # align() : arrondi au multiple de 16, détecte l'overflow
│   ├── malloc.h             # structures et prototypes my_*
│   ├── wrappers.c           # implémentation (my_malloc, my_free, my_calloc, my_realloc)
│   └── malloc.c             # fonctions exportées, simples relais vers my_*
└── tests/
    ├── test_lib.c           # test lié à libmalloc.so
    └── tests.sh           # essais sur des programmes réels
```

Les fonctions publiques ne contiennent aucune logique : elles appellent des fonctions préfixées `my_`.

## Modèle de mémoire

L'allocateur gère deux niveaux de listes chaînées.

```
pages ──▶ [ page C ] ──▶ [ page B ] ──▶ [ page A ] ──▶ NULL
              │
              ▼
   [ en-tête page ][ meta | données ][ meta | libre ][ meta | données ] ...
```

- **Liste de pages** : une `struct page` par appel à `mmap`. Les pages ne sont pas forcément voisines en mémoire.
- **Liste de blocs** : à l'intérieur d'une page, les blocs sont contigus, chacun précédé de sa métadonnée, et couvrent toute la page.

```c
struct page
{
    struct page *next;       /* page suivante */
    size_t size;             /* taille totale mappée (argument de munmap) */
    struct blk_meta *first;  /* premier bloc de la page */
    size_t pad;              /* garde sizeof multiple de 16 */
};

struct blk_meta
{
    size_t size;             /* taille utilisable (sans la métadonnée) */
    size_t free;             /* 1 = libre, 0 = utilisé */
    struct blk_meta *next;
    struct blk_meta *prev;
    char data[];             /* début de la zone utilisateur */
};
```

`sizeof(struct page)` et `sizeof(struct blk_meta)` valent 32 octets, des multiples de 16 : `data` est donc toujours aligné.

Invariant maintenu à chaque opération : la somme des tailles des blocs d'une page, plus une métadonnée par bloc, plus l'en-tête de page, vaut exactement `page->size`.

## Fonctionnement

### `malloc(size)`

1. Alignement de `size` sur 16 (`0` en cas d'overflow, renvoie `NULL`).
2. **First fit** : parcours de toutes les pages et de tous leurs blocs, premier bloc libre de taille suffisante.
3. Si aucun ne convient, `mmap` d'une nouvelle page, arrondie à un nombre entier de pages système (`sysconf(_SC_PAGESIZE)`), contenant un unique bloc libre.
4. **Split** : le bloc est coupé à la taille demandée, le reste devient un nouveau bloc libre. On ne coupe que si le reste peut contenir une métadonnée plus 16 octets.
5. Le bloc est marqué utilisé, et on renvoie l'adresse de ses données.

Une grosse allocation (plus d'une page) produit un seul `mmap` contigu et une seule `struct page`, pas une chaîne de pages.

### `free(ptr)`

1. `ptr == NULL` : rien à faire.
2. Retrouver la métadonnée (`ptr` reculé de `sizeof(struct blk_meta)`).
3. Marquer le bloc libre.
4. **Fusion** avec le suivant s'il est libre, puis avec le précédent s'il est libre (c'est alors le précédent qui absorbe le bloc).
5. Si le bloc obtenu est seul dans sa page (plus de `prev` ni de `next`), la page est retirée de la liste puis rendue au noyau avec `munmap`.

Les pointeurs qui ne se trouvent dans aucune page de l'allocateur sont ignorés. C'est nécessaire en pratique : certains programmes, comme `cat`, obtiennent de la mémoire via `aligned_alloc` de la libc, puis la passent à `free`.

### `calloc(nmemb, size)`

Le produit `nmemb * size` est calculé avec `__builtin_mul_overflow` (`NULL` en cas de débordement), puis `malloc`, puis `memset` à 0. Le `memset` est indispensable : un bloc recyclé contient d'anciennes données, contrairement à une page neuve.

### `realloc(ptr, size)`

Les cas sont traités du moins coûteux au plus coûteux :

1. `ptr == NULL` : équivalent à `malloc`.
2. `size == 0` : équivalent à `free`, renvoie `NULL`.
3. Le bloc est déjà assez grand : on le coupe pour rendre l'excédent, même pointeur.
4. Le bloc suivant est libre et l'ensemble suffit : **extension sur place** (fusion puis découpe), même pointeur, aucune copie.
5. Sinon : nouvelle allocation, `memcpy` de l'ancienne taille, libération de l'ancien bloc. Si la nouvelle allocation échoue, l'ancien bloc reste valide.

## Choix de conception

| Choix | Raison |
|---|---|
| Une `struct page` par `mmap` | Retrouver la taille à `munmap` et libérer une page entière dès qu'elle est vide |
| `prev` dans chaque bloc | Fusion avec le bloc précédent en temps constant |
| `data[]` en tableau flexible | Adresse des données directement après la métadonnée, alignement garanti |
| Aucun `printf` dans l'allocateur | `stdio` appelle `malloc` : récursion infinie avec `LD_PRELOAD` |
| Avancer en octets via `char *` | Arithmétique de pointeurs standard, sans conversion explicite |

## Tests

- **`tests/test_malloc.c`** : 50 tests Criterion sur `my_malloc`, `my_free`, `my_calloc`, `my_realloc` : alignement, absence de chevauchement, réutilisation après `free`, fusions dans tous les ordres, plusieurs pages, overflows, `realloc` sur place et avec déplacement, tests de charge aléatoires.
- **`tests/test_lib.c`** : test suite, lié à `libmalloc.so`, qui appelle les `malloc`, `free`, `calloc` et `realloc` (compilé avec `-fno-builtin` pour que GCC ne supprime pas les appels).
- **`tests/test.sh`** : lance `factor`, `cat`, `ls`, `git status`... avec `LD_PRELOAD=./libmalloc.so` et vérifie que la bibliothèque est bien celle qui répond.

Compilation conseillée avec sanitizers pour la logique :

```sh
gcc -g -fsanitize=address,undefined -D_DEFAULT_SOURCE -std=c99 \
    tests/test_malloc.c src/wrappers.c src/alignment/alignment.c \
    -o test_suite_asan -lcriterion
```

## Limites connues et pistes d'amélioration

- **Pas thread-safe** : une version sûre demande un `pthread_mutex_t` autour de chaque opération.
- **Parcours linéaire** : le first fit traverse tous les blocs, utilisés compris. Une *free-list* des seuls blocs libres, rangés par classe de taille, accélérerait fortement les allocations.
- **Métadonnées volumineuses** : 32 octets par bloc, dont un `size_t` pour un simple booléen. Le statut peut être stocké dans le bit de poids faible de `size` (les tailles sont multiples de 16).
- **Métadonnées voisines des données** : un dépassement de tampon utilisateur les corrompt. Les séparer rendrait l'allocateur résistant à ce cas.
- **`aligned_alloc`, `posix_memalign`, `memalign`** non fournis : leurs pointeurs, issus de la libc, sont ignorés par `free` mais jamais libérés.
- **`realloc` n'étend que vers l'avant** : étendre vers le bloc précédent change l'adresse et imposerait un `memmove`.
- **Vérification de pointeur coûteuse** : tester l'appartenance à une page est linéaire en nombre de pages.

## Ce que ce projet m'a appris

- Le rôle réel d'`mmap`/`munmap` et la contrainte du travail en pages entières.
- L'organisation mémoire d'un bloc : métadonnée, zone utilisateur, alignement, padding.
- La manipulation rigoureuse de listes chaînées doublement liées (insertion, retrait, fusion) sans corrompre les voisins.
- Le fonctionnement de l'interposition de symboles (`LD_PRELOAD`, visibilité des symboles, éditeur de liens dynamique).
- Le débogage avec `gdb`, `nm`, `ldd`, `LD_DEBUG` et les sanitizers.

## Auteur

[chocapic78560](https://github.com/chocapic78560)
