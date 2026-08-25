#!/usr/bin/env node

// ============================================================
// gen-panel.mjs — VCV Rack panel SVG generator
//
// Parses a VCV Rack module's C++ source file, extracts widget
// positions (knobs, jacks, switches, lights, etc.), and emits
// a panel SVG ready for use in Rack.
//
// Usage:
//   Single module (writes res/ra-foo.svg):
//     node util/gen-panel.mjs ra-foo
//
//   Generate all modules (writes res/*.svg, assumes modules are in /src):
//     node util/gen-panel.mjs --all
//
// Flags:
//   --all              Batch-generate SVGs for every src/ra-*.cpp file
//   --hp=N             Override HP width (otherwise auto-detected from SVG)
//   --input-color=#hex Set jack color for inputs  (default #14B274)
//   --output-color=#hex Set jack color for outputs (default #FFB93D)
//   --bg-start=#hex    Top color of background gradient (default #423558)
//   --bg-end=#hex      Bottom color of background gradient (default #221421)
//   --bg-mid=0-100     Midpoint of gradient as % from top (default 33)
//
// Examples:
//   node util/gen-panel.mjs ra-vca
//   node util/gen-panel.mjs --all
//   node util/gen-panel.mjs --hp=6 --input-color=#ff0000 ra-foo
//
// ============================================================

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import * as opentype from 'opentype.js';

// ============================================================
// Configuration — default visual style
// Overridable via CLI flags (see above).
// ============================================================

let CFG = {
  height: 128.5, // Panel height in mm (standard 3U); override with --height=NNN
  strokeWidth: 1.5, // Width of all drawn lines
  bg: { // Background gradient
    start: '#303031', //#423558, #332832
    end: '#232324',//'#221721',
    mid: 33,
  },
  colors: {
    input: '#996dd2',
    output: '#c8b7c7',
  },
};

// ============================================================
// Widget geometry registry
//
// Maps C++ widget type names to their physical dimensions and
// rendering category. Used during SVG generation to draw the
// correct shape and size for each component.
//
// Fields:
//   kind  — rendering category (jack, knob, switch, button, etc.)
//   rad   — radius in rack-units (for round components)
//   hw/hh — half-width / half-height in rack-units (for rect components)
// ============================================================
const WIDGET_INFO = {
  RaPort: { kind: 'jack', rad: 11.85 },
  RaKnob: { kind: 'knob', rad: 14.17 },
  RaKnobLarge: { kind: 'knob', rad: 18 },
  RaKnobSmall: { kind: 'knob', rad: 11.34 },
  RaKnobTrim: { kind: 'knob', rad: 8.93 },
  RaSwitch2: { kind: 'switch', hw: 7, hh: 10.32 },
  RaSwitch3: { kind: 'switch', hw: 6.73, hh: 14.17 },
  RaButton: { kind: 'button', rad: 9 },
  VCVLightBezel: { kind: 'bezel', rad: 10.65 },
  RaRGBLight: { kind: 'bezel', rad: 9 },
  MediumLight: { kind: 'bezel', rad: 9 },
  TinyLight: { kind: 'bezel', rad: 4 },
  VCVLightSlider: { kind: 'slider', hw: 7.5, hh: 20 },
  RaScrew: { kind: 'screw', rad: 7.5 },
};

// Map a widget type string (possibly templated like "VCVLightBezel<WhiteLight>")
// to its base key in WIDGET_INFO.
function resolveBaseType(typeStr) {
  const base = typeStr.replace(/<.*>/, '');
  // Try exact match first, then base-name match
  if (WIDGET_INFO[typeStr]) return WIDGET_INFO[typeStr];
  if (WIDGET_INFO[base]) return WIDGET_INFO[base];
  return null;
}

// Convert rack-units to millimetres (1hp = 5.08mm = 15 rack-units)
const ru2mm = (ru) => ru * 5.08 / 15;

// ============================================================
// Font-to-path rendering
//
// VCV Rack's SVG rasteriser ignores <text> elements, so all text
// is converted to vector paths at generate time with opentype.js
// using the bundled panel font (./font).
// ============================================================

const FONT_SIZE_NAME = 2.0;   // module name, mm (was 2.8)
const FONT_SIZE_LABEL = 1.4;  // control/port labels, mm (was 2.0)

// Resolve the panel font relative to this script: util/ -> repo root ./font
const scriptDir = path.dirname(fileURLToPath(import.meta.url));
let FONT_PATH = path.resolve(scriptDir, '../font/URWGothic-Book.otf');
let font = null;

function loadPanelFont(p) {
	font = null;
	try {
		const buf = fs.readFileSync(p);
		font = opentype.parse(buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength));
	} catch (e) {
		console.error('Warning: could not load panel font (' + p + '):', e.message);
	}
}

// Render a string to a single-line SVG <path> outline, horizontally centred
// on x with the baseline at y. Font sizes are in mm (SVG user units). Falls
// back to a <text> element if the font is unavailable.
function textToPath(text, x, y, fontSize, color, opacity = 1) {
	const op = (opacity < 1) ? ` opacity="${opacity}"` : '';
	if (font) {
		try {
			const advance = font.getAdvanceWidth(text, fontSize);
			const p = font.getPath(text, x - advance / 2, y, fontSize);
			const d = p.toPathData(2);
			if (d)
				return `<path d="${d}" fill="${color}"${op}/>`;
		} catch (e) { /* fall through to <text> */ }
	}
	return `<text x="${x.toFixed(2)}" y="${y.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="${fontSize}" text-anchor="middle"${op}>${text}</text>`;
}

