---
title: Manuel du langage BASTOS
---

![bastos](bastos-title.png)

# Manuel du langage BASTOS

## Présentation

### Matériel

BASTOS-S est un ordinateur composé de deux éléments reliés par un câble série DIN-5 :

- **Terminal Minitel** (Minitel 1B, Minitel 2 ou Magis Club) : fournit l'écran Vidéotex (40 colonnes × 25 lignes avec affichage semi-graphique), le clavier AZERTY avec touches de fonction, et l'alimentation électrique via la prise péri-informatique.

- **Module SonOff Basic** (R2, R3 ou R4) équipé d'un microcontrôleur esp8266 ou esp32c (ESP32-C3) :
  - CPU RISC-V 160 MHz (R4) ou Tensilica Xtensa 80 MHz (R2/R3)
  - RAM : ~56 KB pour BASTOS sur R4, ~32 KB sur R2/R3 (variables, base de données, programme)
  - Disque local : ~1,4 MB sur R4, ~512 KB sur R2/R3 (LittleFS sur Flash) pour les programmes (.bas), les variables sauvegardées et la base de données
  - WiFi 802.11 b/g/n pour la connexion Internet
  - Port série : 1200 et 4800 bps (Minitel 1B), jusqu'à 9600 bps (Minitel 2 et Magis Club)

Il est possible de développer et tester des programmes BASTOS sur PC (Windows/Linux) sans matériel Minitel, via un émulateur web (minterm) connecté à un interpréteur BASTOS local exposé par WebSocket. Voir le [Guide de développement](https://abasty.github.io/minwifi-esp01/BASTOS-DEV-GUIDE-fr). Les programmes ainsi développés peuvent être transférés sur BASTOS-S par FTP.

### Le langage BASTOS

BASTOS est un dialecte BASIC conçu spécifiquement pour fonctionner sur terminal Minitel via liaison série. Les programmes sont composés de lignes numérotées exécutées dans l'ordre ; la ligne 0, ou une ligne sans numéro, est interprétée immédiatement (mode interactif).

```basic
10 PRINT "Bonjour !"
20 PAUSE 1000
30 GOTO 10
```

Le langage offre les capacités suivantes :

- **Contrôle du Minitel** : affichage Vidéotex, positionnement curseur, attributs (couleurs, taille, clignotement), semi-graphique
- **Stockage local** : sauvegarde et chargement de programmes et variables sur le disque local (LittleFS)
- **Base de données** : stockage clé/valeur persistant avec les commandes `GET`, `PUT`, `DB`
- **Connectivité Internet** :
  - Connexion WiFi (commande `WIFI`)
  - Accès aux serveurs Minitel via TCP ou WebSockets (commande `MINITEL`)
  - Transfert de fichiers via FTP (commande `FTP`)
- **Fonctions mathématiques** : trigonométrie, logarithmes, racine carrée, aléatoire
- **Tableaux** : variables dimensionnées avec `DIM`
- **Structures de contrôle** : boucles `FOR`/`NEXT` et `WHILE`/`WEND`, branchements `IF`/`THEN`/`ELSE`, `GOTO`, `GOSUB`/`RETURN`, `LABEL`s nommés

---

## Modes de BASTOS

BASTOS dispose de trois modes opérationnels :

- **Mode interactif** : Les lignes entrées sont interprétées immédiatement. Si
  une ligne possède un numéro, elle est enregistrée dans le programme. C'est le
  mode d'entrée des commandes et d'édition des lignes de programme.

- **Mode exécution** : Un programme est en train de s'exécuter (lancé avec `RUN`
  ou `GOTO`). Le clavier peut être lu avec `INPUT`, `VKEY` et `INKEY$`. L'écran
  est contrôlé par `PRINT` et les commandes TTY. Appuyer deux fois sur ESC permet
  de sortir du mode exécution et de revenir au mode interactif.

- **Mode connecté** : BASTOS est connecté à un serveur via la commande `MINITEL`.
  Les entrées clavier sont envoyées au serveur, et l'écran affiche la réponse du
  serveur. Appuyer deux fois sur ESC permet de sortir du mode connecté et de
  revenir au mode précédent (soit exécution, soit interactif).

```mermaid
flowchart TD
    start([Démarrage])
    interactif["Mode interactif"]
    execution["Mode exécution"]
    connecte["Mode connecté"]

    start --> interactif
    interactif --> |RUN/GOTO| execution
    interactif -->|MINITEL| connecte
    execution -->|ESC ESC| interactif
    execution -->|MINITEL| connecte
    connecte -->|ESC ESC| execution
    connecte -->|ESC ESC| interactif
    linkStyle default stroke:#3f3,stroke-width:2px,color:green;
```

Au démarrage, si un fichier `autoexec.bas` existe sur le disque local, la
commande `RUN "autoexec.bas"` est **automatiquement** exécutée.

Exemple de programme `autoexec.bas` :

```basic
10 RUN "connect.bas"
```

### Édition de ligne (mode interactif)

En mode interactif, chaque ligne tapée peut être éditée avant d'être validée :

- **◄ / ►** (flèches gauche/droite) : déplacent le curseur sur la ligne en
  cours de saisie, sans rien effacer.
- **CORRECTION** (touche 127) : efface le caractère situé avant le curseur.
- **ANNULATION** (touche 1) : efface toute la ligne en cours de saisie.
- **▲** (flèche haut) : rappelle la dernière ligne validée pour l'éditer à
  nouveau — la dernière ligne numérotée du programme (comme le ferait `EDIT
  numligne`), ou la dernière commande immédiate tapée si aucune ligne
  numérotée n'a été validée depuis. Ne fait rien si aucune ligne n'a encore
  été validée, ou si la ligne numérotée rappelée a depuis été supprimée du
  programme.
- **Validation** (ENVOI, REPETITION, SUITE, RETOUR, SOMMAIRE ou GUIDE) :
  soumet la ligne. Si elle contient une erreur de syntaxe, BASTOS émet un
  bip (caractère BEL) et affiche l'erreur, mais **reste en mode édition**
  avec le texte tapé conservé, prêt à être corrigé et validé à nouveau — la
  ligne n'est jamais perdue ni silencieusement rejetée.
  - **SUITE** valide comme ENVOI, mais si la ligne validée est une ligne
    numérotée du programme, charge en plus automatiquement la ligne
    **suivante** du programme pour l'éditer, si elle existe.
  - **RETOUR** valide comme ENVOI, mais charge automatiquement la ligne
    **précédente** du programme pour l'éditer, si elle existe. SUITE et
    RETOUR permettent ainsi de parcourir et corriger une suite de lignes
    sans retaper `EDIT` à chaque fois.
