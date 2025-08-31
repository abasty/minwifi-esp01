# METEOR

PoC BASTOS sur Minitel avec le jeu METEOR (premier jeu en assembleur Z80 sur
ZX81 / 16KB) !

La section "Exigences BASTOS" contient les contraintes et desiderata de
l'équipe de dev "METEOR". Les exigences marquées d'une case à cocher sont
celles retenues par l'équipe de dev "BASTOS v1".

## Design

TBD : Enchainement des écrans.

### Écran d'accueil / règles du jeu

### Écran "Hall of fame" fin de jeu

* Si record battu, possibilité d'entrer son trigramme + un petit mot
* Sauvegarde dans DB
* Animation (si on peut pas utiliser les del et insert char du mode mixte)

### Écran configuration

* Choisir n° table DB, ou avoir la possibilité de connaître le première vide

### Écran credits

* Animation

### Écran play

* Ligne 0 : SCORE et RECORD, record sauvegardé dans DB.
* Scroll down, toutes les deux les deux lignes on insère un METEOR
* vaisseau spacial au bas de l'écran, touches gauche droite, effacé à chaque
  scroll down, réafficher à la position qu'il faut
* Comment gérer le delay (suivant le mode slow / fast / fast fast) ?
* Gestion des collisions : dim screen$(24, 40). À chaque scroll down on déplace
  un "offset" qui dit l'index de la ligne 1.
* Possibilté de tirer : à définir comment

## Dev

### Code

* Exec dans le simulateur
* Programme Basic édité dans VSCode, sauvegardé directement dans le disque émulé
* À chaque impossibilité ou se demande si on peut rajouter quelque chose au Basic
  pour gérer les cas levés par ce PoC

### Assets (séquences videotex / sprites)

Choix préféré :
* Génération avec BASTOS

Alternatives :
* MINOLD.EXE : Langage macro, assez facile à prendre en main, des bugs, manque
  de doc
* COMPO.EXE : Hyper complexe, pas de doc
* Dart + Flutter like Minitel widgets (peut-être pour écrire un serveur, pas
  forcément pour le Basic)
* Éditeur online ?

## Exigences BASTOS

### Composition d'écrans Videotex

1. On utilise BASTOS pour définir des variables contenant du videotex (programme
   `meteor.vdt.bas`)
2. On sauve les variables avec `SAVE "meteor.vdt.var"`
3. On pourra les utiliser dans le programme principal avec `LOAD
   "meteor.vdt.var"`
4. On pourra même distribuer le programme et ses variables en supprimant le
   `LOAD *.var` et en le sauvant avec `SAVE "meteor.bst"`

Exemple `meteor.vdt.bas` :

```basic
output m$
cls
at 10, 13; "*** METEOR ***"
output stop
save"meteor.vdt.var"
run"meteor"
```

Le programme `meteor.bas` commence avec `load "meteor.vdt.var"` :

```basic
10 load"meteor.vdt.var"
20 ?m$
```

Dans `autoexec.bas`, on peut automatiser le cycle de dev :

```
run"meteor.vdt.bas"
```

Desiderata BASTOS :

* [x] Pouvoir rediriger la sortie vers un buffer (variable chaîne de caractères)
  :
  ```basic
  OUTPUT [START] m$
  cls
  at 12,13;"*** METEOR ***\r\n"
  OUTPUT STOP
  ? "m$ défini !"
  ```
* [ ] Accès à la plupart des séquences videotex de changement d'attributs,
  d'edition, de gestion du curseur, etc. (Voir `SYSLIB0.MIN`), smart `REP`, etc.
  => nouvelles fonction TTY
* ~~Opérateur `+=`, valabale au min pour les chaines ou une autre approche~~
* Pouvoir récupérer des _assets_ depuis des fichiers videotex dans une variable
  _string_ `RAW LOAD <VAR-REF>, <FILENAME>`, `RAW SAVE <VAR-REF>, <FILENAME>`,
  `RAW PRINT <VAR-REF>` (affiche l'hexa)

### Cycle de dev dans simulateur

* [ ] Ajout d'un bouton `RESET` dans _minterm_
* Configuration d'un script de _reset_, par défaut `RESET` mais pourrait être :
  ```basic
  RUN"meteor.vdt.bas"
  SAVE"meteor.var"
  RUN"meteor"
  ```
  En phase de dev on peut très bien vivre avec `autoexec.bas`, et le bouton
  _reset_ de _minterm_

## Annexe A

### dosbox-staging

:warning: Pour utliser les programmes exécutable sour DOS, utiliser
`dosbox-staging` plutôt que `dosbox` des repos Debian. Paramètres à changer dans
`dosbox-staging.conf` :

```ini
language                    = fr
...
[autoexec]
# Each line in this section is executed at startup as a DOS command.
mount c "~/Projects/Save Prog/prog/tp7/"
```
