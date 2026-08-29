---
title: Extension VSCode
---

# Extension VSCode BASTOS

Une extension VSCode pour écrire du BASTOS confortablement sur PC : coloration
syntaxique, snippets, renumérotation des lignes, mise en forme automatique du
code et renommage de variable (`F2`).

## Fonctionnalités

- **Coloration syntaxique** : mots-clés, chaînes avec séquences
  d'échappement (`\n`, `\r`, `\e`, `\xNN`...), nombres décimaux et
  hexadécimaux, commentaires, noms d'étiquette entre guillemets après
  `GOTO`/`GOSUB`/`LABEL`/`THEN`/`ELSE`.
- **Snippets** : `for`, `while`, `if`, `ifelse`, `label`, `gosub`, `rem`,
  `input`, `at`, `dim`.
- **`BASTOS: Renumber Selected Lines`** (palette de commandes) :
  renumérote les lignes sélectionnées et met à jour les références
  `GOTO`/`GOSUB`/`THEN`/`ELSE` correspondantes dans tout le fichier.
- **Format Document** (`Maj+Alt+F`) : mots-clés en MAJUSCULES, variables
  en minuscules, espaces normalisés.
- **Renommer une variable ou une étiquette** (`F2`) : renomme toutes les
  occurrences d'une variable dans le fichier (BASTOS n'a pas de portée,
  les variables sont globales), ou d'une étiquette (le nom entre
  guillemets après `LABEL`/`GOTO`/`GOSUB`/`THEN`/`ELSE`).

Le détail de chaque fonctionnalité est documenté dans le
[README du projet](https://github.com/abasty/minwifi-esp01/tree/master/tools/vscode-bastos).

## Installation

L'extension n'est pas publiée sur le Marketplace VSCode ; elle s'installe
depuis un fichier `.vsix` :

**[⬇ Télécharger l'extension (bastos.vsix)](bastos.vsix)**

Puis, dans VSCode :

- Palette de commandes (`Ctrl+Maj+P`) → **Extensions: Install from
  VSIX...** → sélectionner le fichier téléchargé,

ou en ligne de commande, une fois le fichier téléchargé :

```sh
code --install-extension bastos.vsix
```

Recharger VSCode, puis ouvrir un fichier `.bas` : la coloration
syntaxique s'active automatiquement.

## Code source

Le code de l'extension fait partie du dépôt principal, dans
[`tools/vscode-bastos`](https://github.com/abasty/minwifi-esp01/tree/master/tools/vscode-bastos).
