// Generate ra-endless panel SVG from widget positions in the C++ source.
// Usage: node util/gen-panel.mjs > res/ra-endless.svg

const ru2mm = (ru) => ru * 5.08 / 15;
const HP = 10;
const W = HP * 5.08;
const H = 128.5;
const cx = W / 2;

// Component radii in rack units → mm (at SVG resolution)
const RADII = {
  in:      { r: 11.85 },        // PJ301MPort
  out:     { r: 11.85 },        // PJ301MPort
  switch:  { w: 7,   h: 10.32 }, // CKSS half-width/height
  button:  { r: 9 },            // VCVButton
  bezel:   { r: 10.65 },        // VCVLightBezel
  screw:   { r: 7.5 },          // ScrewSilver
};

const elements = [
  { x: 24,  y: 140, label: 'CV',    kind: 'in' },
  { x: 52,  y: 140, label: 'THRU',  kind: 'switch' },
  { x: 80,  y: 140, label: 'RUN',   kind: 'in' },
  { x: 106, y: 140, label: 'RUN',   kind: 'bezel' },
  { x: 130, y: 140, label: 'A/B',   kind: 'button' },

  { x: 23,  y: 177, label: 'WRT',   kind: 'button' },
  { x: 57,  y: 177, label: 'WRT',   kind: 'in' },
  { x: 91,  y: 177, label: 'REST',  kind: 'button' },
  { x: 125, y: 177, label: 'REST',  kind: 'in' },

  { x: 23,  y: 217, label: 'BACK',  kind: 'bezel' },
  { x: 57,  y: 217, label: 'BACK',  kind: 'in' },
  { x: 91,  y: 217, label: 'FWD',   kind: 'bezel' },
  { x: 125, y: 217, label: 'FWD',   kind: 'in' },

  { x: 23,  y: 257, label: 'CLR',   kind: 'button' },
  { x: 57,  y: 257, label: 'CLR',   kind: 'in' },
  { x: 91,  y: 257, label: 'RST',   kind: 'button' },
  { x: 125, y: 257, label: 'RST',   kind: 'in' },

  { x: 75,  y: 277, label: 'POS',   kind: 'in' },

  { x: 30,  y: 305, label: 'A CV',  kind: 'out' },
  { x: 75,  y: 305, label: 'A TRIG',kind: 'out' },
  { x: 120, y: 305, label: 'A END', kind: 'out' },
  { x: 30,  y: 345, label: 'B CV',  kind: 'out' },
  { x: 75,  y: 345, label: 'B TRIG',kind: 'out' },
  { x: 120, y: 345, label: 'B END', kind: 'out' },
];

const disp = { x: 12, y: 21, w: 126, h: 96 };

const color = (kind) => {
  switch (kind) {
    case 'in':  return '#3af';
    case 'out': return '#fa3';
    case 'switch': return '#888';
    case 'button': return '#888';
    case 'bezel': return '#888';
    default: return '#888';
  }
};

let svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W.toFixed(2)} ${H}" width="${W.toFixed(2)}mm" height="${H}mm">
  <rect width="${W.toFixed(2)}" height="${H}" fill="#1a1a1a"/>
  <rect x="0.3" y="0.3" width="${(W - 0.6).toFixed(2)}" height="${H - 0.6}" fill="none" stroke="#333" stroke-width="0.3"/>