// ============================================================
// Expression resolver
//
// Evaluates arithmetic expressions from C++ source at parse
// time, substituting known variables, array lookups, and
// Rack-specific constants (box.size.x, RACK_GRID_WIDTH, etc.).
// ============================================================
class ExprContext {
  constructor(HW) {
    this.vars = {};
    this.arrays = {};
    this.HW = HW; // box.size.x in rack units
  }

  setVar(name, value) { this.vars[name] = value; }
  setArray(name, values) { this.arrays[name] = values; }

  resolve(expr) {
    if (typeof expr === 'number') return expr;
    expr = expr.trim();

    // Strip C/C++ float literal suffixes (e.g. "20.f", "2.5f", "20f") so
    // they parse as plain numbers in both the number check and eval below.
    expr = expr.replace(/(\d)\.?f\b/gi, '$1');

    // Try direct variable lookup
    if (this.vars[expr] !== undefined) return this.vars[expr];

    // Try array lookup: y[i], colX[c], etc.
    const arrMatch = expr.match(/^(\w+)\[(.+)\]$/);
    if (arrMatch) {
      const arr = this.arrays[arrMatch[1]];
      const idx = this.resolve(arrMatch[2]);
      if (arr && idx >= 0 && idx < arr.length) return arr[idx];
      return 0;
    }

    // Try plain number
    const num = parseFloat(expr);
    if (!isNaN(num) && /^-?\d+(\.\d+)?$/.test(expr)) return num;

    // Complex expression — substitute known values and eval
    let cooked = expr;
    // Replace box.size expressions
    cooked = cooked.replace(/\bbox\.size\.x\b/g, String(this.HW));
    cooked = cooked.replace(/\bbox\.size\.y\b/g, '380');
    cooked = cooked.replace(/\bRACK_GRID_WIDTH\b/g, '15');

    // Replace known variables
    for (const [k, v] of Object.entries(this.vars)) {
      const re = new RegExp('\\b' + k + '\\b', 'g');
      cooked = cooked.replace(re, String(v));
    }

    // Resolve array accesses with numeric indices
    cooked = cooked.replace(/(\w+)\[(\d+)\]/g, (_, name, idx) => {
      const arr = this.arrays[name];
      if (arr && arr[+idx] !== undefined) return String(arr[+idx]);
      return '0';
    });

    // Try safe eval
    try {
      const result = Function('"use strict"; return (' + cooked + ')')();
      if (typeof result === 'number' && !isNaN(result)) return result;
    } catch { /* fall through */ }
    return 0;
  }
}

