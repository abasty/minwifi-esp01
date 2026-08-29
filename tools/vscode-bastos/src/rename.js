// Pure rename logic for BASTOS variables and labels, with no dependency on
// the `vscode` module so it can be unit-tested with plain Node.
//
// BASTOS has no scoping: every variable is global (see
// docs/BASTOS-MANUAL-fr.md), so renaming one is just "find every matching
// identifier token in the whole document" — no call-graph or block
// analysis needed. Variable names are also case-insensitive to the real
// tokenizer (it upper-cases every identifier character as it scans, see
// tokenize_keyword() in lib/basic/token.c-static), so "srv$", "SRV$" and
// "Srv$" all refer to the very same variable and must all be renamed
// together.
//
// The trailing "$" of a string variable is never editable: it is what
// makes the variable a string instead of a number, and letting a rename
// drop or add it would silently change the variable's type everywhere.
//
// Labels (LABEL "name", and the "name" after GOTO/GOSUB/THEN/ELSE) are a
// different case: they are ordinary string literals, not identifiers, so
// unlike variables they are matched byte-for-byte — case-sensitively, no
// tokenizer upper-casing involved. Only the string that immediately
// follows one of those five keywords counts as a label reference; the same
// text elsewhere (e.g. a PRINT message) is left alone.

const { tokenizeFullLine } = require("./format.js");
const { KEYWORD_SET } = require("./keywords.js");

