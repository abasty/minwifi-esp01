// Pure formatting logic for BASTOS source lines, with no dependency on the
// `vscode` module so it can be unit-tested with plain Node.
//
// Rules:
//   1. Keywords (instructions, functions, control structures, word
//      operators AND/OR/NOT) are uppercased, and always get exactly one
//      space before and after them, regardless of their neighbour —
//      GOTO"x" / LABEL"x" / THEN"x" become GOTO "x" / LABEL "x" / THEN "x".
//   2. Variable names are lowercased.
//   3/4. Everywhere else (numbers, variables, strings, punctuation,
//      operators), whitespace is removed except the single space that is
//      structurally required between two tokens that would otherwise merge
//      into a different token — i.e. wherever the real tokenizer scans a
//      contiguous run of [0-9A-Za-z$] (see is_char_of_keyword() in
//      lib/basic/token.c-static): "10 END" needs its space (a line number
//      and a keyword would merge into one identifier), "C=7" does not (=
//      is never part of an identifier run).
//
// Comments (`REM ...` and `' ...`) are never touched beyond uppercasing a
// leading REM keyword itself: their content is copied verbatim, exactly as
// written, since it has no effect on execution and reformatting it would
// only destroy the author's own formatting.

const { KEYWORD_SET } = require("./keywords.js");

const ID_CHAR_RE = /[0-9A-Za-z$]/;
const LEADING_LINE_NUMBER_RE = /^(\s*)(\d+)/;

function isIdChar(ch) {
  return ch !== undefined && ID_CHAR_RE.test(ch);
}

// Tokenizes the *code* portion of a line (i.e. before any comment) into
// {text, kind, start, end} tokens, mirroring lib/basic/token.c-static's
// tokenize(). kind is one of "string" | "number" | "keyword" | "variable" |
// "punct". start/end are offsets into `line` (the substring passed in, not
// necessarily the whole document line — see tokenizeFullLine for that).
// Returns { tokens, commentTail, commentStart } where commentTail is the
// raw, untouched remainder of the original line starting at a comment
// marker (or null if the line has no comment), and commentStart is its
// offset into `line`.
function tokenizeCode(line) {
  const tokens = [];
  const n = line.length;
  let i = 0;

  while (i < n) {
    const ch = line[i];

    if (ch === " " || ch === "\t") {
      i++;
      continue;
    }

    if (ch === "'") {
      return { tokens, commentTail: line.slice(i), commentStart: i };
    }

    if (ch === '"') {
      let j = i + 1;
      while (j < n) {
        if (line[j] === "\\") {
          j += 2;
          continue;
        }
        if (line[j] === '"') {
          j++;
          break;
        }
        j++;
      }
      tokens.push({ text: line.slice(i, j), kind: "string", start: i, end: j });
      i = j;
      continue;
    }

    if (ch >= "0" && ch <= "9") {
      let j = i;
      if (ch === "0" && (line[j + 1] === "x" || line[j + 1] === "X")) {
        j += 2;
        while (j < n && /[0-9A-Fa-f]/.test(line[j])) j++;
      } else {
        while (j < n && /[0-9]/.test(line[j])) j++;
        if (line[j] === "." && /[0-9]/.test(line[j + 1] || "")) {
          j++;
          while (j < n && /[0-9]/.test(line[j])) j++;
        }
      }
      tokens.push({ text: line.slice(i, j), kind: "number", start: i, end: j });
      i = j;
      continue;
    }

    if (/[A-Za-z]/.test(ch)) {
      let j = i;
      while (j < n && isIdChar(line[j])) j++;
      const word = line.slice(i, j);
      const upper = word.toUpperCase();

      if (upper === "REM") {
        // REM's payload is technically tokenized by the real interpreter
        // too, but it is never executed and only ever meant to be read as
        // a comment — leave it exactly as the author wrote it.
        tokens.push({ text: "REM", kind: "keyword", start: i, end: j });
        return { tokens, commentTail: line.slice(j) || null, commentStart: j };
      }

      const isKeyword = KEYWORD_SET.has(upper);
      const text = isKeyword ? upper : word.toLowerCase();
      tokens.push({ text, kind: isKeyword ? "keyword" : "variable", start: i, end: j });
      i = j;
      continue;
    }

    if (ch === "<" || ch === ">") {
      const two = line.slice(i, i + 2);
      if (two === "<>" || two === "<=" || two === ">=") {
        tokens.push({ text: two, kind: "punct", start: i, end: i + 2 });
        i += 2;
        continue;
      }
      tokens.push({ text: ch, kind: "punct", start: i, end: i + 1 });
      i++;
      continue;
    }

    if ("=+-*/%&|(),;:?".includes(ch)) {
      tokens.push({ text: ch, kind: "punct", start: i, end: i + 1 });
      i++;
      continue;
    }

    // Unrecognized character (shouldn't happen in valid BASTOS source):
    // pass it through unchanged rather than corrupting the line.
    tokens.push({ text: ch, kind: "punct", start: i, end: i + 1 });
    i++;
  }

  return { tokens, commentTail: null, commentStart: -1 };
}

