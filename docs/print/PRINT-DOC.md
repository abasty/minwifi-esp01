# Impression du livret BASTOS Quick Start

Produit un livret de format A6 depuis le fichier `BASTOS-QUICK-START-fr.md`.
Chaque face A4 porte 4 pages A6 (grille 2×2). Après découpe en 2 bandes et pli
de chaque bande, les bandes s'emboîtent pour former le livret.

---

## Prérequis

### Outils système

```bash
sudo apt install pandoc texlive-xetex texlive-extra-utils texlive-lang-french
```

### Mermaid CLI (rendu des diagrammes)

```bash
sudo apt install npm
npm install -g @mermaid-js/mermaid-cli
```

---

## 1. Générer le PDF livret

Depuis la racine du dépôt :

```bash
bash docs/print/build.sh
```

Le script réalise trois étapes :

1. Convertit les blocs `mermaid` en images PNG (`mmdc`)
2. Convertit le Markdown en PDF A6 (`pandoc` + `xelatex`)
3. Impose les pages pour livret A4 (`pdfbook2`)

Résultat : `/tmp/bastos-qs/quickstart-book.pdf`
(4 pages A4 portrait = **2 feuilles** à imprimer recto-verso)

---

## 2. Imprimer

### Étape A — Rectos (pages 1 et 3)

```bash
lp -P 1,3 /tmp/bastos-qs/quickstart-book.pdf
```

Les feuilles sortent **face vers le bas** dans le bac de sortie.

### Étape B — Trouver le sens de retournement (à faire une seule fois)

1. Imprimer **uniquement la page 1** pour test.
2. Sans toucher à la feuille, noter l'orientation (quel côté en haut, quelle face visible).
3. Retourner la feuille **selon le bord long** (axe vertical — comme pour ouvrir un livre).
4. La replacer dans le bac d'alimentation.
5. Imprimer **uniquement la page 2** :
   ```bash
   lp -P 2 /tmp/bastos-qs/quickstart-book.pdf
   ```
6. Vérifier que la page 2 est au dos de la page 1, dans le bon sens.
   - Si oui → ce retournement est le bon, passer à l'étape C.
   - Si non → essayer l'autre axe (retournement selon le bord court) et recommencer.

### Étape C — Versos (pages paires)

1. Reprendre la pile des rectos sans changer l'ordre.
2. Appliquer le retournement identifié à l'étape B.
3. Replacer la pile dans le bac d'alimentation.
4. Imprimer les versos (pages 2 et 4) :
   ```bash
   lp -P 2,4 /tmp/bastos-qs/quickstart-book.pdf
   ```

---

## 3. Découpe et pli

Chaque face A4 porte **4 pages A6** en grille 2×2 :

```
A4 portrait imprimée (vue de dessus) :

┌──────────┬──────────┐
│          │          │
│          │          │  ← strip A (haut)
│          │          │
├──────────┼──────────┤  ← couper ici (horizontalement)
│          │          │
│          │          │  ← strip B (bas)
│          │          │
└──────────┴──────────┘
           ↑
     plier ici (chaque strip)
```

Pour chaque feuille A4 :
1. Couper **horizontalement** au milieu → 2 bandes (strips) A5 paysage.
2. Plier chaque strip en deux **verticalement** (bord droit rabattu sur bord gauche).
   Le pli forme le dos du feuillet ; les pages s'ouvrent vers la droite.

## 4. Assemblage

Les 4 strips (2 par feuille × 2 feuilles) s'emboîtent dans l'ordre :

```
Feuille 1, strip haut  →  feuillet le plus extérieur
Feuille 1, strip bas   →  s'insère à l'intérieur du précédent
Feuille 2, strip haut  →  s'insère à l'intérieur du précédent
Feuille 2, strip bas   →  feuillet le plus intérieur
```

1. Plier tous les strips.
2. Glisser strip bas de feuille 2 (innermost) dans strip haut de feuille 2.
3. Glisser cet ensemble dans strip bas de feuille 1.
4. Glisser le tout dans strip haut de feuille 1 (outermost).
5. Aligner les plis — c'est le dos du livret.
6. Agrafer ou coudre au dos.

> Le livret est correctement assemblé si les numéros de page se suivent en
> feuilletant de la couverture à la dernière page.
