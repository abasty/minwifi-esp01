# Extension VSCode BASTOS

Coloration syntaxique et confort d'édition pour les fichiers `.bas` de
[BASTOS](../../docs/BASTOS-MANUAL-fr.md), le dialecte BASIC du Minitel de ce
dépôt.

Contenu :

- **Coloration syntaxique** (`syntaxes/bastos.tmLanguage.json`) : numéros de
  ligne, mots-clés de contrôle (`IF`/`THEN`/`ELSE`/`FOR`/`WHILE`...),
  commandes (`PRINT`, `AT`, `CURSOR`, `WIFI`...), fonctions (`CHR$`, `STR$`,
  `INKEY$`...), variables `$` (chaîne) vs numériques, nombres décimaux et
  hexadécimaux, chaînes avec séquences d'échappement (`\n`, `\r`, `\e`,
  `\xNN`...), commentaires `REM "..."` et `' ...` fin de ligne, et les noms
  d'étiquette entre guillemets après `GOTO`/`GOSUB`/`LABEL`/`THEN`/`ELSE`.
- **`language-configuration.json`** : commentaire `'`, appariement des
  parenthèses et guillemets, indentation automatique des blocs
  `FOR`/`NEXT` et `WHILE`/`WEND`.
- **Snippets** (`snippets/bastos.code-snippets`) : `for`, `while`, `if`,
  `ifelse`, `label`, `gosub`, `rem`, `input`, `at`, `dim`.

- **Commande de palette** — `BASTOS: Renumber Selected Lines` : renumérote
  les lignes sélectionnées (numéro de départ et incrément demandés, défaut
  10), et met à jour dans tout le fichier les références directes
  `GOTO`/`GOSUB`/`THEN`/`ELSE numéro` qui pointent vers une ligne
  renumérotée. Les étiquettes entre guillemets n'ont pas besoin d'être
  touchées. Une cible calculée (ex. `GOSUB 2000+(b+c-1)*10`, comme dans
  `disk/connect.bas`) n'est jamais réécrite automatiquement — la commande
  refuse de deviner une expression arithmétique et signale la ligne pour
  une vérification manuelle. Un numéro de destination qui entrerait en
  collision avec une ligne existante hors sélection bloque toute la
  renumérotation (aucune modification n'est appliquée).

- **Format Document** (natif VSCode — `Maj+Alt+F`, clic droit, ou
  `editor.formatOnSave`) : met les mots-clés BASTOS en MAJUSCULES, les
  noms de variables en minuscules, et normalise les espaces selon deux
  règles combinées — voir [src/format.js](src/format.js) :
  - un espace est toujours conservé/ajouté entre un mot-clé et une valeur
    (chaîne, nombre, variable) de part et d'autre : `GOTO "x"`, `"Q" OR`,
    `1 TO`, `THEN "SaisieChoix"` ;
  - partout ailleurs (ponctuation `: ; ,` `(` `)`, opérateurs), les espaces
    superflus sont supprimés — un mot-clé reste collé à la ponctuation qui
    le précède ou le suit : `:GOSUB`, `0;AT`, `CLS:END`, `STR$(255,16,...)`.

  Résultat : le style déjà en place dans `disk/connect.bas`
  (`LABEL "Fin":MODE 1:CLS:END`) est reproduit tel quel. Le contenu des
  commentaires (`REM "..."` et `' ...`) n'est jamais reformaté.

Cette extension ne fait que colorer, faciliter la saisie, renuméroter et
formater : elle n'analyse pas le programme et ne remplace pas de test sur
émulateur ou Minitel réel.

## Installation locale

Pas encore publiée sur le Marketplace. Deux façons de l'essayer :

**Mode développement** (le plus simple pour itérer) :

1. Ouvrir ce dossier (`tools/vscode-bastos`) dans VSCode.
2. Appuyer sur `F5` : une fenêtre « Extension Development Host » s'ouvre
   avec l'extension chargée.
3. Ouvrir un fichier `.bas` (par ex. `disk/bastos.bas`) dans cette fenêtre.

**Installation persistante** (nécessite [`vsce`](https://github.com/microsoft/vscode-vsce)) :

```sh
npm install -g @vscode/vsce
cd tools/vscode-bastos
vsce package
code --install-extension bastos-0.1.0.vsix
```
