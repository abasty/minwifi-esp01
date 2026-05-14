### 3.2 Les commandes décodées par le module écran

| Séquence | Interprétations | (1) | (2) | (3) |
| :--- | :--- | :---: | :---: | :---: |
| | **ECRAN** | | | |
| ESC 3/7 | Mémorisation du contexte écran | | ● | ● |
| ESC 3/8 | Restitution du contexte écran | | ● | ● |
| ESC 6/3 | Reset de l'écran | | ● | ● |
| CSI 3/C 3/3 6/8 | Passage en 40 colonnes | | | ■ |
| CSI 3/F 3/3 6/C | Passage en 80 colonnes | | | ■ |
| CSI 3/C 3/4 6/8 | Passage en mode page | | | ■ |
| CSI 3/F 3/4 6/C | Passage en mode rouleau | | | ■ |
| | **CLAVIER** | | | |
| CSI 3/2 6/8 | Blocage du clavier | | | ● |
| CSI 3/2 6/C | Déblocage du clavier | | | ● |
| | **GESTION DU CURSEUR** | | | |
| CSI 3/1 3/2 6/C | Mise en marche de l'écho local | | | ■ |
| CSI 3/1 3/2 6/8 | Mise en arrêt de l'écho local | | | ■ |
| CSI 3/C 3/1 6/C | Allumage du curseur | | ■ | ■ |
| CSI 3/C 3/1 6/8 | Arrêt du curseur | | ■ | ■ |
| CSI 3/6 6/E | Demande de position du curseur | ■ | ■ | ■ |
| CSI Pr 3/B Pc 5/2 | Réponse à la demande de positionnement du curseur | ● | ● | ● |
| CSI Pr 3/B Pc 4/8 | Positionnement absolu du curseur. Les valeurs par défaut (3/1, 3/1) du couple (Pr, Pc) déterminent la position dite "HOME" | ● | ● | ● |
| ESC 4/4 | Provoque un LF | * | ● | ● |
| ESC 4/5 | Provoque un CR suivi d'un LF | * | ● | ● |
| ESC 4/D | Déplace le curseur vers le haut avec éventuellement, un défilement de l'écran en mode rouleau | * | ● | ● |
| CSI Pn 4/1 | Déplace le curseur vers le haut de n rangées. Arrêt en haut de l'écran. Valeur par défaut Pn=3/1 | ● | ● | ● |
| CSI Pn 4/2 | Déplace le curseur vers le bas de n rangées. Arrêt en bas de l'écran. Valeur par défaut Pn=3/1 | ● | ● | ● |
| CSI Pn 4/3 | Déplace le curseur vers la droite de n emplacements. Arrêt au bord droit de l'écran. Valeur par défaut Pn=3/1 | ● | ● | ● |
| CSI Pn 4/4 | Déplace le curseur vers la gauche de n emplacements. Arrêt au bord gauche de l'écran. Valeur par défaut Pn=3/1 | ● | ● | ● |

* Cf. annexe 3.5.

81

---

# Page 90

### EDITION

| Code | Description | | | |
| :--- | :--- | :---: | :---: | :---: |
| CSI Ps 4/A | Commande d'effacement d'écran. Ps est un Paramètre sélectif dont la valeur par défaut est 3/0 ;<br>• Ps=3/0 effacement depuis la position du curseur incluse jusqu'à la fin de l'écran<br>• Ps=3/1 effacement depuis le début de l'écran jusqu'à la position du curseur incluse<br>• Ps=3/2 effacement de tout l'écran.<br>La position active n'est pas modifiée par cette commande | ● | ● | ● |
| CSI Ps 4/B | Commande d'effacement dans une rangée. Ps est un Paramètre sélectif dont la valeur par défaut est 3/0 ;<br>• Ps=3/0 effacement depuis la position du curseur incluse jusqu'à la fin de la rangée<br>• Ps=3/1 effacement depuis le début de la rangée jusqu'à la position du curseur incluse<br>• Ps=3/2 effacement de toute la rangée.<br>La position active n'est pas modifiée par cette commande | ● | ● | ● |
| CSI Pn 4/C | Prépare l'insertion de n rangées en plaçant dans l'état effacé la rangée active et les n-1 rangées suivantes. Le curseur revient en colonne 1 de la rangée active. | ● | ● | ● |
| CSI 3/4 6/8 | Début du mode insertion de caractères. Pas de débordement. | ● | ● | ● |
| CSI 3/4 6/C | Arrêt du mode insertion de caractère | ● | ● | ● |
| CSI Pn 4/D | Suppression de n rangées. Le contenu de la rangée active et les n-1 rangées suivantes est supprimé. Le curseur revient en colonne 1 de la rangée active. | ● | ● | ● |
| • CSI Pn 4/0 | Suppression de n caractères | ● | ● | ● |
| CSI Pn 5/0 | Suppression de n caractères. Le contenu de la position active et les n-1 positions suivantes est supprimé. La position active n'est pas modifiée | ● | ● | ● |

### COPIE D'ECRAN

| Code | Description | | | |
| :--- | :--- | :---: | :---: | :---: |
| CSI 6/9 | Commande de copie d'écran. Interprétée uniquement en provenance du modem (en connecté) | | | ● |

### STANDARD DE FONCTIONNEMENT

| Code | Description | | | |
| :--- | :--- | :---: | :---: | :---: |
| CSI 3/F 7/B | Retour au standard Télétel mode Vidéotex | | | ● |

• Modèle Philips uniquement.

82

---

# Page 91

| Séquence | Interprétations | (1) | (2) | (3) |
| :--- | :--- | :---: | :---: | :---: |
| | **ATTRIBUT** | | | |
| CSI Pn 6/D | Activation/désactivation des attributs de caractère en format 80 colonnes exclusivement ;<br>- si Pn=3/0 aucun attribut<br>- si Pn=3/1 activation surintensité<br>- si Pn=3/2 3/2 désactivation surintensité<br>- si Pn=3/4 activation souligné<br>- si Pn=3/2 3/4 désactivation souligné<br>- si Pn=3/5 activation clignotant<br>- si Pn=3/2 3/5 désactivation clignotant<br>- si Pn=3/7 activation fond inversé<br>- si Pn=3/2 3/7 désactivation fond inversé | | ● | ● |

(1) Standard Télétel mode Vidéotex.
(2) Standard Télétel mode Mixte.
(3) Standard Téléinformatique.
● indique que la commande est valide dans le mode ou standard courant.
■ Commande n'existant pas sur M1B.