`;

// Screws
const sr = ru2mm(RADII.screw.r);
const screwX1 = sr;
const screwX2 = W - sr;
const screwY1 = sr;
const screwY2 = H - sr;
for (const sx of [screwX1, screwX2]) {
  for (const sy of [screwY1, screwY2]) {
    svg += `  <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="${sr.toFixed(2)}" fill="#2a2a2a" stroke="#444" stroke-width="0.3"/>\n`;
    svg += `  <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="0.8" fill="#444"/>\n`;
  }
}

// Display
const dm = { x: ru2mm(disp.x), y: ru2mm(disp.y), w: ru2mm(disp.w), h: ru2mm(disp.h) };
svg += `  <rect x="${dm.x.toFixed(2)}" y="${dm.y.toFixed(2)}" width="${dm.w.toFixed(2)}" height="${dm.h.toFixed(2)}" rx="1" fill="#0a0f0a" stroke="#1a3a1a" stroke-width="0.4"/>\n`;

// Module name
svg += `  <text x="${cx.toFixed(2)}" y="${(ru2mm(10)).toFixed(2)}" fill="#555" font-family="sans-serif" font-size="2.2" text-anchor="middle" font-weight="bold">ENDLESS</text>\n`;

// Draw each component
for (const el of elements) {
  const mx = ru2mm(el.x);
  const my = ru2mm(el.y);
  const c = color(el.kind);
  const dim = RADII[el.kind];

  if (el.kind === 'in' || el.kind === 'out') {
    // Jack: outer ring + inner dark circle + highlight
    const r = ru2mm(dim.r);
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#111" stroke="${c}" stroke-width="0.35" opacity="0.7"/>\n`;
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="${c}" opacity="0.5"/>\n`;
    // Label below
    const ly = my + r + 1.2;
    svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${c}" font-family="sans-serif" font-size="1.6" text-anchor="middle" opacity="0.7">${el.label}</text>\n`;

  } else if (el.kind === 'switch') {
    // CKSS toggle: small rectangle + lever line
    const hw = ru2mm(dim.w);
    const hh = ru2mm(dim.h);
    svg += `  <rect x="${(mx - hw).toFixed(2)}" y="${(my - hh).toFixed(2)}" width="${(hw * 2).toFixed(2)}" height="${(hh * 2).toFixed(2)}" rx="1" fill="none" stroke="${c}" stroke-width="0.25" opacity="0.5"/>\n`;
    // Lever
    svg += `  <line x1="${mx.toFixed(2)}" y1="${(my + hh * 0.5).toFixed(2)}" x2="${mx.toFixed(2)}" y2="${(my - hh * 0.5).toFixed(2)}" stroke="${c}" stroke-width="0.4" opacity="0.6"/>\n`;
    // Label below
    const ly = my + hh + 1.2;
    svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${c}" font-family="sans-serif" font-size="1.6" text-anchor="middle" opacity="0.7">${el.label}</text>\n`;

  } else if (el.kind === 'button') {
    // VCVButton: filled circle
    const r = ru2mm(dim.r);
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#222" stroke="${c}" stroke-width="0.3" opacity="0.6"/>\n`;
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.4).toFixed(2)}" fill="#444" opacity="0.4"/>\n`;
    const ly = my + r + 1.2;
    svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${c}" font-family="sans-serif" font-size="1.6" text-anchor="middle" opacity="0.7">${el.label}</text>\n`;

  } else if (el.kind === 'bezel') {
    // VCVLightBezel: larger ring with inner LED
    const r = ru2mm(dim.r);
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r.toFixed(2)}" fill="#1a1a1a" stroke="${c}" stroke-width="0.35" opacity="0.6"/>\n`;
    svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${(r * 0.35).toFixed(2)}" fill="#333" opacity="0.5"/>\n`;
    const ly = my + r + 1.2;
    svg += `  <text x="${mx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${c}" font-family="sans-serif" font-size="1.6" text-anchor="middle" opacity="0.7">${el.label}</text>\n`;
  }
}

// Divider lines
const lineY = [ru2mm(118), ru2mm(160), ru2mm(200), ru2mm(240)];
for (const ly of lineY) {
  svg += `  <line x1="2" y1="${ly.toFixed(2)}" x2="${(W - 2).toFixed(2)}" y2="${ly.toFixed(2)}" stroke="#2a2a2a" stroke-width="0.2"/>\n`;
}

// Section labels
const sections = [
  { y: 127, text: 'CTRL' },
  { y: 167, text: 'REC' },
  { y: 207, text: 'STEP' },
  { y: 247, text: 'MISC' },
];
for (const sec of sections) {
  const sy = ru2mm(sec.y);
  svg += `  <text x="2.5" y="${sy.toFixed(2)}" fill="#444" font-family="sans-serif" font-size="1.4" font-weight="bold">${sec.text}</text>\n`;
}

// Track labels for outputs
svg += `  <text x="${ru2mm(15).toFixed(2)}" y="${ru2mm(295).toFixed(2)}" fill="#555" font-family="sans-serif" font-size="1.4" text-anchor="middle">A</text>\n`;
svg += `  <text x="${ru2mm(15).toFixed(2)}" y="${ru2mm(335).toFixed(2)}" fill="#555" font-family="sans-serif" font-size="1.4" text-anchor="middle">B</text>\n`;

svg += `</svg>\n`;

process.stdout.write(svg);
