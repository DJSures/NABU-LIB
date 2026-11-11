// ****************************************************************************************
// NABU MULTIPLAYER HOCKEY - graphics generator
//
// Draws the rink as a 256 x 192 pixel picture, cuts it into 8x8 tiles that obey the
// TMS9918 Graphic Mode 2 rule (two colours per 8-pixel row), removes duplicate tiles,
// and writes graphics.h with the tile patterns, tile colours, the name table for the
// rink and the 16x16 sprite patterns.
//
// Run from this folder whenever you change the drawing:
//
//   node makeGraphics.js
//
// DJ Sures (c) 2026
// https://nabu.ca
// ****************************************************************************************

const fs = require('fs');
const path = require('path');

// TMS9918 colours used by the rink
const BLACK = 1;
const BOARD = 4;   // dark blue boards (same as the backdrop)
const CREASE = 5;  // light blue
const RED = 8;     // medium red lines, circle and nets
const ICE = 15;    // white ice

// Rink rows on screen. Rows 0 and 1 are the scoreboard, rows 2 - 23 are the rink.
const FIRST_ROW = 2;
const ROWS = 22;
const COLS = 32;

// Mirrors a pixel into the top-left quarter so every marking only has to be described
// once. The ice runs x 8 - 247 and y 24 - 183, so the mirror lines are x 127.5 and
// y 103.5.
function mirror(x, y) {

  const mx = x <= 127 ? x : 255 - x;
  const my = y <= 103 ? y : 207 - y;

  return [mx, my];
}

// True when the pixel is on the ice (inside the boards and the rounded corners).
function isIce(x, y) {

  if (x < 8 || x > 247 || y < 24 || y > 183)
    return false;

  const [mx, my] = mirror(x, y);

  // Rounded corner: radius 20 around (28, 44)
  if (mx < 28 && my < 44) {

    const dx = mx + 0.5 - 28;
    const dy = my + 0.5 - 44;

    return dx * dx + dy * dy <= 20 * 20;
  }

  return true;
}

// Colour of one pixel of the rink.
function rinkPixel(x, y) {

  if (!isIce(x, y))
    return BOARD;

  const [mx, my] = mirror(x, y);

  let c = ICE;

  // Goal crease: light blue half circle in front of each net
  {
    const dx = mx + 0.5 - 28;
    const dy = my + 0.5 - 104;

    if (mx >= 32 && dx * dx + dy * dy <= 14 * 14)
      c = CREASE;
  }

  // Goal lines
  if (mx === 28)
    c = RED;

  // Blue lines
  if (mx >= 84 && mx <= 87)
    c = BOARD;

  // Centre red line
  if (mx === 127)
    c = RED;

  // Centre circle
  {
    const dx = x + 0.5 - 128;
    const dy = y + 0.5 - 104;
    const d = Math.sqrt(dx * dx + dy * dy);

    if (d >= 18.5 && d < 20)
      c = RED;
  }

  // End zone face-off dots
  if (Math.abs(mx - 64) <= 1 && Math.abs(my - 64) <= 1)
    c = RED;

  // Nets: frame and mesh behind each goal line
  if (mx >= 18 && mx <= 28 && my >= 90) {

    if (mx === 18 || my === 90 || mx === 28)
      c = RED;
    else if ((mx + my) % 4 === 0 || (mx - my + 400) % 4 === 0)
      c = RED;
    else
      c = ICE;
  }

  return c;
}

// ---------------------------------------------------------------------------
// Cut the rink into tiles
// ---------------------------------------------------------------------------

// One tile = 8 pattern bytes + 8 colour bytes (fg << 4 | bg per row)
function makeTile(tx, ty) {

  const pattern = [];
  const color = [];

  for (let r = 0; r < 8; r++) {

    const y = ty * 8 + r;
    const px = [];
    const counts = {};

    for (let b = 0; b < 8; b++) {

      const c = rinkPixel(tx * 8 + b, y);

      px.push(c);

      counts[c] = (counts[c] || 0) + 1;
    }

    const used = Object.keys(counts).map(Number).sort((a, b) => counts[b] - counts[a]);

    // A row with one colour: no bits set, that colour as the background
    if (used.length === 1) {

      pattern.push(0);
      color.push((used[0] << 4) | used[0]);

      continue;
    }

    // Two colours per row. Ice is the background when present. A third colour
    // (where a line runs into the boards) is dropped into the background.
    const bg = used.includes(ICE) ? ICE : used[0];
    const fg = used.find(c => c !== bg);

    let bits = 0;

    for (let b = 0; b < 8; b++) {

      if (px[b] === fg)
        bits |= 0x80 >> b;
    }

    pattern.push(bits);
    color.push((fg << 4) | bg);
  }

  return { pattern, color };
}

