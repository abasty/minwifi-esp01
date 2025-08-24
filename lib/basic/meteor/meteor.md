# METEOR

## Présentation

PoC BASTOS sur Minitel avec le jeu METEOR ! (premier je en assembleur Z80 sur
ZX81 / 16KB)

## Description

## Écran d'accueil / règles du jeu

## Écran "Hall of fame" fin de jeu

* Si record battu, possibilité d'entrer son trigramme + un petit mot
* Sauvegarde dans DB
* Animation (si on peut pas utiliser les del et insert char du mode mixte)

## Écran configuration

* Choisir n° table DB, ou avoir la possibilité de connaître le première vide

## Écran credits

* Animation

## Écran play

* Ligne 0 : SCORE et RECORD, record sauvegardé dans DB.
* Scroll down, toutes les deux les deux lignes on insère un METEOR
* vaisseau spacial au bas de l'écran, touches gauche droite, effacé à chaque
  scroll down, réafficher à la position qu'il faut
* Comment gérer le delay (suivant le mode slow / fast / fast fast) ?
* Gestion des collisions : dim screen$(24, 40). À chaque scroll down on déplace
  un "offset" qui dit l'index de la ligne 1.
* Possibilté de tirer : à définir comment

# Dev

## Code

* Exec dans le simulateur
* Programme Basic édité dans VSCode, sauvegardé directement dans le disque émulé
* À chaque impossibilité ou se demande si on peut rajouter quelque chose au Basic
  pour gérer les cas levés par ce PoC

## Sequence videotex / sprites

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

### Outils

À prendre dans le code : l'automate de positionnement, des attributs,
évetuellement de l'optimisation (curseur, attributs) en vue de faire un SCREEN$

* MINOLD.EXE : Langage macro, assez facile à prendre en main, des bugs, manque
  de doc
* COMPO.EXE : Hyper complexe, pas de doc
* BASTOS
* Dart + Flutter like Minitel widgets (peut-être pour écrire un serveur, pas
  forcément pour le Basic)
* Éditeur online ?