const VALID_BASE_NAME_RE = /^[A-Za-z][A-Za-z0-9]*$/;
const VALID_LABEL_RE = /^[^"\\\r\n]+$/;
const LABEL_KEYWORDS = new Set(["GOTO", "GOSUB", "LABEL", "THEN", "ELSE"]);

// { name, hasDollar, upperName, range: {start, end} } for the variable
// token under `character` on `line`, or null if there isn't one. `range`
// excludes a trailing "$" so callers only ever edit the renameable part.
function findVariableAt(allLines, line, character) {
  if (line < 0 || line >= allLines.length) return null;
  const tokens = tokenizeFullLine(allLines[line]);
  for (const t of tokens) {
    if (t.kind !== "variable") continue;
    if (character < t.start || character > t.end) continue;
    const hasDollar = t.text.endsWith("$");
    const base = hasDollar ? t.text.slice(0, -1) : t.text;
    return {
      name: t.text,
      hasDollar,
      upperName: base.toUpperCase(),
      range: { start: t.start, end: hasDollar ? t.end - 1 : t.end },
    };
  }
  return null;
}

// Every {line, start, end} occurrence (range excludes "$") of the variable
// identified by (upperName, hasDollar) across the whole document.
function findAllOccurrences(allLines, upperName, hasDollar) {
  const occurrences = [];
  for (let i = 0; i < allLines.length; i++) {
    const tokens = tokenizeFullLine(allLines[i]);
    for (const t of tokens) {
      if (t.kind !== "variable") continue;
      const tHasDollar = t.text.endsWith("$");
      if (tHasDollar !== hasDollar) continue;
      const base = tHasDollar ? t.text.slice(0, -1) : t.text;
      if (base.toUpperCase() !== upperName) continue;
      occurrences.push({ line: i, start: t.start, end: tHasDollar ? t.end - 1 : t.end });
    }
  }
  return occurrences;
}

/**
 * @param {string[]} allLines
 * @param {number} line - 0-based
 * @param {number} character - 0-based
 * @param {string} newName - the replacement base name, without "$"
 * @returns {{ok: true, edits: {line:number, start:number, end:number, newText:string}[]}
 *          | {ok: false, error: string}}
 */
function renameVariable(allLines, line, character, newName) {
  const found = findVariableAt(allLines, line, character);
  if (!found) {
    return { ok: false, error: "Le curseur ne pointe pas sur une variable BASTOS." };
  }

  if (!VALID_BASE_NAME_RE.test(newName)) {
    return {
      ok: false,
      error:
        "Nom de variable invalide : une lettre suivie de lettres/chiffres uniquement (pas de _ en BASTOS).",
    };
  }

  const upperNew = newName.toUpperCase();
  const keywordForm = found.hasDollar ? upperNew + "$" : upperNew;
  if (KEYWORD_SET.has(keywordForm)) {
    return { ok: false, error: `"${keywordForm}" est un mot-clé réservé BASTOS.` };
  }

  if (upperNew !== found.upperName) {
    const collisions = findAllOccurrences(allLines, upperNew, found.hasDollar);
    if (collisions.length > 0) {
      return {
        ok: false,
        error: `Une autre variable "${newName}${found.hasDollar ? "$" : ""}" existe déjà dans ce fichier — renommer fusionnerait les deux.`,
      };
    }
  }

  const occurrences = findAllOccurrences(allLines, found.upperName, found.hasDollar);
  const edits = occurrences.map((o) => ({ ...o, newText: newName }));
  return { ok: true, edits };
}

// { text, range: {start, end} } for the label token under `character` on
// `line` (a string literal right after GOTO/GOSUB/LABEL/THEN/ELSE), or
// null if there isn't one. `range` excludes the surrounding quotes.
function findLabelAt(allLines, line, character) {
  if (line < 0 || line >= allLines.length) return null;
  const tokens = tokenizeFullLine(allLines[line]);
  for (let i = 1; i < tokens.length; i++) {
    const t = tokens[i];
    if (t.kind !== "string") continue;
    if (tokens[i - 1].kind !== "keyword" || !LABEL_KEYWORDS.has(tokens[i - 1].text)) continue;
    if (character < t.start || character > t.end) continue;
    return {
      text: t.text.slice(1, -1),
      range: { start: t.start + 1, end: t.end - 1 },
    };
  }
  return null;
}

// Every {line, start, end} occurrence (range excludes the quotes) of the
// label `labelText` used as a jump target across the whole document.
// Case-sensitive, unlike variables.
function findAllLabelOccurrences(allLines, labelText) {
  const occurrences = [];
  for (let i = 0; i < allLines.length; i++) {
    const tokens = tokenizeFullLine(allLines[i]);
    for (let j = 1; j < tokens.length; j++) {
      const t = tokens[j];
      if (t.kind !== "string") continue;
      if (tokens[j - 1].kind !== "keyword" || !LABEL_KEYWORDS.has(tokens[j - 1].text)) continue;
      if (t.text.slice(1, -1) !== labelText) continue;
      occurrences.push({ line: i, start: t.start + 1, end: t.end - 1 });
    }
  }
  return occurrences;
}

/**
 * @param {string[]} allLines
 * @param {number} line - 0-based
 * @param {number} character - 0-based
 * @param {string} newName - the replacement label text, without quotes
 * @returns {{ok: true, edits: {line:number, start:number, end:number, newText:string}[]}
 *          | {ok: false, error: string}}
 */
function renameLabel(allLines, line, character, newName) {
  const found = findLabelAt(allLines, line, character);
  if (!found) {
    return { ok: false, error: "Le curseur ne pointe pas sur une étiquette BASTOS." };
  }

  if (!VALID_LABEL_RE.test(newName)) {
    return {
      ok: false,
      error: 'Nom d\'étiquette invalide : ne doit pas être vide ni contenir " ou \\.',
    };
  }

  if (newName !== found.text) {
    const collisions = findAllLabelOccurrences(allLines, newName);
    if (collisions.length > 0) {
      return {
        ok: false,
        error: `Une autre étiquette "${newName}" existe déjà dans ce fichier — renommer fusionnerait les deux.`,
      };
    }
  }

  const occurrences = findAllLabelOccurrences(allLines, found.text);
  const edits = occurrences.map((o) => ({ ...o, newText: newName }));
  return { ok: true, edits };
}

// Unified entry point used by the editor integration: tries a variable
// first, then a label, at the given position.
function findRenameTargetAt(allLines, line, character) {
  const variable = findVariableAt(allLines, line, character);
  if (variable) return { type: "variable", ...variable };
  const label = findLabelAt(allLines, line, character);
  if (label) return { type: "label", ...label };
  return null;
}

function renameAt(allLines, line, character, newName) {
  const target = findRenameTargetAt(allLines, line, character);
  if (!target) {
    return { ok: false, error: "Le curseur ne pointe pas sur une variable ou une étiquette BASTOS." };
  }
  return target.type === "variable"
    ? renameVariable(allLines, line, character, newName)
    : renameLabel(allLines, line, character, newName);
}

module.exports = {
  findVariableAt,
  findAllOccurrences,
  renameVariable,
  findLabelAt,
  findAllLabelOccurrences,
  renameLabel,
  findRenameTargetAt,
  renameAt,
};
