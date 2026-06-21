#!/usr/bin/env python3
"""
Remplace les blocs ```mermaid``` par des images PNG générées via mmdc.
Usage: python3 preprocess-mermaid.py <fichier.md> <répertoire-sortie> [puppeteer-config.json]
Écrit le markdown modifié sur stdout.
"""
import re
import sys
import os
import subprocess
import tempfile


def convert_block(code, output_dir, index, puppeteer_config=None):
    img_name = f"mermaid_{index:02d}.png"
    img_path = os.path.join(output_dir, img_name)

    with tempfile.NamedTemporaryFile(mode='w', suffix='.mmd', delete=False) as f:
        f.write(code.strip())
        tmp_mmd = f.name

    cmd = ['mmdc', '-i', tmp_mmd, '-o', img_path, '-b', 'white']
    if puppeteer_config:
        cmd += ['-p', puppeteer_config]

    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
    except FileNotFoundError:
        sys.stderr.write(
            "ERREUR: mmdc introuvable.\n"
            "Installez mermaid-cli :\n"
            "  sudo apt install npm\n"
            "  npm install -g @mermaid-js/mermaid-cli\n"
        )
        sys.exit(1)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"ERREUR mmdc (bloc {index}) :\n{e.stderr}\n")
        raise
    finally:
        os.unlink(tmp_mmd)

    return img_path


def process(md_file, output_dir, puppeteer_config=None):
    with open(md_file, 'r') as f:
        content = f.read()

    counter = [0]

    def replace(m):
        path = convert_block(m.group(1), output_dir, counter[0], puppeteer_config)
        counter[0] += 1
        return f'![Diagramme]({path})'

    content = re.sub(r'```mermaid\n(.*?)```', replace, content, flags=re.DOTALL)

    content = content.replace('<!-- center-on -->', '```{=latex}\n\\begin{center}\n```')
    content = content.replace('<!-- center-off -->', '```{=latex}\n\\end{center}\n```')
    content = content.replace('<!-- pagebreak -->', '```{=latex}\n\\newpage\n```')

    return content


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <fichier.md> <répertoire-sortie> [puppeteer-config.json]",
              file=sys.stderr)
        sys.exit(1)
    puppeteer_config = sys.argv[3] if len(sys.argv) >= 4 else None
    print(process(sys.argv[1], sys.argv[2], puppeteer_config), end='')
