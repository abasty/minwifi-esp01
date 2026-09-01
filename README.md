![bastos](docs/bastos-title.png)

# 1. Documentation utilisateur

La documentation utilisateur est disponible en ligne sur [Documentation en
ligne](https://abasty.github.io/minwifi-esp01/)

Projet annexe : [BASTOS-EDI](https://github.com/abasty/bastos-edi) est un
environnement de développement intégré pour BASTOS, qui s'exécute sur un
ordinateur de bureau. Il permet de développer des programmes BASTOS sur PC
exécutables indifféremment dans l'émulateur intégré ou sur un Sonoff.

# 2. Compiler et déployer

Ce projet contient plusieurs cibles de build :

- **Firmware ESP8266** (`sonoff`) : pour Sonoff Basic R2/R3
- **Firmware ESP32-C3** (`sonoff-r4`) : pour Sonoff Basic R4
- **Binaire Linux** (`bastos`) : version Linux pour tests et développement
- **Fichiers BASTOS** (`disk/`) : programmes et données

## Prérequis

- **VSCode** avec l'extension **PlatformIO IDE**
- **make** (pour le build Linux)
- **scp** (pour le déploiement)

## Build avec VSCode + PlatformIO

### Compiler un firmware

1. Dans VSCode, ouvrir la barre latérale PlatformIO
2. Sélectionner **Project Tasks** → **Build** pour la cible actuelle

Le firmware compilé se trouve dans `.pio/build/<target>/firmware.bin`

### Compiler la version Linux

```bash
cd lib/basic/test
make clean && make TERM=MINITEL
```

Le binaire se trouve dans `lib/basic/test/bin/bastos-linux-amd64`

## Déploiement automatisé

Un script `build_and_deploy.sh` automatise la compilation et le déploiement
optionnel :

```bash
# Builder seulement (sans déploiement)
./build_and_deploy.sh

# Builder et déployer sur un serveur distant via SCP
./build_and_deploy.sh user@remote:/var/ftp

# Ou modifier la variable DEPLOY_SCP_FOLDER dans le script (ligne 7)
./build_and_deploy.sh
```

**Comportement du script** :
- La **compilation se fait toujours** (firmware et version Linux)
- Le **déploiement est optionnel** — seulement si `DEPLOY_SCP_FOLDER` est fourni
  en paramètre
- Si pas de déploiement : un **WARNING** s'affiche avec les instructions de
  configuration

Le script effectue :
1. Compilation des firmwares ESP8266 (`sonoff`) et ESP32-C3 (`sonoff-r4`)
2. Création de l'archive `sonoff-flash.tgz` contenant les binaires de flash et
   un script `flash.sh` pour flasher un Sonoff Basic R2/R3/SV ou R4
3. Compilation de la version Linux (`bastos`)
4. Si `DEPLOY_SCP_FOLDER` est défini : déploiement de `sonoff-flash.tgz`,
   du binaire Linux et des fichiers `disk/` sur le serveur distant

## Flasher un Sonoff à partir de l'archive

L'archive `sonoff-flash.tgz` est auto-suffisante pour flasher un Sonoff sans
avoir à compiler les sources.

### Prérequis

- **esptool** : `sudo apt install esptool`
- Un adaptateur **USB-UART** (CP2102, CH340, FTDI…) connecté au Sonoff

### Récupérer l'archive

```bash
curl -o sonoff-flash.tgz "ftp://anonymous:@abasty-retro.fr:2121/firmware/sonoff-flash.tgz"
tar -xzf sonoff-flash.tgz
```

### Flasher

> **Mise en mode flash** : maintenir le bouton du Sonoff enfoncé prendant 3
> secondes lors de la mise sous tension (par exemple en branchant la prise USB
> sur le PC). Ensuite, lancer `flash.sh`.

```bash
# Détection automatique du port et du modèle (R2/R3/SV ou R4)
./flash.sh

# Ou en spécifiant le modèle explicitement
./flash.sh r2       # Sonoff Basic R2/R3/SV (ESP8266)
./flash.sh r4       # Sonoff Basic R4 (ESP32-C3)

# Si plusieurs adaptateurs USB-UART sont connectés, préciser le port
PORT=/dev/ttyUSB1 ./flash.sh
```