// Pattern ids free for rink tiles: 128 - 255, then 1 - 31. 0 is plain ice and
// 32 - 127 hold the font.
const freeIds = [];

for (let i = 128; i < 256; i++)
  freeIds.push(i);

for (let i = 1; i < 32; i++)
  freeIds.push(i);

const tiles = [];
const tileByKey = {};
const map = [];

// Plain ice is pattern 0 (blank pattern, white on white)
const iceKey = JSON.stringify({ pattern: [0, 0, 0, 0, 0, 0, 0, 0], color: Array(8).fill((ICE << 4) | ICE) });

tileByKey[iceKey] = 0;

for (let ty = FIRST_ROW; ty < FIRST_ROW + ROWS; ty++) {

  for (let tx = 0; tx < COLS; tx++) {

    const tile = makeTile(tx, ty);
    const key = JSON.stringify(tile);

    if (tileByKey[key] === undefined) {

      if (tiles.length >= freeIds.length)
        throw new Error('Too many unique rink tiles');

      tileByKey[key] = freeIds[tiles.length];

      tiles.push(tile);
    }

    map.push(tileByKey[key]);
  }
}

// ---------------------------------------------------------------------------
// Sprites (16x16, one colour each)
// ---------------------------------------------------------------------------

// Makes an empty 16x16 bitmap
function blank() {

  return Array.from({ length: 16 }, () => Array(16).fill(0));
}

function plot(bmp, x, y) {

  x = Math.floor(x);
  y = Math.floor(y);

  if (x >= 0 && x < 16 && y >= 0 && y < 16)
    bmp[y][x] = 1;
}

// Skater seen from above, facing direction d (0 = east, then clockwise in 45 degree
// steps). Shoulders are an ellipse across the facing direction, the stick reaches out
// in front with the blade hooked to one side.
function skater(d) {

  const bmp = blank();
  const a = d * Math.PI / 4;
  const fx = Math.cos(a);
  const fy = Math.sin(a);
  const sx = -fy;
  const sy = fx;

  for (let y = 0; y < 16; y++) {

    for (let x = 0; x < 16; x++) {

      const dx = x + 0.5 - 8;
      const dy = y + 0.5 - 8;
      const u = dx * fx + dy * fy;
      const v = dx * sx + dy * sy;

      if ((u * u) / (3.2 * 3.2) + (v * v) / (5.2 * 5.2) <= 1)
        bmp[y][x] = 1;
    }
  }

  // Stick from the right shoulder out to the blade
  const startX = 8 + sx * 3;
  const startY = 8 + sy * 3;
  const endX = 8 + fx * 7.4 + sx * 0.5;
  const endY = 8 + fy * 7.4 + sy * 0.5;

  for (let t = 0; t <= 1; t += 0.05)
    plot(bmp, startX + (endX - startX) * t, startY + (endY - startY) * t);

  // Blade
  plot(bmp, endX - sx * 1.2, endY - sy * 1.2);
  plot(bmp, endX - sx * 2.2, endY - sy * 2.2);

  return bmp;
}

// Goalie facing east (flip for west): tall and wide with pads and a stick blade
function goalie(flip) {

  const bmp = blank();

  for (let y = 0; y < 16; y++) {

    for (let x = 0; x < 16; x++) {

      const dx = x + 0.5 - 8;
      const dy = y + 0.5 - 8;

      if ((dx * dx) / (4.2 * 4.2) + (dy * dy) / (7.6 * 7.6) <= 1)
        bmp[y][x] = 1;
    }
  }

  // Blocker and glove
  plot(bmp, 12, 4);
  plot(bmp, 12, 5);
  plot(bmp, 12, 11);
  plot(bmp, 12, 12);

  // Stick blade along the ice in front
  for (let y = 9; y < 15; y++)
    plot(bmp, 13, y);

  if (flip)
    return bmp.map(row => row.slice().reverse());

  return bmp;
}

