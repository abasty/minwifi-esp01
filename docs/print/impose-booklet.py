#!/usr/bin/env python3
"""
Impose A6 pages for a "2-strip saddle-stitch" booklet on A4 portrait.

Each A4 sheet side has 4 A6 pages in a 2×2 grid:

  ┌─────────┬─────────┐
  │         │         │  ← strip supérieure
  ├─────────┼─────────┤  ← découper ici
  │         │         │  ← strip inférieure
  └─────────┴─────────┘

Impression  : pages impaires (rectos), retourner sur le BORD LONG, pages paires (versos).
Découpe     : couper chaque A4 horizontalement en 2 strips.
Pliage      : plier chaque strip verticalement (bord droit rabattu sur bord gauche).
Assemblage  : feuille 1 strip haut > feuille 1 strip bas > feuille 2 strip haut > ...

Usage: python3 impose-booklet.py <input.pdf> <output.pdf>
"""
import sys
import math
import os
import subprocess
from pypdf import PdfWriter, PdfReader


def pad_to(input_path, output_path, n_target):
    reader = PdfReader(input_path)
    writer = PdfWriter()
    for page in reader.pages:
        writer.add_page(page)
    w = float(reader.pages[0].mediabox.width)
    h = float(reader.pages[0].mediabox.height)
    for _ in range(n_target - len(reader.pages)):
        writer.add_blank_page(width=w, height=h)
    with open(output_path, 'wb') as f:
        writer.write(f)


def booklet_order(n):
    """
    Compute 4-up (2x2) page order for 2-strip saddle-stitch.
    n must be a multiple of 8.

    For each sheet k, 2 strips (s1=2k-1, s2=2k):
      strip s: front-left = n-2(s-1), front-right = 2(s-1)+1
               back-left  = 2(s-1)+2, back-right  = n-2(s-1)-1

    RECTO page order: [fl1, fr1, fl2, fr2]  (row-major, fits 2x2 grid)
    VERSO page order: [bl1, br1, bl2, br2]

    With long-edge flip: VERSO-left prints behind RECTO-right,
    so front covers (fr) get their inner pages (bl) correctly.
    """
    pages = []
    for k in range(1, n // 8 + 1):
        s1, s2 = 2 * k - 1, 2 * k
        fl1, fr1 = n - 2 * (s1 - 1), 2 * (s1 - 1) + 1
        bl1, br1 = 2 * (s1 - 1) + 2, n - 2 * (s1 - 1) - 1
        fl2, fr2 = n - 2 * (s2 - 1), 2 * (s2 - 1) + 1
        bl2, br2 = 2 * (s2 - 1) + 2, n - 2 * (s2 - 1) - 1
        pages.extend([fl1, fr1, fl2, fr2])  # recto
        pages.extend([bl1, br1, bl2, br2])  # verso
    return pages


def impose(input_pdf, output_pdf):
    n_src = len(PdfReader(input_pdf).pages)
    n = math.ceil(n_src / 8) * 8
    n_sheets = n // 8

    print(f"  Pages A6 : {n_src} → {n} ({n - n_src} pages vierges ajoutées)")
    print(f"  Feuilles A4 : {n_sheets}")

    workdir = os.path.dirname(os.path.abspath(output_pdf))
    padded = os.path.join(workdir, '_padded.pdf')
    pad_to(input_pdf, padded, n)

    order = booklet_order(n)
    page_spec = ','.join(str(p) for p in order)

    r = subprocess.run(
        ['pdfjam', '--nup', '2x2', '--paper', 'a4paper',
         '--quiet', '--outfile', output_pdf, padded, page_spec],
        capture_output=True, text=True
    )
    os.unlink(padded)

    if r.returncode != 0:
        sys.stderr.write(f"ERREUR pdfjam:\n{r.stderr}\n")
        sys.exit(1)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.pdf> <output.pdf>", file=sys.stderr)
        sys.exit(1)
    impose(sys.argv[1], sys.argv[2])
