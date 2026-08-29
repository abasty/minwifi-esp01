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

- **Renommer une variable** (natif VSCode — `F2`, ou clic droit → Rename
  Symbol) : BASTOS n'a pas de portée — toutes les variables sont globales
  (voir [BASTOS-MANUAL-fr.md](../../docs/BASTOS-MANUAL-fr.md#noms-de-variables))
  — donc renommer une variable revient à renommer, dans tout le fichier,
  chaque occurrence de ce nom précis (comparaison insensible à la casse,
  comme le fait le vrai tokenizer). Le `$` final d'une variable chaîne
  n'est jamais éditable (il détermine son type, pas son nom) : seule la
  partie avant le `$` est mise en surbrillance et modifiable. Le
  renommage est refusé si le nouveau nom est un mot-clé réservé, s'il
  entrerait en collision avec une variable existante différente (fusion
  accidentelle), ou si le curseur n'est pas sur une variable.

  `F2` fonctionne aussi sur une **étiquette** : le nom entre guillemets
  après `LABEL`, ou après `GOTO`/`GOSUB`/`THEN`/`ELSE` quand il sert de
  cible de saut. Contrairement aux variables, les étiquettes sont de
  vraies chaînes — la comparaison est donc **sensible à la casse**
  (`"Retour"` ≠ `"retour"`), et seule la chaîne qui suit immédiatement un
  de ces cinq mots-clés compte comme référence : un `PRINT "Retour"`
  affichant un message n'est jamais touché. Même protection anti-fusion
  qu'avec les variables si le nouveau nom entre en collision avec une
  étiquette existante — voir [src/rename.js](src/rename.js).

Cette extension ne fait que colorer, faciliter la saisie, renuméroter,
formater et renommer : elle n'analyse pas le programme et ne remplace pas
de test sur émulateur ou Minitel réel.

## Installation locale

Pas encore publiée sur le Marketplace. Deux façons de l'essayer :

**Mode développement** (le plus simple pour itérer) :

1. Ouvrir ce dossier (`tools/vscode-bastos`) dans VSCode.
2. Appuyer sur `F5` : une fenêtre « Extension Development Host » s'ouvre
   avec l'extension chargée.
3. Ouvrir un fichier `.bas` (par ex. `disk/bastos.bas`) dans cette fenêtre.

**Installation persistante**, via un fichier `.vsix` — pas de compte ni
publication nécessaire, juste un fichier à installer (le tien ou celui
d'un tiers à qui tu l'as transmis) :

```sh
cd tools/vscode-bastos
npx @vscode/vsce package
code --install-extension bastos-0.2.0.vsix
```

(ou dans VSCode : palette de commandes → « Extensions: Install from
VSIX... »). Le `.vsix` est un artefact de build, pas versionné (voir
`.gitignore`) — à régénérer après chaque modification de l'extension.

## Publier

- **Partager un `.vsix`** (voir ci-dessus) : suffit pour un outil de niche
  comme celui-ci, aucune inscription nécessaire. C'est ce qui est fait sur
  [le site de doc](https://abasty.github.io/minwifi-esp01/BASTOS-VSCODE-EXTENSION-fr)
  (page [docs/BASTOS-VSCODE-EXTENSION-fr.md](../../docs/BASTOS-VSCODE-EXTENSION-fr.md),
  fichier [docs/bastos.vsix](../../docs/bastos.vsix)) : après un `vsce
  package`, penser à recopier le `.vsix` généré vers `docs/bastos.vsix`
  et à le committer — ce n'est pas automatisé.
- **VS Code Marketplace** ou **Open VSX** (installable depuis l'onglet
  Extensions de VSCode par n'importe qui) : nécessite un compte éditeur
  (`vsce login` / `ovsx publish`) que seul le mainteneur du dépôt peut
  créer — voir la
  [doc officielle de publication](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)
  le moment venu. `package.json` a déjà un champ `repository` pointant
  vers ce dépôt ; il faudrait encore retirer `"private": true` et ajouter
  une icône PNG dédiée avant un premier `vsce publish`.