function puck() {

  const bmp = blank();

  for (let x = 6; x <= 9; x++) {

    plot(bmp, x, 7);
    plot(bmp, x, 8);
  }

  plot(bmp, 7, 9);
  plot(bmp, 8, 9);
  plot(bmp, 7, 6);
  plot(bmp, 8, 6);

  return bmp;
}

// TMS9918 16x16 order: left 8 columns rows 0-15, then right 8 columns rows 0-15
function spriteBytes(bmp) {

  const out = [];

  for (let half = 0; half < 2; half++) {

    for (let y = 0; y < 16; y++) {

      let b = 0;

      for (let x = 0; x < 8; x++) {

        if (bmp[y][half * 8 + x])
          b |= 0x80 >> x;
      }

      out.push(b);
    }
  }

  return out;
}

const sprites = [blank()];

for (let d = 0; d < 8; d++)
  sprites.push(skater(d));

sprites.push(goalie(false));
sprites.push(goalie(true));
sprites.push(puck());

// ---------------------------------------------------------------------------
// Write graphics.h
// ---------------------------------------------------------------------------

function hex(v) {

  return '0x' + v.toString(16).toUpperCase().padStart(2, '0');
}

function table(values, perLine) {

  const lines = [];

  for (let i = 0; i < values.length; i += perLine)
    lines.push('  ' + values.slice(i, i + perLine).map(v => (typeof v === 'string' ? v : hex(v))).join(', '));

  return lines.join(',\n');
}

let out = '';

out += '// ****************************************************************************************\n';
out += '// NABU MULTIPLAYER HOCKEY - graphics\n';
out += '//\n';
out += '// GENERATED by makeGraphics.js. Do not edit by hand: change the drawing in\n';
out += '// makeGraphics.js and run "node makeGraphics.js" again.\n';
out += '// ****************************************************************************************\n\n';

out += '// Number of rink tiles in RINK_TILE_ID / RINK_TILE_PATTERN / RINK_TILE_COLOR\n';
out += `#define RINK_TILE_COUNT   ${tiles.length}\n\n`;

out += '// First screen row of the rink and how many rows it covers (rows 0 and 1 are the scoreboard)\n';
out += `#define RINK_FIRST_ROW    ${FIRST_ROW}\n`;
out += `#define RINK_ROWS         ${ROWS}\n\n`;

out += '// Sprite pattern numbers in SPRITE_DATA\n';
out += '#define SPR_BLANK         0\n';
out += '#define SPR_SKATER        1   // + facing direction 0 - 7\n';
out += '#define SPR_GOALIE_EAST   9\n';
out += '#define SPR_GOALIE_WEST   10\n';
out += '#define SPR_PUCK          11\n';
out += `#define SPRITE_COUNT      ${sprites.length}\n\n`;

out += '// Pattern id each rink tile is loaded into\n';
out += 'const uint8_t RINK_TILE_ID[RINK_TILE_COUNT] = {\n';
out += table(tiles.map((t, i) => freeIds[i]), 16) + '\n};\n\n';

out += '// 8 pattern bytes per rink tile\n';
out += 'const uint8_t RINK_TILE_PATTERN[RINK_TILE_COUNT * 8] = {\n';
out += table(tiles.flatMap(t => t.pattern), 16) + '\n};\n\n';

out += '// 8 colour bytes per rink tile (foreground << 4 | background, one per pixel row)\n';
out += 'const uint8_t RINK_TILE_COLOR[RINK_TILE_COUNT * 8] = {\n';
out += table(tiles.flatMap(t => t.color), 16) + '\n};\n\n';

out += '// Pattern id for every tile of screen rows RINK_FIRST_ROW - 23\n';
out += 'const uint8_t RINK_MAP[RINK_ROWS * 32] = {\n';
out += table(map, 32) + '\n};\n\n';

out += '// 16x16 sprite patterns, 32 bytes each\n';
out += 'const uint8_t SPRITE_DATA[SPRITE_COUNT * 32] = {\n';
out += table(sprites.flatMap(spriteBytes), 16) + '\n};\n';

fs.writeFileSync(path.join(__dirname, 'graphics.h'), out);

console.log(`graphics.h written: ${tiles.length} rink tiles, ${sprites.length} sprites`);

// Preview of the sprites in the console, handy while tweaking the drawing
if (process.argv.includes('--preview')) {

  sprites.forEach((bmp, i) => {

    console.log(`sprite ${i}`);

    bmp.forEach(row => console.log(row.map(v => (v ? '#' : '.')).join('')));
  });
}