// ============================================================
// C++ Parser
//
// Reads a VCV Rack module source file and extracts:
//   - panel SVG path
//   - module name (from createModel slug)
//   - configParam/Input/Output display labels
//   - HP width (from SVG viewBox or estimated from layout)
//   - widget positions inside the Widget constructor
// ============================================================
function parseModule(filePath) {
  const src = fs.readFileSync(filePath, 'utf8');
  const lines = src.split('\n');

  // ---- Extract panel SVG path ----
  // Match: asset::plugin(pluginInstance, "res/ra-foo.svg")
  const panelMatch = src.match(/asset::plugin\(pluginInstance,\s*"([^"]+\.svg)"\)/);
  const panelSvg = panelMatch ? panelMatch[1] : null;

  // ---- Extract module name from createModel slug ----
  // Match: createModel<FooModule, FooWidget>("ra-foo")
  const modelMatch = src.match(/createModel<\w+,\s*\w+>\s*\(\s*"([^"]+)"\s*\)/);
  const moduleName = modelMatch ? modelMatch[1] : path.basename(filePath, '.cpp');

  // ---- Extract display labels from configParam/Input/Output calls ----
  // Builds a lookup: configNames["GAIN_PARAM"] = "Gain"
  const configNames = {};
  // Match: configParam(GAIN_PARAM, ..., "Gain", ...)
  const configParamRe = /configParam\(\s*(\w+)\s*,.*?"([^"]+?)"/g;
  let m;
  while ((m = configParamRe.exec(src)) !== null) configNames[m[1]] = m[2];
  // Match: configInput(IN_ID, "Audio In")
  const configInputRe = /configInput\(\s*(\w+)\s*,\s*"([^"]+?)"\s*\)/g;
  while ((m = configInputRe.exec(src)) !== null) configNames[m[1]] = m[2];
  // Match: configOutput(OUT_ID, "Audio Out")
  const configOutputRe = /configOutput\(\s*(\w+)\s*,\s*"([^"]+?)"\s*\)/g;
  while ((m = configOutputRe.exec(src)) !== null) configNames[m[1]] = m[2];

  // ---- Determine HP width & existing height ----
  // Preferred source: read the existing panel SVG's viewBox and compute
  // width = mmWidth / 5.08 (1hp = 5.08mm). The existing height is also
  // preserved so plain regenerations can't silently truncate tall panels.
  let HP = 0;
  let existingHeight = 0;
  if (panelSvg) {
    const svgDir = path.dirname(path.resolve(filePath));
    const svgFull = path.resolve(svgDir, '..', panelSvg);
    if (fs.existsSync(svgFull)) {
      const svgContent = fs.readFileSync(svgFull, 'utf8');
      const vbParts = svgContent.match(/viewBox="\s*([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)/);
      if (vbParts) {
        const mmWidth = parseFloat(vbParts[3]);
        HP = Math.round(mmWidth / 5.08);
        existingHeight = parseFloat(vbParts[4]);
      }
    }
  }

  // Fallback HP estimation: use the rightmost x-coordinate from Vec() calls.
  // This handles cases where no SVG exists yet (e.g. generating the first panel).
  if (!HP) {
    let maxX = 0;
    // Find all numeric x-coordinates in Vec(x, y) calls
    const xRe = /Vec\(\s*([\d.]+)\s*,/g;
    let xm;
    while ((xm = xRe.exec(src)) !== null) {
      const xv = parseFloat(xm[1]);
      if (xv > maxX) maxX = xv;
    }
    // Also detect expressions like box.size.x - RACK_GRID_WIDTH (right-edge placement)
    const offsetRe = /box\.size\.x\s*-\s*([\d.]+)/g;
    let om;
    while ((om = offsetRe.exec(src)) !== null) {
      const w = parseFloat(om[1]) + 10;
      if (w > maxX) maxX = w;
    }
    if (maxX > 0) {
      // Add 1hp (15 units) margin beyond the rightmost component
      const guessedHP = Math.ceil((maxX + 15) / 15);
      // Clamp to a reasonable range to avoid bogus values
      if (guessedHP >= 4 && guessedHP <= 12) HP = guessedHP;
    }
  }
  if (!HP) HP = 4;             // fallback default
  const HW = HP * 15;          // box.size.x in rack-units

  // ---- Isolate the Widget constructor body ----
  // Find the opening { of FooWidget::FooWidget(...) { ... }
  const widgetStartRe = /\b(\w+Widget)\s*\([^)]*\)\s*\{/;
  const consMatch = src.match(widgetStartRe);
  if (!consMatch) {
    console.error('Could not find Widget constructor in', filePath);
    process.exit(1);
  }
  const constructorStart = src.indexOf(consMatch[0]);
  // Extract everything between the outer { }
  const body = extractBraces(src, constructorStart + consMatch[0].length - 1);
  const bodyLines = body.split('\n');

  // ---- Walk the constructor body line by line ----
  // The ExprContext tracks local variables, arrays, and loop
  // counters so we can resolve expressions like y+28*i at parse time.
  const ctx = new ExprContext(HW);
  const components = [];

  // Process all lines — handles variable declarations, for-loops,
  // assignments, and component creation calls.
  processBlock(bodyLines, ctx, components);

  return {
    moduleName,
    panelSvg,
    HP,
    HW,
    existingHeight,
    components,
    configNames,
  };
}

// Extract the text between the first matching { } brace pair
// starting from openBraceIdx. Handles nested braces correctly.
function extractBraces(src, openBraceIdx) {
  let depth = 0;
  let start = -1;
  for (let i = openBraceIdx; i < src.length; i++) {
    if (src[i] === '{') {
      if (depth === 0) start = i + 1;
      depth++;
    } else if (src[i] === '}') {
      depth--;
      if (depth === 0) return src.slice(start, i);
    }
  }
  return src.slice(start);
}

// Process a block of C++ source lines, recognising:
//   - for loops          → unroll by iterating the loop variable
//   - variable decls     → store in ExprContext for later resolution
//   - array decls        → store array values in ExprContext
//   - compound assignment→ update tracked variables (y += 28)
//   - simple assignment  → update tracked variables (y = expr)
//   - component creation → parse via parseComponentLine() and collect
//
// loopVars are injected temporarily and cleaned up on exit, so
// recursive calls (e.g. loop unrolling) get fresh loop-iterator values.
function processBlock(lines, ctx, components, loopVars = {}) {
  // Merge loop variables into context temporarily
  for (const [k, v] of Object.entries(loopVars)) ctx.setVar(k, v);

  let i = 0;
  while (i < lines.length) {
    const line = lines[i].trim();

    // Skip empty lines and C++-style comments
    if (!line || line.startsWith('//')) { i++; continue; }

    // ---- For-loop detection and unrolling ----
    // Pattern: for (int VAR = START; VAR < END; VAR++)
    const forMatch = line.match(/^\s*for\s*\(\s*(?:int|float)\s+(\w+)\s*=\s*([^;]+);\s*\1\s*<\s*([^;]+);\s*(?:\1\+\+|--\1|\1\s*=\s*\1\s*\+[^)]*)\s*\)\s*\{?\s*$/);
    if (forMatch) {
      const varName = forMatch[1];
      const startVal = ctx.resolve(forMatch[2]);
      const endExpr = forMatch[3];

      // Collect the loop body by tracking brace depth
      let bodyLines = [];
      let braceCount = 0;
      let j = i;
      if (line.includes('{')) {
        braceCount = 1;
        j = i + 1;
      } else {
        j = i + 1;
        while (j < lines.length && !lines[j].trim().startsWith('{')) j++;
        if (j < lines.length) {
          braceCount = 1;
          j++;
        }
      }
      while (j < lines.length && braceCount > 0) {
        const bl = lines[j];
        for (const ch of bl) {
          if (ch === '{') braceCount++;
          else if (ch === '}') braceCount--;
        }
        if (braceCount > 0) bodyLines.push(bl);
        j++;
      }

      // Unroll: process the body once per iteration, injecting the loop variable
      const endVal = ctx.resolve(endExpr);
      if (isFinite(startVal) && isFinite(endVal)) {
        for (let iter = startVal; iter < endVal; iter++) {
          processBlock(bodyLines, ctx, components, { [varName]: iter });
        }
      }

      i = j;
      continue;
    }

    // ---- Display widget detection ----
    // Displays are created with patterns like:
    //   auto *display = new SomeDisplay();
    //   display->box.pos = Vec(x, y);
    //   display->box.size = Vec(w, h);
    // or with createWidget<SomeDisplay>(Vec(x, y))
    const dispMatch = line.match(/^\s*(?:\w+(?:\s*[*&]\s*|\s+))?\w+\s*=\s*(?:new\s+|createWidget<)(\w+Display)\s*[^(]*\(/);
    if (dispMatch) {
      // Look ahead a few lines for ->box.pos / ->box.size assignments
      let pos = null, size = null;
      for (let k = i; k < Math.min(i + 5, lines.length); k++) {
        const sub = lines[k].trim();
        const posM = sub.match(/->box\.pos\s*=\s*(?:mm2px\()?\s*Vec\(([^)]+)\)\s*\)?/);
        const szM = sub.match(/->box\.size\s*=\s*(?:mm2px\()?\s*Vec\(([^)]+)\)\s*\)?/);
        if (posM) {
          const [px, py] = posM[1].split(',').map(s => ctx.resolve(s.trim()));
          const isMM = posM[0].includes('mm2px(');
          pos = { x: isMM && isFinite(px) ? px * 15 / 5.08 : px, y: isMM && isFinite(py) ? py * 15 / 5.08 : py };
        }
        if (szM) {
          const [sw, sh] = szM[1].split(',').map(s => ctx.resolve(s.trim()));
          const isMM = szM[0].includes('mm2px(');
          size = { w: isMM && isFinite(sw) ? sw * 15 / 5.08 : sw, h: isMM && isFinite(sh) ? sh * 15 / 5.08 : sh };
        }
      }
      // Fallback: extract position from createWidget argument list
      if (!pos) {
        const createPosM = line.match(/createWidget<\w+Display>\s*\(\s*(?:mm2px\()?\s*Vec\(([^)]+)\)/);
        if (createPosM) {
          const [px, py] = createPosM[1].split(',').map(s => ctx.resolve(s.trim()));
          const isMM = createPosM[0].includes('mm2px(');
          pos = { x: isMM && isFinite(px) ? px * 15 / 5.08 : px, y: isMM && isFinite(py) ? py * 15 / 5.08 : py };
        }
      }
      components.push({ kind: 'display', pos, size });
      i++;
      continue;
    }

    // ---- Single-line array declarations ----
    // Pattern: float/int NAME[N] = { val1, val2, ... };
    // Must come before variable declarations since float colX[8] = {...} matches both patterns.
    const arrDeclMatch = line.match(/^\s*(?:float|int)\s+(\w+)\[\d*\]\s*=\s*\{(.+?)\}\s*;/);
    if (arrDeclMatch) {
      const values = arrDeclMatch[2].split(',').map(s => ctx.resolve(s.trim()));
      // Keep only numeric values; discard enum references we can't resolve
      ctx.setArray(arrDeclMatch[1], values.filter(v => isFinite(v)));
      i++;
      continue;
    }

    // ---- Variable declarations (with initialiser) ----
    // Pattern: float/int VAR = EXPR;
    const varMatch = line.match(/^\s*(?:float|int)\s+(\w+)\s*=\s*(.+?);\s*$/);
    if (varMatch) {
      const val = ctx.resolve(varMatch[2]);
      ctx.setVar(varMatch[1], val);
      i++;
      continue;
    }

    // ---- Static const array (single-line) ----
    // Pattern: static const int NAME[] = { Module::ENUM1, Module::ENUM2 };
    const staticArrMatch = line.match(/^\s*static\s+const\s+(?:int|float)\s+(\w+)\[\]\s*=\s*\{(.+?)\}\s*;/);
    if (staticArrMatch) {
      const values = staticArrMatch[2].split(',').map(s => {
        // Preserve enum-style strings (Module::ENUM_NAME) for label lookup
        const enumMatch = s.trim().match(/::(\w+)/);
        if (enumMatch) return enumMatch[1];
        return ctx.resolve(s.trim());
      });
      ctx.setArray(staticArrMatch[1], values);
      i++;
      continue;
    }

    // ---- Multi-line array declarations ----
    // Pattern: static const int NAME[] = {
    //            Module::ENUM1,
    //            Module::ENUM2,
    //          };
    const arrStart = line.match(/^\s*(?:static\s+)?const\s+(?:int|float)\s+(\w+)\[\]\s*=\s*\{\s*$/);
    if (arrStart) {
      const arrName = arrStart[1];
      const values = [];
      let j = i + 1;
      while (j < lines.length) {
        const sub = lines[j].trim();
        if (sub === '};' || sub === '}') break;
        const end = sub.match(/^\s*(.+?)\s*\};?\s*$/);
        if (end) {
          const last = end[1].trim();
          const enumMatch = last.match(/::(\w+)/);
          values.push(enumMatch ? enumMatch[1] : ctx.resolve(last));
          break;
        }
        const item = sub.replace(/,$/, '').trim();
        if (item) {
          const enumMatch = item.match(/::(\w+)/);
          values.push(enumMatch ? enumMatch[1] : ctx.resolve(item));
        }
        j++;
      }
      ctx.setArray(arrName, values);
      i = j + 1;
      continue;
    }

    // ---- Compound assignment: y += 28 / y -= 5 ----
    const assignMatch = line.match(/^\s*(\w+)\s*(\+=|-=)\s*(.+?);\s*$/);
    if (assignMatch) {
      const varName = assignMatch[1];
      const op = assignMatch[2];
      const val = ctx.resolve(assignMatch[3]);
      if (ctx.vars[varName] !== undefined) {
        ctx.setVar(varName, op === '+=' ? ctx.vars[varName] + val : ctx.vars[varName] - val);
      }
      i++;
      continue;
    }

    // ---- Simple assignment (previously-declared variable): y = expr; ----
    const simpleAssign = line.match(/^\s*(\w+)\s*=\s*(.+?);\s*$/);
    if (simpleAssign && ctx.vars[simpleAssign[1]] !== undefined) {
      ctx.setVar(simpleAssign[1], ctx.resolve(simpleAssign[2]));
      i++;
      continue;
    }

    // ---- Component creation calls (knobs, jacks, switches, lights, screws) ----
    const compMatch = parseComponentLine(line, ctx);
    if (compMatch) {
      components.push(compMatch);
      i++;
      continue;
    }

    i++;
  }

  // Clean up loop variables so they don't leak to sibling iterations
  for (const k of Object.keys(loopVars)) delete ctx.vars[k];
}

// Try to parse a component creation call from a single line.
// Returns { kind, type, x, y, role, label, ... } or null.
function parseComponentLine(line, ctx) {
  // Generic create*Centered calls:
  // addParam(createParamCentered<Type>(Vec(x, y), module, ENUM))
  // addInput(createInputCentered<Type>(Vec(x, y), module, ENUM))
  // addOutput(createOutputCentered<Type>(Vec(x, y), module, ENUM))
  // addChild(createLightCentered<Type>(Vec(x, y), module, ENUM))
  // addParam(createLightParamCentered<Type>(Vec(x, y), module, PARAM_ENUM, LIGHT_ENUM))
  // addChild(createWidget<Type>(Vec(x, y)))

  const patterns = [
    // createLightParamCentered<Type>(Vec(x, y), module, PARAM, LIGHT)
    {
      re: /createLightParamCentered<([^>]+(?:<[^>]+>)?)>\s*\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?,\s*module,\s*([^,]+),\s*([^)]+)\)/,
      role: 'param',
      isBezel: true,
    },
    // createParamCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /createParamCentered<([^>]+(?:<[^>]+>)?)>\s*\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?,\s*module,\s*([^)]+)\)/,
      role: 'param',
    },
    // createInputCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /createInputCentered<([^>]+(?:<[^>]+>)?)>\s*\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?,\s*module,\s*([^)]+)\)/,
      role: 'input',
    },
    // createOutputCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /createOutputCentered<([^>]+(?:<[^>]+>)?)>\s*\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?,\s*module,\s*([^)]+)\)/,
      role: 'output',
    },
    // createLightCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /createLightCentered<([^>]+(?:<[^>]+>)?)>\s*\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?,\s*module,\s*([^)]+)\)/,
      role: 'light',
    },
    // createWidget<Type>(Vec(x, y)) — screws
    {
      re: /createWidget<([^>]+)>\((?:mm2px\()?\s*Vec\(([^,]+),\s*([^)]+)\)\s*\)?\)/,
      role: 'widget',
    },
  ];

  for (const pat of patterns) {
    const m = line.match(pat.re);
    if (m) {
      const typeStr = m[1];
      const isMM = m[0].includes('mm2px(');
      let x = ctx.resolve(m[2]);
      let y = ctx.resolve(m[3]);
      if (isMM && isFinite(x) && isFinite(y)) {
        x = x * 15 / 5.08;
        y = y * 15 / 5.08;
      }
      const enumName = m[4] || '';
      const lightEnum = m[5] || '';

      // Resolve array accesses in enum (e.g. outIds[i] → OUT_1_2)
      const resolved = ctx.resolve(enumName);
      const resolvedEnum = (typeof resolved === 'string' && /^[A-Z]/.test(resolved)) ? resolved : '';

      // Strip module:: prefix and + i suffix from enum names for cleaner labels
      const labelEnum = resolvedEnum || enumName.replace(/.*::/, '').replace(/\s*\+.*$/, '');
      const lightLabel = lightEnum.replace(/.*::/, '').replace(/\s*\+.*$/, '');

      // Determine base info
      const wi = resolveBaseType(typeStr);

      // For VCVLightBezel used as param (createLightParamCentered), default to bezel
      // if no widget info found; prefer resolved kind otherwise
      const finalKind = wi ? wi.kind : (pat.isBezel ? 'bezel' : 'unknown');

      const component = {
        kind: finalKind,
        type: typeStr,
        x,
        y,
        role: pat.role,
        enum: resolvedEnum || enumName,
        label: labelEnum,
        lightEnum: lightLabel,
      };

      // Copy dimensional info from widget info
      if (wi) {
        if (wi.rad !== undefined) component.rad = wi.rad;
        if (wi.hw !== undefined) { component.hw = wi.hw; component.hh = wi.hh; }
      }

      return component;
    }
  }

  return null;
}