- **ESC ESC** (deux appuis consécutifs) : abandonne la ligne en cours de
  saisie sans la valider. Si elle avait été rappelée avec `EDIT` ou la
  flèche haut puis modifiée, la ligne d'origine du programme reste
  inchangée, même après une tentative de validation en erreur.

| État de départ | Touche(s) | Effet | État d'arrivée |
|---|---|---|---|
| Ligne vide | Caractère, ◄ ou ► | insertion ou déplacement du curseur | Édition en cours |
| Édition en cours | Caractère, ◄, ► ou CORRECTION | modification de la ligne | Édition en cours |
| Édition en cours | ANNULATION | efface toute la ligne | Ligne vide |
| Ligne vide | ▲ | rappelle la dernière ligne validée | Édition en cours |
| Édition en cours | Validation, syntaxe correcte | la ligne est stockée | Ligne vide |
| Édition en cours | SUITE, syntaxe correcte, ligne suivante existante | la ligne est stockée, la ligne suivante est chargée | Édition en cours |
| Édition en cours | RETOUR, syntaxe correcte, ligne précédente existante | la ligne est stockée, la ligne précédente est chargée | Édition en cours |
| Édition en cours | Validation, erreur de syntaxe | bip + message d'erreur | Erreur (reste en édition) |
| Erreur (reste en édition) | Correction, puis Validation | la ligne corrigée est stockée | Ligne vide |
| Erreur (reste en édition) | ESC ESC | abandon de la ligne | Ligne vide |

La commande `EDIT [numligne]` a un effet proche de la flèche haut, mais
permet de cibler explicitement une ligne du programme :

- `EDIT` seul, ou `EDIT 0`, édite la **première** ligne du programme. Ne
  fait rien si le programme est vide.
- `EDIT numligne` édite la ligne `numligne` si elle existe, sinon la
  première ligne **suivante** existante (même logique que `GOTO`). Ne fait
  rien si aucune ligne ne correspond (programme vide, ou `numligne`
  au-delà de la dernière ligne).

---

## Commandes du programme

| Commande | Description |
|----------|-------------|
| `RUN` | Exécuter depuis la première ligne |
| `RUN numligne` | Exécuter depuis une ligne précise |
| `RUN "fichier.bas"` | Charger et exécuter un programme ASCII |
| `RUN "fichier.bst", numligne` | Charger et exécuter un programme binaire et ses variables depuis une ligne |
| `LIST` | Lister 20 lignes depuis la position courante |
| `LIST numligne` | Lister 20 lignes depuis `numligne` |
| `LIST numligne, nombre` | Lister `nombre` lignes depuis `numligne` |
| `LL` | Identique à `LIST` |
| `EDIT [numligne]` | Rappeler une ligne du programme pour l'éditer (voir [Édition de ligne](#édition-de-ligne-mode-interactif)) |
| `NEW` | Supprimer toutes les lignes du programme et les variables |
| `CLEAR` | Effacer les variables et stopper l'exécution |
| `END` | Terminer le programme et effacer les variables |
| `STOP` | Suspendre l'exécution |
| `CONT` | Reprendre après `STOP` |
| `SAVE "fichier.bas"` | Sauvegarder le programme en ASCII |
| `SAVE "fichier.bst"` | Sauvegarder le programme et les variables en binaire |
| `SAVE "fichier.var"` | Sauvegarder les variables uniquement |
| `LOAD "fichier.bas"` | Charger un programme ASCII |
| `LOAD "fichier.bst"` | Charger le programme et les variables depuis un fichier binaire |
| `LOAD "fichier.var"` | Charger les variables uniquement |
| `ERASE "fichier"` | Supprimer un fichier |
| `CAT` | Lister les fichiers locaux |
| `CAT "motif"` | Lister les fichiers locaux dont le nom correspond à un motif (`*`, `?`), ex. `CAT "*.bas"` |
| `FREE` | Afficher l'utilisation mémoire |
| `RESET` | Réinitialiser le système |
| `BASTOS` | Afficher la version et réinitialiser les attributs écran par défaut |

---

## Entrées / Sorties

### Sortie vers écran

La commande `PRINT` affiche des expressions à l'écran, suivies d'un saut de
ligne. `?` est un raccourci pour `PRINT`.

```basic
PRINT expr [, expr ...]      ' Espace entre les éléments
PRINT expr ; expr            ' Pas d'espace entre les éléments
PRINT                        ' Afficher une ligne vide
? expr                       ' Identique à PRINT expr
```

Un `;` en fin de ligne supprime le saut de ligne final :

```basic
10 PRINT "Entrez une valeur : ";
20 INPUT n
```

Les expressions numériques et les chaînes peuvent être mélangées librement :

```basic
PRINT "Résultat : "; a * 2
PRINT "Nom : " n$ ", âge : " a
```

Positionner l'affichage avec `AT ligne, col` :

```basic
AT 5, 10; "Bonjour"
```

### Contrôle de l'écran du Minitel

L'écran dispose de deux modes d'affichage : **Vidéotex (40 colonnes)** et
**Téléinformatique (80 colonnes)**. La ligne 0 est une ligne d'état ; la zone
d'affichage comporte 24 lignes en dessous. L'accès à la ligne 0 avec `LINE0`
(équivalent à `AT 0,1`) sauvegarde la position courante du curseur et les
attributs. Pour quitter la ligne 0 et retourner à la zone d'affichage, utiliser
`AT` ou `"\n"` (saut de ligne) ; `"\n"` restaure à la fois la position
sauvegardée et les attributs.

Les caractères proviennent de deux jeux : **G0 (ASCII)** pour le texte régulier,
et **G1 (semi-graphiques)** pour les graphiques à base de pixels. En G0, les
attributs sont soit locaux (INK, INVERSE, FLASH, SIZE) soit globaux (PAPER,
UNDERLINE) ; les attributs globaux doivent être précédés d'un séparateur espace.
En G1, tous les attributs sont locaux et ne nécessitent pas de séparateur ;
cependant, SIZE et INVERSE ne sont pas supportés, et UNDERLINE gère les
semi-graphiques disjoints.

Les fonctions TTY renvoient une chaîne contenant la séquence d'échappement
correspondante. Utilisées comme instruction, elles se comportent comme `PRINT
...; ` (émettent la séquence sans saut de ligne final). Utilisées comme
expression, elles peuvent être affectées ou intégrées dans une chaîne :

```basic
CLS                      ' instruction : envoie la séquence d'effacement écran
c$ = CLS                 ' expression : stocke la séquence dans c$
PRINT INK 3 "bonjour"    ' en ligne : changer la couleur puis afficher
m$ = AT 10, 13           ' construire une chaîne avec positionnement
m$ = m$ + "hi"
```

