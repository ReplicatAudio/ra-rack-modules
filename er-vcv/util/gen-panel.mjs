// Generate ra-endless panel SVG from widget positions in the C++ source.
// Usage: node util/gen-panel.mjs > res/ra-endless.svg

const ru2mm = (ru) => ru * 5.08 / 15;
const HP = 10;
const W = HP * 5.08;
const H = 128.5;
const cx = W / 2;

const elements = [
  // Each entry: [rackX, rackY, label, kind]
  // kind: 'in', 'out', 'param', 'switch', 'button', 'bezel'
  { x: 24,  y: 140, label: 'CV',       kind: 'in' },
  { x: 52,  y: 140, label: 'THRU',     kind: 'switch' },
  { x: 80,  y: 140, label: 'RUN',      kind: 'in' },
  { x: 106, y: 140, label: 'RUN',      kind: 'bezel' },
  { x: 130, y: 140, label: 'A/B',      kind: 'button' },

  { x: 23,  y: 177, label: 'WRT',      kind: 'button' },
  { x: 57,  y: 177, label: 'WRT',      kind: 'in' },
  { x: 91,  y: 177, label: 'REST',     kind: 'button' },
  { x: 125, y: 177, label: 'REST',     kind: 'in' },

  { x: 23,  y: 217, label: 'BACK',     kind: 'bezel' },
  { x: 57,  y: 217, label: 'BACK',     kind: 'in' },
  { x: 91,  y: 217, label: 'FWD',      kind: 'bezel' },
  { x: 125, y: 217, label: 'FWD',      kind: 'in' },

  { x: 23,  y: 257, label: 'CLR',      kind: 'button' },
  { x: 57,  y: 257, label: 'CLR',      kind: 'in' },
  { x: 91,  y: 257, label: 'RST',      kind: 'button' },
  { x: 125, y: 257, label: 'RST',      kind: 'in' },

  { x: 75,  y: 277, label: 'POS',      kind: 'in' },

  { x: 30,  y: 305, label: 'A CV',     kind: 'out' },
  { x: 75,  y: 305, label: 'A TRIG',   kind: 'out' },
  { x: 120, y: 305, label: 'A END',    kind: 'out' },
  { x: 30,  y: 345, label: 'B CV',     kind: 'out' },
  { x: 75,  y: 345, label: 'B TRIG',   kind: 'out' },
  { x: 120, y: 345, label: 'B END',    kind: 'out' },
];

const disp = { x: 12, y: 21, w: 126, h: 96 };

const color = (kind) => {
  switch (kind) {
    case 'in':  return '#3af';
    case 'out': return '#fa3';
    case 'param':
    case 'switch':
    case 'button':
    case 'bezel': return '#888';
    default: return '#888';
  }
};

const symbol = (kind) => {
  switch (kind) {
    case 'in':  return '▼';
    case 'out': return '▲';
    case 'switch': return '⬊';
    case 'button': return '●';
    case 'bezel': return '○';
    default: return '●';
  }
};

let svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W.toFixed(2)} ${H}" width="${W.toFixed(2)}mm" height="${H}mm">
  <defs>
    <filter id="glow"><feGaussianBlur stdDeviation="0.3" result="blur"/><feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
  </defs>
  <rect width="${W.toFixed(2)}" height="${H}" fill="#111"/>
  <rect x="0.3" y="0.3" width="${(W - 0.6).toFixed(2)}" height="${H - 0.6}" fill="none" stroke="#333" stroke-width="0.3"/>
`;

// Screws
const screwR = 7.5 * 5.08 / 15;
const screwX1 = 0 + screwR;
const screwX2 = W - screwR;
const screwY1 = 0 + screwR;
const screwY2 = H - screwR;

for (const sx of [screwX1, screwX2]) {
  for (const sy of [screwY1, screwY2]) {
    svg += `  <circle cx="${sx.toFixed(2)}" cy="${sy.toFixed(2)}" r="${screwR.toFixed(2)}" fill="#333" stroke="#555" stroke-width="0.2"/>\n`;
    svg += `  <line x1="${(sx - 1.5).toFixed(2)}" y1="${sy.toFixed(2)}" x2="${(sx + 1.5).toFixed(2)}" y2="${sy.toFixed(2)}" stroke="#555" stroke-width="0.3"/>\n`;
    svg += `  <line x1="${sx.toFixed(2)}" y1="${(sy - 1.5).toFixed(2)}" x2="${sx.toFixed(2)}" y2="${(sy + 1.5).toFixed(2)}" stroke="#555" stroke-width="0.3"/>\n`;
  }
}

// Display
const dm = { x: ru2mm(disp.x), y: ru2mm(disp.y), w: ru2mm(disp.w), h: ru2mm(disp.h) };
svg += `  <rect x="${dm.x.toFixed(2)}" y="${dm.y.toFixed(2)}" width="${dm.w.toFixed(2)}" height="${dm.h.toFixed(2)}" rx="1" fill="#050a05" stroke="#1a3a1a" stroke-width="0.3"/>\n`;

// Module name
svg += `  <text x="${cx.toFixed(2)}" y="${(ru2mm(11)).toFixed(2)}" fill="#666" font-family="sans-serif" font-size="2" text-anchor="middle" font-weight="bold">ENDLESS</text>\n`;

// Elements
for (const el of elements) {
  const mx = ru2mm(el.x);
  const my = ru2mm(el.y);
  const c = color(el.kind);
  const sym = symbol(el.kind);
  const r = 2;

  // Circle for jack/button/switch/bezel
  svg += `  <circle cx="${mx.toFixed(2)}" cy="${my.toFixed(2)}" r="${r}" fill="none" stroke="${c}" stroke-width="0.25" opacity="0.6"/>\n`;

  // Label below
  const lx = mx;
  const ly = my + r + 1.6;
  svg += `  <text x="${lx.toFixed(2)}" y="${ly.toFixed(2)}" fill="${c}" font-family="sans-serif" font-size="1.8" text-anchor="middle" opacity="0.8">${el.label}</text>\n`;
}

// Divider lines between sections
const lineY = [ru2mm(118), ru2mm(160), ru2mm(200), ru2mm(240)];
for (const ly of lineY) {
  svg += `  <line x1="2" y1="${ly.toFixed(2)}" x2="${(W - 2).toFixed(2)}" y2="${ly.toFixed(2)}" stroke="#333" stroke-width="0.2"/>\n`;
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
  svg += `  <text x="2.5" y="${sy.toFixed(2)}" fill="#555" font-family="sans-serif" font-size="1.5" font-weight="bold">${sec.text}</text>\n`;
}

// Track labels for outputs
svg += `  <text x="${ru2mm(15).toFixed(2)}" y="${ru2mm(293).toFixed(2)}" fill="#666" font-family="sans-serif" font-size="1.5" text-anchor="middle">A</text>\n`;
svg += `  <text x="${ru2mm(15).toFixed(2)}" y="${ru2mm(333).toFixed(2)}" fill="#666" font-family="sans-serif" font-size="1.5" text-anchor="middle">B</text>\n`;

svg += `</svg>\n`;

process.stdout.write(svg);