// Tokenizes a *whole* document line (leading line number included) into
// tokens with start/end offsets absolute within that line, plus a trailing
// "comment" token if the line has one. Used by features that need to know
// *where* a token sits in the document (e.g. rename), unlike formatLine
// which only needs token order.
function tokenizeFullLine(line) {
  const m = LEADING_LINE_NUMBER_RE.exec(line);
  const prefixLen = m ? m[0].length : 0;
  const rest = line.slice(prefixLen);

  const { tokens, commentTail, commentStart } = tokenizeCode(rest);

  const result = [];
  if (m) {
    result.push({ text: m[2], kind: "number", start: prefixLen - m[2].length, end: prefixLen });
  }
  for (const t of tokens) {
    result.push({ text: t.text, kind: t.kind, start: t.start + prefixLen, end: t.end + prefixLen });
  }
  if (commentTail !== null) {
    result.push({
      text: commentTail,
      kind: "comment",
      start: commentStart + prefixLen,
      end: line.length,
    });
  }
  return result;
}

function isValueLike(kind) {
  return kind === "string" || kind === "number" || kind === "variable";
}

// A space is inserted between two tokens if:
//   - gluing them together would merge two identifier-charset runs into a
//     different token (e.g. a line number directly against a keyword), or
//   - exactly one side is a keyword and the other is a value (string,
//     number or variable) — a keyword always stands out from the value it
//     operates on: GOTO "x", "Q" OR, 1 TO, THEN "SaisieChoix".
// Two punctuation/separator characters (`:` `;` `,` `(` `)`) never force a
// space around a keyword: ":GOSUB", "0;AT", "CLS:END" stay tight, matching
// the compact style already used throughout disk/*.bas.
function needsSpaceBetween(prev, next) {
  if (isIdChar(prev.text[prev.text.length - 1]) && isIdChar(next.text[0])) {
    return true;
  }
  const prevKw = prev.kind === "keyword";
  const nextKw = next.kind === "keyword";
  if (prevKw && isValueLike(next.kind)) return true;
  if (nextKw && isValueLike(prev.kind)) return true;
  return false;
}

function joinTokens(tokens) {
  let out = "";
  let prev = null;
  for (const t of tokens) {
    if (t.text === "") continue;
    if (prev && needsSpaceBetween(prev, t)) {
      out += " ";
    }
    out += t.text;
    prev = t;
  }
  return { text: out, lastToken: prev };
}

function formatLine(line) {
  const m = LEADING_LINE_NUMBER_RE.exec(line);
  const lineNumber = m ? m[2] : null;
  const rest = m ? line.slice(m[0].length) : line;

  const { tokens, commentTail } = tokenizeCode(rest);

  const codeTokens = lineNumber !== null
    ? [{ text: lineNumber, kind: "number" }, ...tokens]
    : tokens;

  const { text: out, lastToken } = joinTokens(codeTokens);

  if (commentTail === null) {
    return out;
  }

  // commentTail is raw/verbatim: if it already starts with whitespace
  // (its own original separation from the code), never add a second space
  // on top of it. A REM's payload is really tokenized as a value by the
  // real interpreter (usually a string), so it behaves like one for
  // spacing purposes ("REM "x""); a bare "'" marker is not a value, so it
  // stays tight against a keyword just like ":" or ";" ("END'x").
  const isRemTail = lastToken !== null && lastToken.kind === "keyword" && lastToken.text === "REM";
  const alreadySpaced = commentTail[0] === " " || commentTail[0] === "\t";
  const commentAsToken = { text: commentTail, kind: isRemTail ? "string" : "punct" };
  const needsSpace =
    !alreadySpaced && lastToken !== null && needsSpaceBetween(lastToken, commentAsToken);
  return out + (needsSpace ? " " : "") + commentTail;
}

function formatDocument(allLines) {
  const edits = new Map();
  for (let i = 0; i < allLines.length; i++) {
    const formatted = formatLine(allLines[i]);
    if (formatted !== allLines[i]) {
      edits.set(i, formatted);
    }
  }
  return edits;
}

module.exports = { formatLine, formatDocument, tokenizeFullLine };