En mode Videotex (MODE 0 ou MODE 1), les fonctions TTY émettent des séquences
Vidéotex. En mode téléinformatique (MODE 2), elles émettent des séquences CSI
(`ESC [ ...`).

#### Écran

```basic
CLS                  ' Effacer l'écran
CLEOL                ' Effacer jusqu'à la fin de ligne
CURSOR n             ' 0=masquer, 1=afficher le curseur
BEEP                 ' Émettre un bip
MODE n               ' Mode écran : 0/1 = 40 cols Videotex, ≥2 = 80 cols téléinformatique
LINE0                ' Placer le curseur en ligne 0, colonne 1 (ligne d'état)
ECHO n               ' 0=écho désactivé, 1=écho activé
G0                   ' Passer au jeu de caractères ASCII
G1                   ' Passer au jeu de caractères semi-graphiques
SCROLL 0             ' Mode page
SCROLL 1             ' Mode rouleau
SCROLL               ' En mode rouleau, scrolle vers le haut
SCROLL UP            ' En mode rouleau, scrolle vers le haut
SCROLL DOWN          ' En mode rouleau, scrolle vers le bas
INS LINE             ' Insérer une ligne
INS CHAR n           ' 0=désactivé, 1=activé — bascule le mode insertion
DEL LINE             ' Supprimer une ligne
DEL CHAR             ' Supprimer un caractère
```

`INS CHAR 1` passe le terminal en mode insertion : les caractères
affichés ensuite décalent le reste de la ligne vers la droite au lieu de
l'écraser. `INS CHAR 0` le désactive à nouveau. Contrairement à la plupart
des autres fonctions TTY, aucune des formes de `INS`/`DEL` ne dépend du
mode écran courant (40 ou 80 colonnes) — les mêmes codes sont envoyés dans
les deux cas.

`MODE 0` et `MODE 1` passent tous les deux en mode Videotex 40 colonnes,
mais ne sont pas identiques : `MODE 0` envoie uniquement le changement de
largeur de colonne, tandis que `MODE 1` renvoie en plus toute la séquence
d'initialisation du terminal (écho local désactivé, mode rouleau, clavier
minuscule, clavier étendu) — la même que celle envoyée automatiquement au
démarrage. Utiliser `MODE 1` pour réinitialiser complètement le terminal
dans son état normal (par exemple après un `MODE 2`), et `MODE 0` quand
seule la largeur de colonne doit changer.

Exemple illustrant `DEL CHAR` :

```basic
10 CLS
20 FOR i = 1 TO 24
30 PRINT REP$ 40, "*";
40 NEXT i
50 AT 12, 15
60 PRINT ">>> DEL CHAR <<<"
70 PAUSE 1000
80 AT 12, 20
90 FOR i = 1 TO 20
100 DEL CHAR
110 PAUSE 100
120 NEXT i
```

Ce programme remplit l'écran d'astérisques (lignes 10-40), affiche un message au
centre (lignes 50-60), puis positionne le curseur ligne 12, colonne 20 et
supprime 20 caractères consécutifs, créant un « trou » visible dans l'affichage.

#### Positionnement du curseur

```basic
AT ligne, col
```

Lignes et colonnes sont indexées à partir de 1.

En mode Videotex, les mouvements relatifs du curseur peuvent être insérés dans
les chaînes par leur code hexadécimal :

```basic
PRINT "Bonjour\x08\x08Hi"  ' Reculer deux fois, afficher "Hi"
```

| Code | Hexa | Déplacement |
|------|------|-------------|
| 8 | `\x08` | Gauche (retour arrière) |
| 9 | `\x09` | Droite (tabulation) |
| 10 | `\x0a` | Bas (saut de ligne) |
| 11 | `\x0b` | Haut (tabulation verticale) |

Exemple de dessin d'un cadre avec les caractères semi-graphiques G1 :

```basic
10 CLS
30 REM "Cadre 20x10 au centre"
40 x = 10
50 y = 7
60 w = 20
70 h = 10
80 REM "Coin haut gauche"
90 AT y, x
100 PRINT G1 "7";
110 REM "Ligne horizontale haut"
120 FOR i = 1 TO w - 2
130 PRINT "\x23";
140 NEXT i
150 REM "Coin haut droit"
160 PRINT "k"
170 REM "Lignes verticales"
180 FOR i = 1 TO h - 2
190 AT y + i, x
200 PRINT G1; "5";
210 AT y + i, x + w - 1
220 PRINT G1; "j"
230 NEXT i
240 REM "Coin bas gauche"
250 AT y + h - 1, x ; G1
260 PRINT "u";
270 REM "Ligne horizontale bas"
280 FOR i = 1 TO w - 2
290 PRINT "p";
300 NEXT i
310 REM "Coin bas droit"
320 PRINT "z"
340 AT y + 5, x + 5
350 PRINT "BASTOS"
```

Ce programme dessine un cadre de 20×10 caractères centré à l'écran en utilisant
les caractères semi-graphiques G1. Le cadre utilise : `7` (coin haut gauche),
`\x23` (ligne horizontale haute), `k` (coin haut droit), `5` (ligne verticale
gauche), `j` (ligne verticale droite), `u` (coin bas gauche), `p` (ligne
horizontale basse) et `z` (coin bas droit). Le texte « BASTOS » est affiché à
l'intérieur du cadre en mode G0 (ASCII).

#### Couleurs et attributs

```basic
INK couleur          ' Couleur de premier plan (0-7)
PAPER couleur        ' Couleur d'arrière-plan (0-7) ; sans effet en mode téléinformatique (≥2)
FLASH n              ' 0=normal, 1=clignotant
INVERSE n            ' 0=normal, 1=vidéo inverse
UNDERLINE n          ' 0=normal, 1=souligné
SIZE n               ' 0=normale, 1=double hauteur, 2=double largeur, 3=double taille
```

En mode téléinformatique, `INK 7` active la surbrillance ; `INK 0`–`6` la
désactive. `PAPER` est sans effet. `SIZE` est un attribut local en mode
Videotex.

#### Graphiques

Chaque caractère à l'écran est une matrice semi-graphique de 3 lignes × 2
colonnes de pixels. L'écran (hors ligne 0) comporte 24 lignes de 40 caractères,
soit **80 pixels de largeur × 72 pixels de hauteur**. L'origine **(0, 0) est en
bas à gauche** : x varie de 0–79 (gauche à droite), y varie de 0–71 (bas vers
haut).

```basic
PLOT x, y            ' Allumer un pixel
UNPLOT x, y          ' Éteindre un pixel
TEST x, y            ' Renvoie 1 si le pixel est allumé, 0 sinon
```

#### Vitesse

Définir la vitesse du port série entre SonOff et Minitel :

