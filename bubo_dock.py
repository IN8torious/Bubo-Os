"""
bubo_dock.py — BUBO OS full dock renderer
Voxel-style 3D pixel art icons — chunky, simple, instantly readable
Eight slots: Home, Files, Browser, Games, CROW, Terminal, Settings, Boo (Rinnegan)
"""

from PIL import Image, ImageDraw, ImageFilter
import math

# ── Dock geometry ──────────────────────────────────────────────────────────────
ICON_SIZE   = 24
SLOT_GAP    = 6
DOCK_PAD_X  = 10
DOCK_PAD_Y  = 6
NUM_ICONS   = 8
DOCK_W      = NUM_ICONS * ICON_SIZE + (NUM_ICONS - 1) * SLOT_GAP + DOCK_PAD_X * 2
DOCK_H      = ICON_SIZE + DOCK_PAD_Y * 2
DOCK_R      = DOCK_H // 2

# ── Voxel helper ───────────────────────────────────────────────────────────────
def vox(d, x, y, size, color, depth=4):
    """
    Draw a single voxel block — top face, right face, left face.
    Gives a chunky isometric 3D look with just three rectangles.
    """
    r, g, b = color[0], color[1], color[2]
    a = color[3] if len(color) > 3 else 255

    # Top face (lightest)
    top = (min(r+60,255), min(g+60,255), min(b+60,255), a)
    d.rectangle([x, y, x+size-1, y+size//2-1], fill=top)

    # Front face (base color)
    front = (r, g, b, a)
    d.rectangle([x, y+size//2, x+size-1, y+size-1], fill=front)

    # Right shadow strip
    dark = (max(r-50,0), max(g-50,0), max(b-50,0), a)
    d.rectangle([x+size-depth, y, x+size-1, y+size-1], fill=dark)

    # Bottom shadow strip
    d.rectangle([x, y+size-depth, x+size-1, y+size-1], fill=dark)


def vox_grid(d, grid, ox, oy, cell, color, depth=3):
    """
    Draw a 2D grid of voxels.
    grid is a list of strings, '#' = filled, '.' = empty.
    ox, oy = top-left origin. cell = pixel size of each voxel.
    """
    for row_i, row in enumerate(grid):
        for col_i, ch in enumerate(row):
            if ch == '#':
                vox(d,
                    ox + col_i * cell,
                    oy + row_i * cell,
                    cell, color, depth)


# ── Icon pixel art grids (8x8) ─────────────────────────────────────────────────

GRID_HOME = [
    "...##...",
    "..####..",
    ".######.",
    "########",
    ".##..##.",
    ".##..##.",
    ".######.",
    "........",
]

GRID_FILES = [
    "####....",
    "########",
    "########",
    "########",
    "########",
    "########",
    "########",
    "........",
]

GRID_BROWSER = [
    ".######.",
    "#.####.#",
    "##.##.##",
    "########",
    "########",
    "##.##.##",
    "#.####.#",
    ".######.",
]

GRID_GAMES = [
    "........",
    ".######.",
    "#.#.#..#",
    "########",
    "#..#.#.#",
    ".######.",
    "..#..#..",
    "........",
]

GRID_TERMINAL = [
    "........",
    ".##.....",
    "..##....",
    "...##...",
    "..##....",
    ".##.....",
    "........",
    "....####",
]

GRID_SETTINGS = [
    "...##...",
    "..####..",
    ".##..##.",
    "##....##",
    ".##..##.",
    "..####..",
    "...##...",
    "........",
]

GRID_CROW = [
    "...##...",
    "..####..",
    ".######.",
    "..####..",
    ".######.",
    "########",
    ".##..##.",
    "........",
]

GRID_RINNEGAN = [
    ".######.",
    "#.####.#",
    "##.##.##",
    "###..###",
    "###..###",
    "##.##.##",
    "#.####.#",
    ".######.",
]

# ── Icon draw functions ────────────────────────────────────────────────────────

def _icon(d, cx, cy, icon_s, grid, color, depth=3):
    """Center a voxel grid inside an icon slot."""
    cell = icon_s // 8
    ox = cx - (8 * cell) // 2
    oy = cy - (8 * cell) // 2
    vox_grid(d, grid, ox, oy, cell, color, depth)


def draw_icon_home(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_HOME, color)

def draw_icon_files(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_FILES, color)

def draw_icon_browser(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_BROWSER, color)

def draw_icon_games(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_GAMES, color)

def draw_icon_terminal(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_TERMINAL, color)

def draw_icon_settings(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_SETTINGS, color)

def draw_icon_crow(d, cx, cy, icon_s, color, storm=False, **kw):
    c = (220, 40, 40, 255) if storm else color
    _icon(d, cx, cy, icon_s, GRID_CROW, c)
    # Storm pulse ring
    if storm:
        r = icon_s // 2 + 4
        d.ellipse([cx-r, cy-r, cx+r, cy+r], outline=(220, 60, 60, 120), width=2)

def draw_icon_bubo(d, cx, cy, icon_s, color, **kw):
    _icon(d, cx, cy, icon_s, GRID_RINNEGAN, color)
    # Pupil dot
    cell = icon_s // 8
    d.ellipse([cx - cell, cy - cell, cx + cell, cy + cell],
              fill=(10, 6, 20, 255))


# ── Icon registry ──────────────────────────────────────────────────────────────
DOCK_ICONS = [
    # (label,     draw_fn,          glow_color RGB,       is_crow)
    ('Home',      draw_icon_home,     (100, 180, 255),    False),
    ('Files',     draw_icon_files,    (255, 200,  80),    False),
    ('Browser',   draw_icon_browser,  ( 80, 220, 180),    False),
    ('Games',     draw_icon_games,    (180,  80, 255),    False),
    ('CROW',      draw_icon_crow,     (210,  30,  30),    True ),
    ('Terminal',  draw_icon_terminal, ( 80, 255, 120),    False),
    ('Settings',  draw_icon_settings, (200, 200, 200),    False),
    ('Boo',       draw_icon_bubo,     (160, 100, 255),    False),
]


# ── Main dock renderer ─────────────────────────────────────────────────────────

def draw_dock(img, storm=False):
    """
    Composite the full BUBO OS dock onto img (RGBA 1920×1080).
    storm=True activates red glow on CROW and the storm pulse dot.
    """
    W, H = img.size
    dock_x = (W - DOCK_W) // 2
    dock_y = H - DOCK_H - 22

    overlay = Image.new('RGBA', (W, H), (0, 0, 0, 0))

    # Drop shadow
    shadow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    sd.rounded_rectangle(
        [dock_x + 8, dock_y + 10, dock_x + DOCK_W + 8, dock_y + DOCK_H + 10],
        radius=DOCK_R, fill=(0, 0, 0, 160))
    shadow = shadow.filter(ImageFilter.GaussianBlur(18))
    overlay.alpha_composite(shadow)

    d = ImageDraw.Draw(overlay)

    # Pill background
    pill_outline = (160, 20, 20, 200) if storm else (60, 40, 80, 200)
    d.rounded_rectangle(
        [dock_x, dock_y, dock_x + DOCK_W, dock_y + DOCK_H],
        radius=DOCK_R, fill=(12, 6, 22, 225), outline=pill_outline, width=2)

    # Icon slots
    slot_x = dock_x + DOCK_PAD_X
    slot_y = dock_y + DOCK_PAD_Y

    for label, draw_fn, glow_rgb, is_crow in DOCK_ICONS:
        # Glow halo
        halo = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        hd = ImageDraw.Draw(halo)
        hd.rounded_rectangle(
            [slot_x - 3, slot_y - 3, slot_x + ICON_SIZE + 3, slot_y + ICON_SIZE + 3],
            radius=10, fill=(*glow_rgb, 30))
        halo = halo.filter(ImageFilter.GaussianBlur(7))
        overlay.alpha_composite(halo)
        d = ImageDraw.Draw(overlay)

        # Slot bubble
        d.rounded_rectangle(
            [slot_x, slot_y, slot_x + ICON_SIZE, slot_y + ICON_SIZE],
            radius=10, fill=(18, 8, 30, 215), outline=(*glow_rgb, 100), width=1)

        # Draw icon
        icon_layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        id_ = ImageDraw.Draw(icon_layer)
        cx = slot_x + ICON_SIZE // 2
        cy = slot_y + ICON_SIZE // 2
        draw_fn(id_, cx, cy, ICON_SIZE, (*glow_rgb, 255), storm=storm)
        overlay.alpha_composite(icon_layer)
        d = ImageDraw.Draw(overlay)

        slot_x += ICON_SIZE + SLOT_GAP

    # Storm pulse dot under CROW (index 4)
    if storm:
        crow_cx = (dock_x + DOCK_PAD_X) + 4 * (ICON_SIZE + SLOT_GAP) + ICON_SIZE // 2
        pd = ImageDraw.Draw(overlay)
        pd.ellipse(
            [crow_cx - 4, dock_y + DOCK_H + 4, crow_cx + 4, dock_y + DOCK_H + 12],
            fill=(220, 60, 60, 255))

    img.alpha_composite(overlay)
    return img
