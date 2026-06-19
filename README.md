![bastos](docs/bastos-title.png)

# 1. Documentation utilisateur

La documentation utilisateur est disponible en ligne sur [Documentation en
ligne](https://abasty.github.io/minwifi-esp01/)

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
3. Ou utiliser le terminal VSCode :
   ```bash
   pio run -e sonoff          # Compiler pour Sonoff
   pio run -e sonoff-r4       # Compiler pour ESP32-C3
   ```

Le firmware compilé se trouve dans `.pio/build/<target>/firmware.bin`

### Compiler la version Linux

```bash
cd lib/basic/test
make clean && make TERM=MINITEL
```

Le binaire se trouve dans `lib/basic/test/bin/bastos`

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

Le script effectue (selon la configuration) :
1. Compilation des firmware ESP8266 et ESP32-C3
2. Compilation de la version Linux
3. Copie des binaires et fichiers BASTOS via SCP (optionnel)
4. Copie de la documentation via SCP (optionnel)