```basic
SLOW                 ' 1200 bps (défaut)
FAST                 ' 4800 bps
FAST2                ' 9600 bps (Minitel 2 et Magis Club uniquement)
```

### Sortie vers variable

Rediriger l'affichage vers une variable chaîne :

```basic
OUTPUT m$
CLS
AT 10, 13; "*** METEOR ***"
OUTPUT STOP
PRINT m$
```

### Entrée clavier

Lit une valeur au clavier et l'affecte à une variable.

```basic
INPUT variable
INPUT "invite", variable
```

- Pour une variable numérique, attend un nombre.
- Pour une variable chaîne (`$`), lit jusqu'à Entrée.
- Après la saisie, `VKEY` contient le code de la touche de validation.

Codes VKEY des touches de fonction du Minitel :

| Touche | VKEY | Notes |
|--------|------|-------|
| Entrée / ENVOI | 13 | Termine la saisie |
| CORRECTION | 127 | Efface le dernier caractère ; non retourné par INPUT |
| ANNULATION | 1 | Efface toute la saisie ; non retourné par INPUT |
| REPETITION | 2 | Termine la saisie |
| SUITE | 4 | Termine la saisie |
| RETOUR | 5 | Termine la saisie |
| SOMMAIRE | 6 | Termine la saisie |
| GUIDE | 14 | Termine la saisie |

Lecture non bloquante (sans attente) :

```basic
10 k$ = INKEY$
20 IF k$ <> "" THEN PRINT "Touche : " k$
30 PAUSE 100
40 GOTO 10
```

Les touches non imprimables (comme les touches de fonction) ne peuvent pas être
lues comme des caractères réguliers. Utiliser `CODE INKEY$` pour obtenir le code
numérique :

```basic
10 k = CODE INKEY$
20 PAUSE 100
30 IF k = 0 THEN GOTO 10
40 PRINT "Code touche : "; k
50 GOTO 10
```

---

## Types, variables, calculs

### Types

| Type | Suffixe | Stockage |
|------|---------|----------|
| Nombre | aucun | flottant 32 bits |
| Chaîne | `$` | longueur variable |

Les littéraux numériques peuvent être écrits en décimal ou en hexadécimal :

```basic
a = 255
a = 0xff
a = 0x1f
```

Les littéraux chaîne supportent les séquences d'échappement :

| Échappement | Description |
|-------------|-------------|
| `\n` | Saut de ligne |
| `\r` | Retour chariot |
| `\e` | Échap (0x1B) |
| `\xNN` | Octet hexadécimal |

```basic
a$ = "bonjour\n"
b$ = "\e[2J"          ' effacement écran ANSI
c$ = "\x1b\x41\x42"  ' Échap + 'A' + 'B'
```

### Caractères semi-graphiques

Appuyer sur **Ctrl+G** pour basculer entre les jeux G0 (ASCII) et G1
(semi-graphiques). Les caractères tapés en mode G1 sont affichés depuis le jeu
semi-graphique. Les images suivantes montrent la correspondance entre les
caractères G0 au clavier et leurs équivalents semi-graphiques G1 :

![Correspondance G0↔G1](g02g1.png)

### Caractères UTF-8 supportés (conversion Minitel)

Le clavier Minitel permet de taper tous ces caractères (accents, symboles,
flèches, tracé de lignes), mais éditer un programme dans l'éditeur de
ligne intégré de BASTOS reste bien plus contraignant que dans un éditeur
de texte sur PC (VSCode par exemple) : coloration syntaxique, copier/coller,
recherche, voire assistance par IA. Pour écrire et éditer des programmes
BASTOS sur PC avec ce confort, ces caractères peuvent être saisis sous
leur forme UTF-8 normale dans un fichier `.bas` ASCII, avec n'importe quel
éditeur de texte : lors du `LOAD` de ce fichier, BASTOS convertit
automatiquement une liste limitée de caractères UTF-8 en séquences Minitel
équivalentes, sans intervention de l'utilisateur.

Les lettres accentuées et symboles passent par le jeu G2 (préfixe SS2,
code `0x19` — un simple-shift, qui ne perturbe donc pas le jeu de
caractères actif par ailleurs). Les flèches
utilisent aussi le jeu G2 de la même façon. Les caractères de tracé de
lignes sont en revanche de simples glyphes du jeu G0 — le même jeu que les
chiffres et les lettres — donc ils sont convertis en un seul octet, sans
aucun décalage de jeu. Deux d'entre eux (`|` pour la barre verticale du
milieu, `_` pour la barre horizontale du bas) sont déjà de l'ASCII pur : il
n'y a rien à convertir, le même octet est déjà le code Minitel.

Pour saisir un caractère qui n'est pas directement accessible au clavier,
la plupart des éditeurs sous Linux (dont VSCode) acceptent la combinaison
**Ctrl+Maj+U**, suivie des chiffres du point de code, puis **Entrée** ou
**Espace** — ce sont ces chiffres qui sont indiqués dans la colonne
« Point de code » ci-dessous. (Sous Windows : taper les chiffres puis
**Alt+X** dans les éditeurs qui le supportent. Sous macOS : activer la
disposition clavier « Unicode Hex Input », puis **Option** + chiffres.)

| Glyph | Point de code | Séquence UTF-8 | Séquence Minitel produite |
|-------|----------------|----------------|---------------------------|
| à | `E0` | `\xC3\xA0` | `\x19Aa` |
| è | `E8` | `\xC3\xA8` | `\x19Ae` |
| ù | `F9` | `\xC3\xB9` | `\x19Au` |
| é | `E9` | `\xC3\xA9` | `\x19Be` |
| â | `E2` | `\xC3\xA2` | `\x19Ca` |
| ê | `EA` | `\xC3\xAA` | `\x19Ce` |
| î | `EE` | `\xC3\xAE` | `\x19Ci` |
| ô | `F4` | `\xC3\xB4` | `\x19Co` |
| û | `FB` | `\xC3\xBB` | `\x19Cu` |
| ä | `E4` | `\xC3\xA4` | `\x19Ha` |
| ë | `EB` | `\xC3\xAB` | `\x19He` |
| ï | `EF` | `\xC3\xAF` | `\x19Hi` |
| ö | `F6` | `\xC3\xB6` | `\x19Ho` |
| ü | `FC` | `\xC3\xBC` | `\x19Hu` |
| ç | `E7` | `\xC3\xA7` | `\x19Kc` |
| Ç | `C7` | `\xC3\x87` | `\x19KC` |
| ß | `DF` | `\xC3\x9F` | `\x19\x7B` |
| £ | `A3` | `\xC2\xA3` | `\x19\x23` |
| § | `A7` | `\xC2\xA7` | `\x19\x27` |
| ° | `B0` | `\xC2\xB0` | `\x19\x30` |
| ± | `B1` | `\xC2\xB1` | `\x19\x31` |
| ÷ | `F7` | `\xC3\xB7` | `\x19\x38` |
| ¼ | `BC` | `\xC2\xBC` | `\x19\x34` |
| ½ | `BD` | `\xC2\xBD` | `\x19\x35` |
| ¾ | `BE` | `\xC2\xBE` | `\x19\x36` |
| Œ | `152` | `\xC5\x92` | `\x19\x6A` |
| œ | `153` | `\xC5\x93` | `\x19\x7A` |
| ← | `2190` | `\xE2\x86\x90` | `\x19\x2C` |
| ↑ | `2191` | `\xE2\x86\x91` | `\x19\x2D` |
| → | `2192` | `\xE2\x86\x92` | `\x19\x2E` |
| ↓ | `2193` | `\xE2\x86\x93` | `\x19\x2F` |
| ▏ (verticale, gauche) | `258F` | `\xE2\x96\x8F` | `\x7B` |
| \| (verticale, milieu) | *(touche clavier)* | `\x7C` | `\x7C` (inchangé) |
| ▕ (verticale, droite) | `2595` | `\xE2\x96\x95` | `\x7D` |
| ▔ (horizontale, haut) | `203E` | `\xE2\x80\xBE` | `\x7E` |
| ─ (horizontale, milieu) | `2500` | `\xE2\x94\x80` | `\x60` |
| _ (horizontale, bas) | *(touche clavier)* | `\x5F` | `\x5F` (inchangé) |

