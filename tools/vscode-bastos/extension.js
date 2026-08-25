const vscode = require("vscode");
const { renumberSelection } = require("./src/renumber.js");

async function renumberLinesCommand() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== "bastos") {
    vscode.window.showErrorMessage(
      "BASTOS: ouvrez un fichier .bas et placez le curseur ou une sélection dessus."
    );
    return;
  }

  const selection = editor.selection;
  const selStart = selection.start.line;
  const selEnd = selection.isEmpty ? selection.start.line : selection.end.line;

  const document = editor.document;
  const allLines = [];
  for (let i = 0; i < document.lineCount; i++) {
    allLines.push(document.lineAt(i).text);
  }

  const defaultStart = (() => {
    const m = /^(\s*)(\d+)\b/.exec(allLines[selStart]);
    return m ? m[2] : "10";
  })();

  const startInput = await vscode.window.showInputBox({
    prompt: "Numéro de départ",
    value: defaultStart,
    validateInput: (v) =>
      /^\d+$/.test(v) && parseInt(v, 10) > 0 ? null : "Entier positif requis",
  });
  if (startInput === undefined) return;

  const incrementInput = await vscode.window.showInputBox({
    prompt: "Incrément",
    value: "10",
    validateInput: (v) =>
      /^\d+$/.test(v) && parseInt(v, 10) > 0 ? null : "Entier positif requis",
  });
  if (incrementInput === undefined) return;

  const start = parseInt(startInput, 10);
  const increment = parseInt(incrementInput, 10);

  const result = renumberSelection(allLines, selStart, selEnd, start, increment);
  if (!result.ok) {
    vscode.window.showErrorMessage("BASTOS: " + result.error);
    return;
  }

  await editor.edit((editBuilder) => {
    for (const [lineIndex, newText] of result.edits) {
      editBuilder.replace(document.lineAt(lineIndex).range, newText);
    }
  });

  if (result.warnings.length > 0) {
    const details = result.warnings
      .map((w) => `ligne ${w.line}: ${w.text}`)
      .join("\n");
    vscode.window
      .showWarningMessage(
        `BASTOS: ${result.warnings.length} cible(s) calculée(s) référencent peut-être une ligne renumérotée et n'ont pas pu être mises à jour automatiquement — à vérifier à la main.`,
        "Voir le détail"
      )
      .then((choice) => {
        if (choice === "Voir le détail") {
          const channel = vscode.window.createOutputChannel("BASTOS");
          channel.appendLine(details);
          channel.show();
        }
      });
  }
}

function activate(context) {
  context.subscriptions.push(
    vscode.commands.registerCommand("bastos.renumberLines", renumberLinesCommand)
  );
}

function deactivate() {}

module.exports = { activate, deactivate };