// ============================================================
// SVG Generation
//
// Produces a panel SVG from the parsed module data. The SVG
// uses millimetre units matching Rack's coordinate system
// (viewBox in mm, panel height 128.5mm).
//
// The output includes:
//   - Background gradient (simulated with vertical strips to
//     avoid SVG url(#id) issues inside Rack)
//   - Corner screws
//   - Module name label
//   - Display rectangles
//   - Jacks, knobs, switches, buttons, bezels, lights, sliders
//     each drawn with appropriate shapes and colors
//   - Labels from configParam/Input/Output names
// ============================================================
function generateSVG(info) {
  const { moduleName, HP, components, configNames } = info;
  const W = HP * 5.08; // panel width in mm
  const H = CFG.height; // panel height in mm
  const cx = W / 2;    // horizontal centre in mm
  const HW = HP * 15;  // panel width in rack-units (for ru2mm conversion)

  // Linear interpolation between two hex color strings
  // t=0 returns a, t=1 returns b, intermediate values blend
  const lerpColor = (a, b, t) => {
    const ah = parseInt(a.slice(1), 16), bh = parseInt(b.slice(1), 16);
    const ar = (ah >> 16) & 0xff, ag = (ah >> 8) & 0xff, ab = ah & 0xff;
    const br = (bh >> 16) & 0xff, bg = (bh >> 8) & 0xff, bb = bh & 0xff;
    const rr = Math.round(ar + (br - ar) * t);
    const rg = Math.round(ag + (bg - ag) * t);
    const rb = Math.round(ab + (bb - ab) * t);
    return '#' + ((1 << 24) | (rr << 16) | (rg << 8) | rb).toString(16).slice(1);
  };

  let svg = `<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" viewBox="0 0 ${W.toFixed(2)} ${H}" width="${W.toFixed(2)}mm" height="${H}mm">\n`;

  // ---- Layer 1: Background ----
  svg += `  <g inkscape:groupmode="layer" id="layer-background" inkscape:label="Background">\n`;
  svg += `    <rect x="0.3" y="0.3" width="${(W - 0.6).toFixed(2)}" height="${H - 0.6}" fill="none" stroke="#333" stroke-width="${CFG.strokeWidth}"/>\n`;

  // Background gradient — inline strips to avoid url(#id) issues in Rack
  if (CFG.bg.start === CFG.bg.end) {
    svg += `    <rect x="0" y="0" width="${W.toFixed(2)}" height="${H}" fill="${CFG.bg.start}"/>\n`;
  } else {
    const STRIPS = 100;
    const stripH = H / STRIPS;
    for (let i = 0; i < STRIPS; i++) {
      const yPos = i / STRIPS;  // 0..1 from top to bottom
      const mid = CFG.bg.mid / 100;
      const t = Math.min(yPos / mid, 1);
      const color = lerpColor(CFG.bg.start, CFG.bg.end, t);
      svg += `    <rect x="0" y="${(i * stripH).toFixed(3)}" width="${W.toFixed(2)}" height="${(stripH + 0.01).toFixed(3)}" fill="${color}"/>\n`;
    }
  }
  svg += `  </g>\n`;

  // ---- Layer 2: Component Outlines ----
  svg += `  <g inkscape:groupmode="layer" id="layer-outlines" inkscape:label="Component Outlines">\n`;

  // Screws — always at corners
  const sr = ru2mm(7.5);
  const screwX1 = sr;
  const screwX2 = W - sr;
  const screwY1 = sr;
  const screwY2 = H - sr;
  for (const sx of [screwX1, screwX2]) {
    for (const sy of [screwY1, screwY2]) {
      svg += `    <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="${sr.toFixed(2)}" fill="#2a2a2a" stroke="#444" stroke-width="${CFG.strokeWidth}"/>\n`;
      svg += `    <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="0.8" fill="#444"/>\n`;
    }
  }

  // Collect displays and widgets
  const displays = components.filter(c => c.kind === 'display');
  const widgets = components.filter(c => c.kind !== 'display');

  // Draw displays
  for (const d of displays) {
    if (d.pos && d.size) {
      const dx = ru2mm(d.pos.x);
      const dy = ru2mm(d.pos.y);
      const dw = ru2mm(d.size.w);
      const dh = ru2mm(d.size.h);
      svg += `    <rect x="${dx.toFixed(2)}" y="${dy.toFixed(2)}" width="${dw.toFixed(2)}" height="${dh.toFixed(2)}" rx="1" fill="#0a0f0a" stroke="#1a3a1a" stroke-width="${CFG.strokeWidth + 0.2}"/>\n`;
    }
  }

  // Draw widget shapes (no text)
  for (const w of widgets) {
    const mx = ru2mm(w.x);
    const my = ru2mm(w.y);

    // Determine color
    let color = '#888';
    if (w.role === 'input') color = CFG.colors.input;
    else if (w.role === 'output') color = CFG.colors.output;

    switch (w.kind) {
      case 'slider': {
        const sw = ru2mm(w.hw || 5);
        const sh = ru2mm(w.hh || 15);
        svg += `    <rect x="${(mx - sw).toFixed(2)}" y="${(my - sh).toFixed(2)}" width="${(sw * 2).toFixed(2)}" height="${(sh * 2).toFixed(2)}" rx="1.5" fill="#1a1a1a" stroke="${color}" stroke-width="${CFG.strokeWidth}" opacity="0.5"/>\n`;
        break;
      }
      case 'jack': {
        const r = ru2mm(w.rad);
        if (w.role === 'output') {
          const sw = CFG.strokeWidth + 0.1;
          const d = r * 2 + sw;
          svg += `    <rect x="${(mx - r - sw / 2).toFixed(2)}" y="${(my - r - sw / 2).toFixed(2)}" width="${d.toFixed(2)}" height="${d.toFixed(2)}" rx="1.5" fill="${color}" opacity="0.7"/>\n`;
          svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.65).toFixed(2)}" fill="#111" opacity="0.5"/>\n`;
        } else {
          svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#111" stroke="${color}" stroke-width="${CFG.strokeWidth + 0.1}" opacity="0.7"/>\n`;
          svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="${color}" opacity="0.5"/>\n`;
        }
        break;
      }
      case 'knob': {
        const r = ru2mm(w.rad);
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#222" stroke="${color}" stroke-width="${CFG.strokeWidth}" opacity="0.6"/>\n`;
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="#333" opacity="0.4"/>\n`;
        // Indicator line
        const indX = mx + r * 0.65;
        svg += `    <line x1="${mx.toFixed(2)}" y1="${my.toFixed(2)}" x2="${indX.toFixed(2)}" y2="${my.toFixed(2)}" stroke="#666" stroke-width="0.5" opacity="0.6"/>\n`;
        break;
      }
      case 'switch': {
        const hw = ru2mm(w.hw || 7);
        const hh = ru2mm(w.hh || 10.32);
        svg += `    <rect x="${(mx - hw).toFixed(2)}" y="${(my - hh).toFixed(2)}" width="${(hw * 2).toFixed(2)}" height="${(hh * 2).toFixed(2)}" rx="1" fill="none" stroke="${color}" stroke-width="${CFG.strokeWidth - 0.1}" opacity="0.5"/>\n`;
        svg += `    <line x1="${mx.toFixed(2)}" y1="${(my + hh * 0.5).toFixed(2)}" x2="${mx.toFixed(2)}" y2="${(my - hh * 0.5).toFixed(2)}" stroke="${color}" stroke-width="${CFG.strokeWidth + 0.2}" opacity="0.6"/>\n`;
        break;
      }
      case 'button': {
        const r = ru2mm(w.rad);
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#222" stroke="${color}" stroke-width="${CFG.strokeWidth}" opacity="0.6"/>\n`;
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.4).toFixed(2)}" fill="#444" opacity="0.4"/>\n`;
        break;
      }
      case 'bezel': {
        const r = ru2mm(w.rad);
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#1a1a1a" stroke="${color}" stroke-width="${CFG.strokeWidth + 0.1}" opacity="0.6"/>\n`;
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="#333" opacity="0.5"/>\n`;
        break;
      }
      case 'screw':
        // Screws already drawn above; skip duplicates
        break;
      default:
        // Unknown — draw small dot
        svg += `    <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="1" fill="#f0f"/>\n`;
        break;
    }
  }

  svg += `  </g>\n`;

  // Warn if any component or its label extends past the panel bottom (screws
  // are auto-placed at the corners, so they're excluded).
  for (const c of components) {
    if (c.kind === 'screw') continue;
    let extent = 0;
    if (c.kind === 'display') {
      if (c.pos && c.size) extent = ru2mm(c.pos.y + c.size.h);
    } else {
      const r = c.rad !== undefined ? c.rad : Math.max(c.hh || 0, c.hw || 0);
      extent = ru2mm(c.y + r) + 2.0; // +2mm label offset
    }
    if (extent > H)
      console.error(`${c.enum || c.kind} at y=${ru2mm(c.y || 0).toFixed(1)}mm extends past panel bottom (${H}mm)`);
  }

  // ---- Layer 3: Text ----
  svg += `  <g inkscape:groupmode="layer" id="layer-text" inkscape:label="Text">\n`;

  // Module name
  const displayName = moduleName.charAt(0).toUpperCase() + moduleName.slice(1);
  svg += `    ${textToPath(displayName, cx, ru2mm(10), FONT_SIZE_NAME, '#555')}\n`;

  // Draw widget labels
  for (const w of widgets) {
    const mx = ru2mm(w.x);
    const my = ru2mm(w.y);

    // Determine color
    let color = '#888';
    if (w.role === 'input') color = CFG.colors.input;
    else if (w.role === 'output') color = CFG.colors.output;

    // Determine label from config name or prettified enum
    let label = '';
    if (w.enum) {
      // Strip module prefix and any + i suffix
      let enumKey = w.enum.replace(/.*::/, '').replace(/\s*\+.*$/, '');
      if (configNames[enumKey]) {
        label = configNames[enumKey];
      } else {
        // Prettify enum name: remove trailing _INPUT/_OUTPUT/_PARAM, replace _ with space
        label = enumKey.replace(/_(INPUT|OUTPUT|PARAM)$/, '').replace(/_/g, ' ');
      }
    }

    if (!label) continue;

    let ly;
    switch (w.kind) {
      case 'slider': {
        const sh = ru2mm(w.hh || 15);
        ly = my + sh + 2.0;
        break;
      }
      case 'jack': {
        const r = ru2mm(w.rad);
        ly = my + r + 2.0;
        break;
      }
      case 'knob': {
        const r = ru2mm(w.rad);
        ly = my + r + 2.0;
        break;
      }
      case 'switch': {
        const hh = ru2mm(w.hh || 10.32);
        ly = my + hh + 2.0;
        break;
      }
      case 'button': {
        const r = ru2mm(w.rad);
        ly = my + r + 2.0;
        break;
      }
      case 'bezel': {
        const r = ru2mm(w.rad);
        ly = my + r + 2.0;
        break;
      }
      default:
        continue;
    }

    svg += `    ${textToPath(label, mx, ly, FONT_SIZE_LABEL, color, 0.7)}\n`;
  }

  svg += `  </g>\n`;
  svg += `</svg>\n`;
  return svg;
}

// ============================================================
// Main — CLI entry point
//
// Parses CLI flags (see header comment), then either:
//   --all   : batch-generate SVGs for all src/ra-*.cpp files
//   [file]  : generate a single SVG to res/
// ============================================================

let moduleName = 'ra-endless';          // default if no module given
let overrideHP = 0;                    // --hp=N override
let heightFlag = false;                // --height=NNN given
let allFlag = false;                   // --all mode

for (let i = 2; i < process.argv.length; i++) {
  const arg = process.argv[i];
  if (arg === '--all') {
    allFlag = true;
  } else if (arg.startsWith('--hp=')) {
    overrideHP = parseInt(arg.slice(5), 10);
  } else if (arg.startsWith('--height=')) {
    CFG.height = Math.max(60, parseFloat(arg.slice(9)));
    heightFlag = true;
  } else if (arg.startsWith('--input-color=')) {
    CFG.colors.input = arg.slice(14);
  } else if (arg.startsWith('--output-color=')) {
    CFG.colors.output = arg.slice(15);
  } else if (arg.startsWith('--bg-start=')) {
    CFG.bg.start = arg.slice(11);
  } else if (arg.startsWith('--bg-end=')) {
    CFG.bg.end = arg.slice(9);
  } else if (arg.startsWith('--bg-mid=')) {
    CFG.bg.mid = Math.max(0, Math.min(100, parseInt(arg.slice(9), 10)));
  } else if (arg.startsWith('--font=')) {
    FONT_PATH = arg.slice(7);
  } else if (!arg.startsWith('--')) {
    moduleName = arg;
  }
}

loadPanelFont(FONT_PATH);

// ---- Batch mode: scan src/ for ra-*.cpp, write each SVG to res/ ----
if (allFlag) {
  const files = fs.readdirSync('modules/src').filter(f => f.startsWith('ra-') && f.endsWith('.cpp'));
  const dir = path.resolve('modules/res');
  fs.mkdirSync(dir, { recursive: true });
  for (const f of files) {
    const fp = path.resolve('modules/src', f);
    const info = parseModule(fp);
    if (overrideHP) info.HP = overrideHP;
    if (!heightFlag) CFG.height = info.existingHeight || 128.5;
    const svg = generateSVG(info);
    const outName = f.replace(/\.cpp$/, '.svg');
    fs.writeFileSync(path.join(dir, outName), svg);
    console.error(`${outName} (${info.HP}hp)`);
  }
  process.exit(0);
}

// ---- Single-file mode: read one source, write SVG to res/ ----
const srcPath = path.resolve('modules/src', moduleName + '.cpp');

if (!fs.existsSync(srcPath)) {
  console.error('File not found:', srcPath);
  process.exit(1);
}

const info = parseModule(srcPath);
if (overrideHP) info.HP = overrideHP;
if (!heightFlag) CFG.height = info.existingHeight || 128.5;
const svg = generateSVG(info);

const outName = moduleName + '.svg';
const outDir = path.resolve('modules/res');
fs.mkdirSync(outDir, { recursive: true });
const outPath = path.join(outDir, outName);
fs.writeFileSync(outPath, svg);
console.error(`${outName} (${info.HP}hp)`);