Les autres caractères UTF-8 ne sont pas convertis et restent inchangés.

### Noms de variables

- Numérique : un ou plusieurs caractères, ex. `x`, `compteur`, `total`
- Chaîne : nom terminé par `$`, ex. `nom$`, `buf$`
- Les mots-clés sont insensibles à la casse ; le contenu des chaînes est
  sensible à la casse.

```basic
compteur = 3.14
nom$ = "bonjour"
```

Affectation avec ou sans `LET` :

```basic
LET x = 42
x = 42
```

### Tableaux

Déclarer avec `DIM` avant utilisation. Les tableaux sont indexés à partir de
**1**.

```basic
DIM a(10)             ' tableau 1-D de 10 nombres
DIM m(5, 5)           ' tableau 2-D
DIM s$(10, 25)        ' tableau de chaînes, 25 caractères max chacune
```

Accès :

```basic
a(3) = 99
PRINT a(3)
m(2, 4) = 1.5
s$(1) = "premier"
```

### Opérations sur les chaînes

Les parenthèses sont optionnelles pour toutes les fonctions ; ne les utiliser
que pour grouper.

| Opération | Syntaxe | Exemple |
|-----------|---------|---------|
| Concaténation | `a$ + b$` | `"bonjour" + " monde"` |
| Longueur | `LEN s$` | `LEN "abc"` → `3` |
| Lecture sous-chaîne | `s$(début TO fin)` | `a$(11 TO 13)` |
| Écriture sous-chaîne | `s$(début TO fin) = "..."` | `a$(1 TO 3) = "XYZ"` |
| Code ASCII | `CODE s$` | `CODE "A"` → `65` |
| Caractère | `CHR$ n` | `CHR$ 65` → `"A"` |
| Vers nombre | `VAL s$` | `VAL "3.14"` → `3.14` |
| Vers chaîne | `STR$ n` | `STR$ 42` → `"42"` |
| Vers chaîne, dans une base | `STR$ n, base` | `STR$(255, 16)` → `"FF"` |
| Vers chaîne, avec un format | `STR$ n, base, format` | `STR$(5, 10, "000.00")` → `"005.00"` |
| Recherche | `INDEX s1$, s2$` | `INDEX "bonjour", "on"` → `2` |
| Recherche depuis pos | `INDEX s1$, s2$, début` | |
| Répétition | `REP n, s$` | `REP 3, "-"` → `"---"` |

```basic
PRINT LEN a$
PRINT CODE k$
z$ = CHR$ 0
ia$ = CHR$(CODE a$ & 223)   ' parenthèses pour grouper uniquement
```

Le 2e argument de `STR$` convertit le nombre (tronqué en entier) dans la
base donnée (2 à 36, chiffres `0`-`9` puis `A`-`Z`) :

```basic
PRINT STR$(255, 16)   ' "FF"
PRINT STR$(10, 2)     ' "1010"
PRINT STR$(-255, 16)  ' "-FF"
```

Le 3e argument, s'il est donné, est un format à la « BASIC » : `#` affiche
un chiffre, ou un blanc si c'est un zéro de tête non nécessaire ; `0`
affiche toujours un chiffre (complété par des zéros) ; `.` marque la
virgule décimale. Le signe `-` d'un nombre négatif se place juste devant
les chiffres (éventuellement blanchis) ; une valeur plus large que le
modèle s'affiche en entier plutôt que d'être tronquée.

La présence ou non d'un `.` dans le format détermine comment la base est
utilisée :

- **Avec un `.`** : la valeur est affichée en décimal (la base est
  ignorée) — les chiffres après la virgule sont toujours affichés,
  arrondis au nombre de `#`/`0` après le `.`, jamais transformés en
  blancs.
- **Sans `.`** : la valeur (tronquée en entier) est d'abord convertie
  dans la base donnée, puis cette chaîne de chiffres est complétée/blanchie
  contre le format, exactement comme la partie entière du cas décimal
  ci-dessus. C'est la façon naturelle de compléter par des zéros une
  conversion hexadécimale ou binaire sur une largeur fixe :

```basic
PRINT STR$(10, 2, "00000000")   ' "00001010" (binaire, complété à 8 chiffres)
PRINT STR$(255, 16, "0000")     ' "00FF" (hexadécimal, complété à 4 chiffres)
```

```basic
PRINT STR$(5, 10, "###.##")    ' "  5.00" (zéros de tête blanchis)
PRINT STR$(5, 10, "000.00")    ' "005.00" (zéros de tête conservés)
PRINT STR$(123.456, 10, "###.#")  ' "123.5" (arrondi)
PRINT STR$(-5, 10, "###.##")   ' "-  5.00"
```

Les indices de sous-chaîne utilisent le mot-clé `TO`. `début` vaut `1` par
défaut, `fin` vaut `LEN s$` par défaut :

```basic
a$ = "Alice and Bob"
PRINT a$(11 TO 13)   ' "Bob"
PRINT a$(TO 5)       ' "Alice"  (début omis → 1)
PRINT a$(7 TO)       ' "d Bob"  (fin omise → LEN a$)
PRINT a$(TO)         ' chaîne entière
```

### Opérateurs arithmétiques

