#!/usr/bin/env node
// Generate a panel SVG from a VCV Rack module C++ source file.
// Usage: node util/gen-panel.mjs [src/ra-xxxx.cpp] > out.svg

import fs from 'node:fs';
import path from 'node:path';

// ============================================================
// Configuration
// ============================================================
const SW = 1.0;  // base stroke width

let CFG = {
  bg: {
    start: '#443852',
    end: '#242425',
    mid: 33,
  },
  colors: {
    input: '#62a0ea',
    output: '#CF5DD0',
  },
};

const ru2mm = (ru) => ru * 5.08 / 15;

// Widget physical dimensions in rack units
// rad = radius; hw = half-width; hh = half-height
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

// ============================================================
// Expression resolver
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
// ============================================================
function parseModule(filePath) {
  const src = fs.readFileSync(filePath, 'utf8');
  const lines = src.split('\n');

  // ---- Extract panel SVG path ----
  const panelMatch = src.match(/asset::plugin\(pluginInstance,\s*"([^"]+\.svg)"\)/);
  const panelSvg = panelMatch ? panelMatch[1] : null;

  // ---- Extract module name from createModel ----
  const modelMatch = src.match(/createModel<\w+,\s*\w+>\s*\(\s*"([^"]+)"\s*\)/);
  const moduleName = modelMatch ? modelMatch[1] : path.basename(filePath, '.cpp');

  // ---- Extract config names (display labels for params/inputs/outputs) ----
  const configNames = {};
  // configParam(ENUM, ..., "Name", ...)
  const configParamRe = /configParam\(\s*(\w+)\s*,.*?"([^"]+?)"/g;
  let m;
  while ((m = configParamRe.exec(src)) !== null) configNames[m[1]] = m[2];
  // configInput(ENUM, "Name")
  const configInputRe = /configInput\(\s*(\w+)\s*,\s*"([^"]+?)"\s*\)/g;
  while ((m = configInputRe.exec(src)) !== null) configNames[m[1]] = m[2];
  // configOutput(ENUM, "Name")
  const configOutputRe = /configOutput\(\s*(\w+)\s*,\s*"([^"]+?)"\s*\)/g;
  while ((m = configOutputRe.exec(src)) !== null) configNames[m[1]] = m[2];

  // ---- Determine HP from panel SVG ----
  let HP = 0;
  if (panelSvg) {
    const svgDir = path.dirname(path.resolve(filePath));
    const svgFull = path.resolve(svgDir, '..', panelSvg);
    if (fs.existsSync(svgFull)) {
      const svgContent = fs.readFileSync(svgFull, 'utf8');
      const vbParts = svgContent.match(/viewBox="\s*([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)/);
      if (vbParts) {
        const mmWidth = parseFloat(vbParts[3]);
        HP = Math.round(mmWidth / 5.08);
      }
    }
  }

  // Fallback: estimate from the rightmost numeric x-coordinate in Vec() calls.
  // Only kicks in when SVG is unavailable or its HP is implausibly large
  // for the component layout (e.g. pre-generated output read on a 2nd run).
  if (!HP) {
    let maxX = 0;
    const xRe = /Vec\(\s*([\d.]+)\s*,/g;
    let xm;
    while ((xm = xRe.exec(src)) !== null) {
      const xv = parseFloat(xm[1]);
      if (xv > maxX) maxX = xv;
    }
    // Components at box.size.x - N imply a position near the right edge
    const offsetRe = /box\.size\.x\s*-\s*([\d.]+)/g;
    let om;
    while ((om = offsetRe.exec(src)) !== null) {
      const w = parseFloat(om[1]) + 10;
      if (w > maxX) maxX = w;
    }
    if (maxX > 0) {
      // The rightmost component center + 15 units (~1HP margin) ≈ panel width
      const guessedHP = Math.ceil((maxX + 15) / 15);
      if (guessedHP >= 4 && guessedHP <= 12) HP = guessedHP;
    }
  }
  if (!HP) HP = 4;
  const HW = HP * 15; // box.size.x in rack units

  // ---- Parse widget constructor body ----
  // Find the Widget constructor
  const widgetStartRe = /\b(\w+Widget)\s*\([^)]*\)\s*\{/;
  const consMatch = src.match(widgetStartRe);
  if (!consMatch) {
    console.error('Could not find Widget constructor in', filePath);
    process.exit(1);
  }
  const constructorStart = src.indexOf(consMatch[0]);
  const body = extractBraces(src, constructorStart + consMatch[0].length - 1);
  const bodyLines = body.split('\n');

  // ---- Process body with expression context ----
  const ctx = new ExprContext(HW);
  const components = [];

  // Known arrays from enum IDs (we match these when we see patterns like RaChanceModule::OUT_1_2 + i)
  // We'll collect actual numeric enum values from enums.

  // Extract array declarations and variable declarations, and process component creation
  processBlock(bodyLines, ctx, components);

  return {
    moduleName,
    panelSvg,
    HP,
    HW,
    components,
    configNames,
  };
}

// Extract text between matching { } starting at openBraceIdx
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

// Process a block of lines, handling for loops, variable decls, component calls
function processBlock(lines, ctx, components, loopVars = {}) {
  // Merge loop variables into context temporarily
  for (const [k, v] of Object.entries(loopVars)) ctx.setVar(k, v);

  let i = 0;
  while (i < lines.length) {
    const line = lines[i].trim();

    // Skip empty lines and comments
    if (!line || line.startsWith('//')) { i++; continue; }

    // Detect for loop: for (int VAR = START; VAR < END; VAR++)
    const forMatch = line.match(/^\s*for\s*\(\s*(?:int|float)\s+(\w+)\s*=\s*([^;]+);\s*\1\s*<\s*([^;]+);\s*(?:\1\+\+|--\1|\1\s*=\s*\1\s*\+[^)]*)\s*\)\s*\{?\s*$/);
    if (forMatch) {
      const varName = forMatch[1];
      const startVal = ctx.resolve(forMatch[2]);
      const endExpr = forMatch[3];

      // Collect loop body — find matching closing brace
      let bodyLines = [];
      let braceCount = 0;
      let j = i;
      // If the for line already has {, start body from next line
      if (line.includes('{')) {
        braceCount = 1;
        j = i + 1;
      } else {
        j = i + 1;
        // Find opening brace
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

      // Unroll loop
      const endVal = ctx.resolve(endExpr);
      if (isFinite(startVal) && isFinite(endVal)) {
        for (let iter = startVal; iter < endVal; iter++) {
          processBlock(bodyLines, ctx, components, { [varName]: iter });
        }
      }

      i = j;
      continue;
    }

    // Detect display creation: auto *display = new SomeDisplay();
    const dispMatch = line.match(/^\s*(?:auto\s*)?\*?\s*\w+\s*=\s*new\s+(\w+Display)\(/);
    if (dispMatch) {
      // Look at next lines for ->box.pos = Vec(x, y) and ->box.size = Vec(w, h)
      let pos = null, size = null;
      for (let k = i; k < Math.min(i + 5, lines.length); k++) {
        const sub = lines[k].trim();
        const posM = sub.match(/->box\.pos\s*=\s*Vec\(([^)]+)\)/);
        const szM = sub.match(/->box\.size\s*=\s*Vec\(([^)]+)\)/);
        if (posM) {
          const [px, py] = posM[1].split(',').map(s => ctx.resolve(s.trim()));
          pos = { x: px, y: py };
        }
        if (szM) {
          const [sw, sh] = szM[1].split(',').map(s => ctx.resolve(s.trim()));
          size = { w: sw, h: sh };
        }
      }
      components.push({ kind: 'display', pos, size });
      i++;
      continue;
    }

    // Detect display via setPosition/setSize style (alternative pattern)
    // (already handled above)

    // Detect variable / array declarations
    // float/int VAR = EXPR;
    const varMatch = line.match(/^\s*(?:float|int)\s+(\w+)\s*=\s*(.+?);\s*$/);
    if (varMatch) {
      const val = ctx.resolve(varMatch[2]);
      ctx.setVar(varMatch[1], val);
      i++;
      continue;
    }

    // float TYPE[] = { ... };
    const arrMatch = line.match(/^\s*(?:float|int)\s+(\w+)\[\]\s*=\s*\{(.+?)\}\s*;/);
    if (arrMatch) {
      const values = arrMatch[2].split(',').map(s => ctx.resolve(s.trim()));
      // Filter out non-numeric values (like enum references)
      ctx.setArray(arrMatch[1], values.filter(v => isFinite(v)));
      i++;
      continue;
    }

    // static const int TYPE[] = { ... };
    const staticArrMatch = line.match(/^\s*static\s+const\s+(?:int|float)\s+(\w+)\[\]\s*=\s*\{(.+?)\}\s*;/);
    if (staticArrMatch) {
      const values = staticArrMatch[2].split(',').map(s => {
        // Handle RaChanceModule::OUT_1_2 style enum references
        const enumMatch = s.trim().match(/::(\w+)/);
        if (enumMatch) return enumMatch[1]; // store as enum name for later
        return ctx.resolve(s.trim());
      });
      ctx.setArray(staticArrMatch[1], values);
      i++;
      continue;
    }

    // Multi-line array: detect start of `static const int NAME[] = {`
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

    // Assignment: y += 28, y -= 5
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

    // Simple assignment (no type keyword): y = expr;
    const simpleAssign = line.match(/^\s*(\w+)\s*=\s*(.+?);\s*$/);
    if (simpleAssign && ctx.vars[simpleAssign[1]] !== undefined) {
      ctx.setVar(simpleAssign[1], ctx.resolve(simpleAssign[2]));
      i++;
      continue;
    }

    // ---- Component creation calls ----
    const compMatch = parseComponentLine(line, ctx);
    if (compMatch) {
      components.push(compMatch);
      i++;
      continue;
    }

    i++;
  }

  // Clean up loop vars
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
      re: /addParam\(createLightParamCentered<([^>]+(?:<[^>]+>)?)>\s*\(Vec\(([^,]+),\s*([^)]+)\),\s*module,\s*([^,]+),\s*([^)]+)\)\)/,
      role: 'param',
      isBezel: true,
    },
    // createParamCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /addParam\(createParamCentered<([^>]+(?:<[^>]+>)?)>\s*\(Vec\(([^,]+),\s*([^)]+)\),\s*module,\s*([^)]+)\)\)/,
      role: 'param',
    },
    // createInputCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /addInput\(createInputCentered<([^>]+(?:<[^>]+>)?)>\s*\(Vec\(([^,]+),\s*([^)]+)\),\s*module,\s*([^)]+)\)\)/,
      role: 'input',
    },
    // createOutputCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /addOutput\(createOutputCentered<([^>]+(?:<[^>]+>)?)>\s*\(Vec\(([^,]+),\s*([^)]+)\),\s*module,\s*([^)]+)\)\)/,
      role: 'output',
    },
    // createLightCentered<Type>(Vec(x, y), module, ENUM)
    {
      re: /addChild\(createLightCentered<([^>]+(?:<[^>]+>)?)>\s*\(Vec\(([^,]+),\s*([^)]+)\),\s*module,\s*([^)]+)\)\)/,
      role: 'light',
    },
    // createWidget<Type>(Vec(x, y)) — screws
    {
      re: /addChild\(createWidget<([^>]+)>\(Vec\(([^,]+),\s*([^)]+)\)\)\)/,
      role: 'widget',
    },
  ];

  for (const pat of patterns) {
    const m = line.match(pat.re);
    if (m) {
      const typeStr = m[1];
      const x = ctx.resolve(m[2]);
      const y = ctx.resolve(m[3]);
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

      // For VCVLightBezel used as param (createLightParamCentered), it's a bezel
      const finalKind = pat.isBezel ? 'bezel' : (wi ? wi.kind : 'unknown');

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
// ============================================================
function generateSVG(info) {
  const { moduleName, HP, components, configNames } = info;
  const W = HP * 5.08; // mm
  const H = 128.5;
  const cx = W / 2;
  const HW = HP * 15;

  // Interpolate between two hex colors: t=0 → a, t=1 → b
  const lerpColor = (a, b, t) => {
    const ah = parseInt(a.slice(1), 16), bh = parseInt(b.slice(1), 16);
    const ar = (ah >> 16) & 0xff, ag = (ah >> 8) & 0xff, ab = ah & 0xff;
    const br = (bh >> 16) & 0xff, bg = (bh >> 8) & 0xff, bb = bh & 0xff;
    const rr = Math.round(ar + (br - ar) * t);
    const rg = Math.round(ag + (bg - ag) * t);
    const rb = Math.round(ab + (bb - ab) * t);
    return '#' + ((1 << 24) | (rr << 16) | (rg << 8) | rb).toString(16).slice(1);
  };

  let svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W.toFixed(2)} ${H}" width="${W.toFixed(2)}mm" height="${H}mm">
  <rect x="0.3" y="0.3" width="${(W - 0.6).toFixed(2)}" height="${H - 0.6}" fill="none" stroke="#333" stroke-width="${SW}"/>
`;

  // Background gradient — inline strips to avoid url(#id) issues in Rack
  const STRIPS = 100;
  const stripH = H / STRIPS;
  for (let i = 0; i < STRIPS; i++) {
    const yPos = i / STRIPS;  // 0..1 from top to bottom
    const mid = CFG.bg.mid / 100;
    const t = Math.min(yPos / mid, 1);
    const color = lerpColor(CFG.bg.start, CFG.bg.end, t);
    svg += `  <rect x="0" y="${(i * stripH).toFixed(3)}" width="${W.toFixed(2)}" height="${(stripH + 0.01).toFixed(3)}" fill="${color}"/>\n`;
  }

  // Screws — always at corners
  const sr = ru2mm(7.5);
  const screwX1 = sr;
  const screwX2 = W - sr;
  const screwY1 = sr;
  const screwY2 = H - sr;
  for (const sx of [screwX1, screwX2]) {
    for (const sy of [screwY1, screwY2]) {
      svg += `  <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="${sr.toFixed(2)}" fill="#2a2a2a" stroke="#444" stroke-width="${SW}"/>\n`;
      svg += `  <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="0.8" fill="#444"/>\n`;
    }
  }

  // Module name
  const displayName = moduleName.charAt(0).toUpperCase() + moduleName.slice(1);
  svg += `  <text x="${cx.toFixed(2)}" y="${ru2mm(10).toFixed(2)}" fill="#555" font-family="sans-serif" font-size="2.8" text-anchor="middle" font-weight="bold">${displayName}</text>\n`;

  // Collect displays and components
  const displays = components.filter(c => c.kind === 'display');
  const widgets = components.filter(c => c.kind !== 'display');

  // Draw displays
  for (const d of displays) {
    if (d.pos && d.size) {
      const dx = ru2mm(d.pos.x);
      const dy = ru2mm(d.pos.y);
      const dw = ru2mm(d.size.w);
      const dh = ru2mm(d.size.h);
      svg += `  <rect x="${dx.toFixed(2)}" y="${dy.toFixed(2)}" width="${dw.toFixed(2)}" height="${dh.toFixed(2)}" rx="1" fill="#0a0f0a" stroke="#1a3a1a" stroke-width="${SW + 0.2}"/>\n`;
    }
  }

  // Draw widgets
  for (const w of widgets) {
    const mx = ru2mm(w.x);
    const my = ru2mm(w.y);

    // Determine colour
    let color = '#888';
    if (w.role === 'input') color = CFG.colors.input;
    else if (w.role === 'output') color = CFG.colors.output;
    else if (w.kind === 'screw') color = '#888';

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

    switch (w.kind) {
      case 'jack': {
        const r = ru2mm(w.rad);
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#111" stroke="${color}" stroke-width="${SW + 0.1}" opacity="0.7"/>\n`;
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="${color}" opacity="0.5"/>\n`;
        if (label) {
          const ly = my + r + 2.0;
          svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="2.0" text-anchor="middle" opacity="0.7">${label}</text>\n`;
        }
        break;
      }
      case 'knob': {
        const r = ru2mm(w.rad);
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#222" stroke="${color}" stroke-width="${SW}" opacity="0.6"/>\n`;
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="#333" opacity="0.4"/>\n`;
        // Indicator line
        const indX = mx + r * 0.65;
        svg += `  <line x1="${mx.toFixed(2)}" y1="${my.toFixed(2)}" x2="${indX.toFixed(2)}" y2="${my.toFixed(2)}" stroke="#666" stroke-width="0.5" opacity="0.6"/>\n`;
        if (label) {
          const ly = my + r + 2.0;
          svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="2.0" text-anchor="middle" opacity="0.7">${label}</text>\n`;
        }
        break;
      }
      case 'switch': {
        const hw = ru2mm(w.hw || 7);
        const hh = ru2mm(w.hh || 10.32);
        svg += `  <rect x="${(mx - hw).toFixed(2)}" y="${(my - hh).toFixed(2)}" width="${(hw * 2).toFixed(2)}" height="${(hh * 2).toFixed(2)}" rx="1" fill="none" stroke="${color}" stroke-width="${SW - 0.1}" opacity="0.5"/>\n`;
        svg += `  <line x1="${mx.toFixed(2)}" y1="${(my + hh * 0.5).toFixed(2)}" x2="${mx.toFixed(2)}" y2="${(my - hh * 0.5).toFixed(2)}" stroke="${color}" stroke-width="${SW + 0.2}" opacity="0.6"/>\n`;
        if (label) {
          const ly = my + hh + 2.0;
          svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="2.0" text-anchor="middle" opacity="0.7">${label}</text>\n`;
        }
        break;
      }
      case 'button': {
        const r = ru2mm(w.rad);
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#222" stroke="${color}" stroke-width="${SW}" opacity="0.6"/>\n`;
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.4).toFixed(2)}" fill="#444" opacity="0.4"/>\n`;
        if (label) {
          const ly = my + r + 2.0;
          svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="2.0" text-anchor="middle" opacity="0.7">${label}</text>\n`;
        }
        break;
      }
      case 'bezel': {
        const r = ru2mm(w.rad);
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#1a1a1a" stroke="${color}" stroke-width="${SW + 0.1}" opacity="0.6"/>\n`;
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="#333" opacity="0.5"/>\n`;
        if (label) {
          const ly = my + r + 2.0;
          svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${color}" font-family="sans-serif" font-size="2.0" text-anchor="middle" opacity="0.7">${label}</text>\n`;
        }
        break;
      }
      case 'screw':
        // Screws already drawn above; skip duplicates
        break;
      default:
        // Unknown — draw small dot
        svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="1" fill="#f0f"/>\n`;
        break;
    }
  }

  svg += `</svg>\n`;
  return svg;
}

// ============================================================
// Main
// ============================================================
let filePath = 'src/ra-endless.cpp';
let overrideHP = 0;
let allFlag = false;
for (let i = 2; i < process.argv.length; i++) {
  const arg = process.argv[i];
  if (arg === '--all') allFlag = true;
  else if (arg.startsWith('--hp=')) overrideHP = parseInt(arg.slice(5), 10);
  else if (arg.startsWith('--input-color=')) CFG.colors.input = arg.slice(14);
  else if (arg.startsWith('--output-color=')) CFG.colors.output = arg.slice(15);
  else if (arg.startsWith('--bg-start=')) CFG.bg.start = arg.slice(11);
  else if (arg.startsWith('--bg-end=')) CFG.bg.end = arg.slice(9);
  else if (arg.startsWith('--bg-mid=')) CFG.bg.mid = Math.max(0, Math.min(100, parseInt(arg.slice(9), 10)));
  else if (!arg.startsWith('--')) filePath = arg;
}

if (allFlag) {
  const files = fs.readdirSync('src').filter(f => f.startsWith('ra-') && f.endsWith('.cpp'));
  const dir = path.resolve('res');
  fs.mkdirSync(dir, { recursive: true });
  for (const f of files) {
    const fp = path.resolve('src', f);
    const info = parseModule(fp);
    if (overrideHP) info.HP = overrideHP;
    const svg = generateSVG(info);
    const outName = f.replace(/\.cpp$/, '.svg');
    fs.writeFileSync(path.join(dir, outName), svg);
    console.error(`${outName} (${info.HP}hp)`);
  }
  process.exit(0);
}

const fullPath = path.resolve(filePath);

if (!fs.existsSync(fullPath)) {
  console.error('File not found:', fullPath);
  process.exit(1);
}

const info = parseModule(fullPath);
if (overrideHP) info.HP = overrideHP;
const svg = generateSVG(info);
process.stdout.write(svg);
