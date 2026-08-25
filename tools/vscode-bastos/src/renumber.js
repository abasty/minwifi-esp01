// Pure renumbering logic for BASTOS programs, with no dependency on the
// `vscode` module so it can be unit-tested with plain Node.
//
// BASTOS line numbers are plain integers at the start of a line. Jump
// targets appear after GOTO / GOSUB / THEN / ELSE, either as a bare line
// number or as a quoted label (labels never need renumbering). A target can
// also be a *computed* expression such as `GOSUB 2000+(b+c-1)*10`: we must
// not rewrite the "2000" there, since it is not a literal jump but the base
// of an arithmetic expression — instead we warn about it so the user can
// check it by hand.

const LINE_NUMBER_RE = /^(\s*)(\d+)\b/;
const REF_RE = /\b(GOTO|GOSUB|THEN|ELSE)(\s+)(\d+)\b(?!\s*[+\-*/%&|])/gi;
const COMPUTED_REF_RE = /\b(GOTO|GOSUB)(\s+)(\d+)\s*[+\-*/%&|]/gi;

// Returns a list of [start, end) ranges of `line` that are neither inside a
// double-quoted string nor inside a comment (REM ... / ' ...), i.e. the
// ranges it is safe to look for keywords in.
function safeSegments(line) {
  const segments = [];
  const n = line.length;
  let i = 0;
  let segStart = 0;

  while (i < n) {
    const ch = line[i];

    if (ch === '"') {
      segments.push([segStart, i]);
      i++; // skip opening quote
      while (i < n) {
        if (line[i] === '\\') {
          i += 2;
          continue;
        }
        if (line[i] === '"') {
          i++;
          break;
        }
        i++;
      }
      segStart = i;
      continue;
    }

    if (ch === "'") {
      segments.push([segStart, i]);
      return segments; // rest of the line is a comment
    }

    if ((ch === 'R' || ch === 'r') && /^rem\b/i.test(line.slice(i))) {
      const prev = i > 0 ? line[i - 1] : '';
      if (!/[A-Za-z0-9_]/.test(prev)) {
        segments.push([segStart, i]);
        return segments; // REM comment: rest of the line is ignored
      }
    }

    i++;
  }

  segments.push([segStart, n]);
  return segments;
}

// Applies a set of {start, end, replacement} edits to `line`, back to front
// so earlier offsets stay valid.
function applyEdits(line, edits) {
  let out = line;
  const sorted = [...edits].sort((a, b) => b.start - a.start);
  for (const e of sorted) {
    out = out.slice(0, e.start) + e.replacement + out.slice(e.end);
  }
  return out;
}

/**
 * @param {string[]} allLines - every line of the document, no EOL chars
 * @param {number} selStart - first selected line index (0-based, inclusive)
 * @param {number} selEnd - last selected line index (0-based, inclusive)
 * @param {number} start - first new line number
 * @param {number} increment - step between renumbered lines
 * @returns {{ok: true, edits: Map<number,string>, warnings: {line:number, text:string}[]}
 *          | {ok: false, error: string}}
 */
function renumberSelection(allLines, selStart, selEnd, start, increment) {
  if (!Number.isInteger(start) || start <= 0) {
    return { ok: false, error: "Numéro de départ invalide." };
  }
  if (!Number.isInteger(increment) || increment <= 0) {
    return { ok: false, error: "Incrément invalide." };
  }

  const targets = [];
  for (let i = selStart; i <= selEnd; i++) {
    const m = LINE_NUMBER_RE.exec(allLines[i]);
    if (m) {
      targets.push({
        index: i,
        oldNum: parseInt(m[2], 10),
        leading: m[1],
        numLen: m[2].length,
      });
    }
  }
  if (targets.length === 0) {
    return { ok: false, error: "Aucune ligne numérotée dans la sélection." };
  }

  const oldToNew = new Map();
  targets.forEach((t, idx) => {
    t.newNum = start + idx * increment;
    oldToNew.set(t.oldNum, t.newNum);
  });

  const selectedIndexSet = new Set(targets.map((t) => t.index));
  const otherNumbers = new Set();
  for (let i = 0; i < allLines.length; i++) {
    if (selectedIndexSet.has(i)) continue;
    const m = LINE_NUMBER_RE.exec(allLines[i]);
    if (m) otherNumbers.add(parseInt(m[2], 10));
  }

  const collisions = targets
    .filter((t) => otherNumbers.has(t.newNum))
    .map((t) => `${t.oldNum} → ${t.newNum} (déjà utilisé hors sélection)`);
  if (collisions.length > 0) {
    return {
      ok: false,
      error: "Collision de numéros de ligne : " + collisions.join(", "),
    };
  }

  const targetByIndex = new Map(targets.map((t) => [t.index, t]));
  const edits = new Map();
  const warnings = [];

  for (let i = 0; i < allLines.length; i++) {
    let text = allLines[i];
    const t = targetByIndex.get(i);
    if (t) {
      text =
        t.leading +
        String(t.newNum) +
        text.slice(t.leading.length + t.numLen);
    }

    const segments = safeSegments(text);
    const refEdits = [];
    for (const [segStart, segEnd] of segments) {
      const chunk = text.slice(segStart, segEnd);

      REF_RE.lastIndex = 0;
      let m;
      while ((m = REF_RE.exec(chunk))) {
        const oldNum = parseInt(m[3], 10);
        if (oldToNew.has(oldNum)) {
          const numStart = segStart + m.index + m[1].length + m[2].length;
          refEdits.push({
            start: numStart,
            end: numStart + m[3].length,
            replacement: String(oldToNew.get(oldNum)),
          });
        }
      }

      COMPUTED_REF_RE.lastIndex = 0;
      while ((m = COMPUTED_REF_RE.exec(chunk))) {
        const oldNum = parseInt(m[3], 10);
        if (oldToNew.has(oldNum)) {
          warnings.push({ line: i + 1, text: text.trim() });
        }
      }
    }

    if (refEdits.length > 0) {
      text = applyEdits(text, refEdits);
    }

    if (text !== allLines[i]) {
      edits.set(i, text);
    }
  }

  return { ok: true, edits, warnings };
}

module.exports = { renumberSelection, safeSegments };