| Opérateur | Description |
|-----------|-------------|
| `*` `/` `%` | Multiplication, division, modulo |
| `+` `-` | Addition, soustraction |
| `&` | ET binaire |
| `\|` | OU binaire |

### Opérateurs de comparaison

| Opérateur | Signification |
|-----------|---------------|
| `=` | Égal |
| `<>` | Différent |
| `<` `>` | Inférieur / supérieur |
| `<=` `>=` | Inférieur ou égal / supérieur ou égal |

Le résultat est `1` (vrai) ou `0` (faux).

### Opérateurs logiques

```basic
IF a > 0 AND b > 0 THEN PRINT "les deux positifs"
IF a = 0 OR b = 0 THEN PRINT "l'un est nul"
IF NOT a THEN PRINT "a est nul"
```

### Fonctions mathématiques

Les parenthèses sont optionnelles ; ne les utiliser que pour grouper des
sous-expressions.

| Fonction | Description |
|----------|-------------|
| `ABS x` | Valeur absolue |
| `INT x` | Troncature entière |
| `SGN x` | Signe : -1, 0 ou 1 |
| `SQR x` | Racine carrée |
| `SIN x` `COS x` `TAN x` | Trigonométrie |
| `ASN x` `ACS x` `ATN x` | Trigonométrie inverse |
| `EXP x` | e^x |
| `LN x` | Logarithme naturel |
| `RND` | Flottant aléatoire 0.0–1.0 |
| `PI` | 3.1415926536 |

```basic
PRINT ABS -5
PRINT SQR 2
PRINT INT(a / b)    ' parenthèses pour grouper
```

`RAND graine` initialise le générateur pseudo-aléatoire lu par `RND`, pour
qu'un programme puisse reproduire la même séquence de valeurs `RND` d'une
exécution à l'autre (pratique pour des tests, ou un jeu qui veut un niveau
reproductible à partir d'une graine donnée) :

```basic
RAND 42
```

---

## Structures de contrôle

### Instructions multiples sur une ligne (`:`)

Plusieurs instructions peuvent être placées sur la même ligne, séparées par
`:` :

```basic
10 a = 1 : b = 2 : c = 3
20 PRINT a : PRINT b : PRINT c
```

Avec `IF`, l'instruction (ou la suite d'instructions séparées par `:`) qui
suit `THEN` n'est exécutée que si la condition est vraie ; sinon, **tout le
reste de la ligne** est ignoré :

```basic
10 IF x > 0 THEN PRINT "positif" : compteur = compteur + 1
```

Une boucle `FOR`/`NEXT` complète peut elle aussi tenir sur une seule ligne :

```basic
10 FOR i = 1 TO 3 : PRINT i : NEXT
```

### REM et commentaires (`'`)

`REM` ajoute un commentaire sur sa propre ligne. Ce n'est pas une structure
de contrôle à proprement parler ; elle provoque simplement la poursuite de
l'exécution à la ligne suivante sans effectuer aucune action.

```basic
REM "commentaire"
```

```basic
10 REM "Initialiser les variables"
20 x = 0
30 REM "Ceci est un commentaire"
40 PRINT x
```

Une apostrophe `'` introduit également un commentaire, mais en fin de ligne,
après une ou plusieurs instructions : tout ce qui suit `'` jusqu'à la fin de
la ligne est ignoré à l'exécution, y compris un `:` éventuel. Le commentaire
reste conservé dans le programme et réapparaît tel quel avec `LIST`.

```basic
10 x = 1 ' initialise x
20 PRINT x : PRINT x * 2 ' affiche x puis son double
```

### IF / THEN / ELSE

```basic
IF expression THEN numligne
IF expression THEN instruction [: instruction ...]
IF expression THEN ... ELSE numligne
IF expression THEN ... ELSE instruction [: instruction ...]
```

```basic
10 INPUT "x : ", x
20 IF x < 0 THEN PRINT "négatif" ELSE PRINT "positif ou nul"
30 IF x = 0 THEN 10
```

`ELSE` est optionnel. Quand il est présent, il introduit la ou les
instructions à exécuter quand le test du `IF` est faux ; quand il est
absent, un test faux saute simplement jusqu'à la fin de la ligne, comme
avant. Une seule des deux branches s'exécute — une fois qu'une branche
`THEN` vraie est terminée (y compris ses éventuelles instructions
`:`-chaînées), un `ELSE` présent sur la même ligne est toujours ignoré, et
inversement.

`ELSE`, comme `THEN`, accepte soit un numéro de ligne seul (raccourci pour
`GOTO numligne`), soit une ou plusieurs instructions séparées par `:` :

```basic
10 IF a = 0 THEN 100 ELSE 200
```

Une cible seule sur `THEN`/`ELSE` peut aussi être un nom d'[étiquette](#label)
entre guillemets (raccourci pour `GOTO "nom"`), résolu de la même façon —
voir LABEL plus bas :

```basic
10 IF a = 0 THEN "zero" ELSE "nonzero"
```

Les `IF` peuvent être imbriqués sur une même ligne via `:` ; chaque `ELSE`
se rattache au `IF` non apparié le plus proche, de la même façon que dans
la plupart des autres langages :

```basic
10 IF a = 1 THEN PRINT "a": IF b = 1 THEN PRINT "b aussi" ELSE PRINT "pas b"
```

`ELSE` ne peut pas être directement suivi d'un autre `IF` — la clause après
`ELSE` (comme celle après `THEN`) doit commencer par une instruction simple,
pas par `IF` lui-même. Pour tester un troisième cas, il faut placer une
instruction sans effet juste après `ELSE`, puis y enchaîner le `IF` imbriqué
avec `:` — `LET x=x` (une affectation sans effet) est un choix courant :

```basic
10 IF x < 0 THEN PRINT "négatif" ELSE LET x=x: IF x = 0 THEN PRINT "nul" ELSE PRINT "positif"
```

Ceci affiche exactement un des trois libellés. Si `x < 0` est vrai, `THEN`
affiche « négatif » et tout le `ELSE` (y compris le `IF` imbriqué) est
sauté jusqu'à la fin de la ligne. Si `x < 0` est faux, l'exécution saute
directement au `ELSE`, exécute le `LET x=x` sans effet, puis enchaîne sur le
`IF x = 0 ... ELSE ...` imbriqué, qui tranche entre « nul » et « positif ».

⚠️ **Piège courant** : `IF a>0 THEN a=a-1` ne fait pas ce que l'on croit — et
le même piège existe juste après `ELSE`. BASTOS n'interprète pas `a=a-1`
comme une affectation, mais comme un test de comparaison (« a est-il égal à
a-1 ? »), qui vaut `0` ou `1`. Ce résultat est ensuite traité exactement
comme le numéro de ligne d'un `GOTO` raccourci (le même mécanisme que `THEN
10`) : le programme saute réellement vers la ligne `0` ou `1`, ce qui n'est
ni une affectation ni un test sans effet. Pour écrire une vraie affectation
après `THEN`/`ELSE`, utiliser `LET` explicitement : `IF a>0 THEN LET a=a-1`.

### FOR / NEXT

```basic
FOR var = début TO fin
FOR var = début TO fin STEP pas
NEXT var
NEXT
```

- `var` doit être une lettre unique `A`–`Z`.
- Un `STEP` négatif compte à rebours.
- Les boucles peuvent être imbriquées.
- `NEXT` sans variable referme toujours la boucle la plus imbriquée
  actuellement active.
- `NEXT var` doit obligatoirement nommer cette même boucle la plus
  imbriquée ; nommer une boucle englobante non encore refermée provoque une
  erreur.

```basic
10 FOR i = 1 TO 5
20 PRINT i
30 NEXT i

40 FOR i = 10 TO 1 STEP -1
50 PRINT i
60 NEXT i
```

```basic
10 FOR i = 1 TO 2
20 FOR j = 1 TO 2
30 PRINT i; j
40 NEXT           ' referme la boucle j (la plus imbriquée)
50 NEXT i          ' referme la boucle i
```

### WHILE / WEND

```basic
WHILE condition
WEND
```

- La condition est vérifiée à chaque fois que `WHILE` est atteint, et de
  nouveau chaque fois que `WEND` renvoie l'exécution vers lui —
  contrairement à `FOR`, il n'y a pas de variable de boucle, donc la
  condition peut dépendre de n'importe quoi.
- Tant que la condition est vraie, l'exécution continue normalement dans
  le corps de la boucle ; quand `WEND` est atteint, il revient sur
  `WHILE` pour la vérifier à nouveau.
- Dès que la condition est fausse, l'exécution saute directement après le
  `WEND` correspondant, sans exécuter le corps.
- Les boucles peuvent être imbriquées, jusqu'à 8 niveaux.
- Un `WEND` sans `WHILE` actif est une erreur d'exécution.
- Revenir sur le `WHILE` d'une boucle active autrement que par son propre
  `WEND` (par exemple avec un `GOTO`) est une erreur d'exécution — la
  boucle ne peut être revérifiée qu'en atteignant son `WEND`.
- Si le `WEND` correspondant à un `WHILE` n'est jamais atteint alors que
  sa condition est fausse (par exemple, aucun `WEND` ne le suit nulle
  part dans le programme), le programme s'arrête simplement à cet
  endroit — comme un `GOTO` vers un numéro de ligne qui n'existe pas. Ce
  n'est pas considéré comme une erreur, car il n'y a pas de moyen fiable
  de distinguer ce cas d'un `WEND` qui aurait pu être atteint lors d'une
  autre exécution.

```basic
10 i = 1
20 WHILE i <= 5
30 PRINT i
40 i = i + 1
50 WEND
60 PRINT "fini"
```

Autre exemple, avec une condition qui ne se contente pas de compter : le
calcul du PGCD (plus grand commun diviseur) par l'algorithme d'Euclide, dont
on déduit le PPCM (plus petit commun multiple, `a * b / pgcd(a, b)`) :

```basic
10 INPUT "A: ", A: INPUT "B: ", B: X = A: Y = B
20 WHILE Y <> 0: T = X % Y: X = Y: Y = T: WEND
30 PRINT "PGCD ="; X
40 PRINT "PPCM ="; A * B / X
```

### GOTO

```basic
GOTO numligne
GOTO "label"
```

```basic
10 PRINT "boucle"
20 PAUSE 1000
30 GOTO 10
```

`GOTO` accepte aussi un nom d'étiquette entre guillemets à la place d'un
numéro de ligne — voir [LABEL](#label) ci-dessous.

### GOSUB / RETURN

```basic
GOSUB numligne     ' Appeler un sous-programme
GOSUB "label"      ' Appeler un sous-programme par son étiquette
RETURN             ' Retourner à l'appelant
```

Jusqu'à 32 appels imbriqués.

```basic
10 GOSUB 1000
20 END

1000 PRINT "Dans le sous-programme"
1010 RETURN
```

### LABEL

```basic
LABEL "nom"
LABEL START
```

Donne un nom à une ligne pour que `GOTO`/`GOSUB` — ainsi qu'une cible seule
sur `THEN`/`ELSE` (voir [IF / THEN / ELSE](#if--then--else)) — puisse y
sauter sans connaître son numéro — pratique pour un programme renuméroté ou
modifié au fil du temps.

```basic
1000 LABEL "decadix"
1010 PRINT "Dans decadix"
1020 RETURN
```

```basic
10 GOSUB "decadix"
20 END
```

La première fois qu'un nom d'étiquette est utilisé comme cible de saut
(`GOTO`/`GOSUB`, ou une cible seule sur `THEN`/`ELSE`), BASTOS recherche la
ligne `LABEL "nom"` correspondante — soit parce qu'elle a déjà été exécutée
(le cas normal, puisque `LABEL` s'exécute comme n'importe quelle
instruction, dans l'ordre du programme), soit, si elle n'a pas encore été
exécutée, en parcourant tout le programme pour la trouver. Dans les deux
cas, le numéro de ligne est ensuite retenu, de sorte que les sauts suivants
vers la même étiquette sont instantanés. Une étiquette doit être la toute
première instruction de sa ligne (`1000 LABEL "decadix"`, éventuellement
suivie d'autres instructions séparées par `:`) pour être trouvée par ce
parcours — utilisée ailleurs sur une ligne, `LABEL` fonctionne toujours
lorsqu'elle s'exécute réellement, mais ne sera pas trouvée à l'avance.
Cibler une étiquette absente de tout le programme est une erreur.

`LABEL START` parcourt tout le programme en une seule passe et mémorise
d'avance toutes les lignes `LABEL "nom"` qu'il trouve — utile en début de
programme pour éviter de payer le coût du parcours au premier saut vers
chaque étiquette :

```basic
10 LABEL START
20 GOSUB "decadix"
30 END

1000 LABEL "decadix"
1010 PRINT "Dans decadix"
1020 RETURN
```

Un simple `RUN` efface toutes les variables, y compris les positions
d'étiquettes mémorisées (par l'une ou l'autre forme de `LABEL`) — modifier
un programme, puis modifier une ligne qui porte un `LABEL`, sans `RUN`
entre les deux, peut donc laisser une cible de saut obsolète jusqu'au
prochain `RUN`.

`CLEAR`/`END` effacent également les positions d'étiquettes mémorisées, au
même titre que n'importe quelle autre variable — une étiquette n'est qu'une
variable dans son propre espace de noms. De même, le compte de variables de
`FREE` inclut les étiquettes.

### PAUSE

```basic
PAUSE millisecondes
```

```basic
PRINT "Attendre 2 secondes..."
PAUSE 2000
PRINT "Terminé"
```

---

## Fichiers et Base de données

### Fichiers

Lire le contenu d'un fichier sous forme de chaîne.

```basic
contenu$ = FILE "nomfichier"                  ' Lire tout le fichier
donnees$ = FILE "nomfichier", offset, taille  ' Lire taille octets à offset
```

La fonction `FILE` renvoie le contenu du fichier sous forme de chaîne. La forme
complète lit le fichier entier, tandis que la forme partielle lit un nombre
spécifique d'octets à partir d'un offset donné.

```basic
10 REM "Lire le fichier de configuration"
20 config$ = FILE "config.txt"
30 PRINT config$
```

Exemple de lecture et affichage d'un fichier ligne par ligne avec limite de 39
colonnes :

```basic
10 CLS ;AT 24,1;CURSOR 0
20 FAST
1000 a$=FILE "bastos.txt"
1100 deb=1
1110 fin=INDEX a$,"\n",deb
1120 IF fin<=0 THEN 2000
1130 l$=a$(deb,fin-1)
1140 deb=fin+1
1150 PRINT l$( TO 39)
1210 PAUSE 50
1220 GOTO 1110
2000 CURSOR 1
```

Ce programme lit le fichier entier en mémoire (ligne 1000), puis l'analyse ligne
par ligne en utilisant `INDEX` pour trouver les sauts de ligne (ligne 1110).
Chaque ligne est extraite (ligne 1130) et seuls les 39 premiers caractères sont
affichés (ligne 1150), garantissant un affichage correct sur un écran Vidéotex
de 40 colonnes.

### Base de données

BASTOS dispose d'un magasin clé/valeur organisé en sets numérotés.

```basic
GET set                      ' Renvoie toutes les clés du set sous forme de chaîne
GET set, "clé"               ' Renvoie la valeur associée à une clé
PUT set, "clé", "valeur"     ' Stocker ou mettre à jour une paire clé/valeur
DB LIST set                    ' Lister toutes les entrées du set
DB ERASE set, "clé"            ' Supprimer une entrée par clé du set
```

Les sets sont numérotés à partir de 0. Clés et valeurs sont des chaînes. Les
connexions WiFi, Minitel et FTP utilisent des sets spécifiques pour persister
leur configuration.

```basic
PUT 1, "ville", "Paris"
PRINT GET(1, "ville")    ' "Paris"
```

`GET set` renvoie toutes les clés séparées par `\n`. La dernière clé est
également suivie d'un `\n`. Utiliser `INDEX` pour les parcourir :

```basic
10 keys$ = GET 1
20 pos = 1
30 nl = INDEX(keys$, "\n", pos)
40 IF nl = 0 THEN END
50 key$ = keys$(pos TO nl - 1)
60 PRINT key$ " = " GET(1, key$)
70 pos = nl + 1
80 GOTO 30
```

---

## Réseau

### WiFi

```basic
WIFI SCAN                    ' Scanner les points d'accès disponibles
WIFI "ssid"                  ' Se connecter par SSID (mot de passe demandé si nécessaire)
WIFI START "ssid"            ' Identique (START est l'action par défaut)
WIFI n                       ' Se connecter au réseau numéro n (après SCAN)
WIFI LIST                    ' Lister les réseaux sauvegardés
WIFI STATUS                  ' Afficher l'état de la connexion
WIFI ERASE "ssid"            ' Supprimer un réseau sauvegardé
WIFI STOP                    ' Se déconnecter
```

Après `WIFI SCAN`, les réseaux sont numérotés ; utiliser `WIFI n` pour se
connecter par index.

### Format URN

Les connexions réseau sont identifiées par un URN dont les parties sont séparées par `:` :

```
protocole:hôte:port[:chemin[:login[:motdepasse]]]
```

Pour `ftp`, `login` vaut `anonymous` et `motdepasse` vaut `pat@frites.be` par
défaut si omis.

| Protocole | Description |
|-----------|-------------|
| `tcp` | Socket TCP brut |
| `ws` | WebSocket |
| `ftp` | FTP |

Exemples :

```
tcp:go.minipavi.fr:516
ws:3611.re:80:/ws
ftp:abasty-retro.fr:2121:bastos
ftp:files.example.com:21:/pub:monuser:monmotdepasse
ws:mntl.joher.com:2018:/?echo
```

### Minitel

Connexion à un serveur Minitel (émulation terminal Vidéotex) et sauvegarde
sous un nom.

```basic
MINITEL "nom", "urn"         ' Connecter et sauvegarder sous "nom"
MINITEL START "nom", "urn"   ' Identique (START est l'action par défaut)
MINITEL "nom"                ' Se reconnecter à une connexion sauvegardée
MINITEL LIST                 ' Lister les connexions Minitel sauvegardées
MINITEL ERASE "nom"          ' Supprimer une connexion sauvegardée
```

Exemples :

```basic
MINITEL "minipavi", "tcp:go.minipavi.fr:516"
MINITEL "3615", "ws:3615co.de:80:/ws"
MINITEL "3615"               ' reconnexion par nom sauvegardé
```

Une fois connecté, le programme suspend son exécution jusqu'à ce que
l'utilisateur quitte le mode connecté (ESC ESC) — une instruction
`:`-chaînée après `MINITEL` sur la même ligne n'est donc exécutée qu'à ce
moment-là, pas immédiatement après la connexion :

```basic
10 MODE 2 : MINITEL "3615" : MODE 1
```

Ici, `MODE 1` ne s'exécute qu'après que l'utilisateur soit revenu du mode
connecté.

### FTP

```basic
FTP "nom", "urn"             ' Connecter et sauvegarder sous "nom"
FTP START "nom", "urn"       ' Identique (START est l'action par défaut)
FTP "nom"                    ' Se reconnecter à une connexion sauvegardée
FTP LIST                     ' Lister les connexions FTP sauvegardées
FTP STATUS                   ' Afficher l'état de la connexion FTP
FTP PUT "fichier"            ' Envoyer un fichier (même nom local et distant)
FTP GET "fichier"            ' Recevoir un fichier (même nom local et distant)
FTP CAT                      ' Lister les fichiers distants
FTP CAT "motif"              ' Lister les fichiers distants correspondant à un motif
FTP ERASE "nom"              ' Supprimer une connexion sauvegardée
FTP STOP                     ' Se déconnecter
```

Exemple de session :

```basic
10 FTP "bastos", "ftp:abasty-retro.fr:2121:bastos"
20 FTP STATUS
30 FTP GET "snake.bas"
40 FTP STOP
```

---
