#!/usr/bin/env python3
# BUBO OS Desktop Compositor — Storm State Preview
# Shows the desktop when Boo is working hard:
# Akatsuki dawn darkened, rain particles, lightning bolts, weather widget active
# Brunswick, Ohio 44212 — real weather, real storm

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageEnhance
from datetime import datetime
import random
import math

random.seed(42)

# ── Load Akatsuki dawn wallpaper ──────────────────────────────────────────────
bg = Image.open("/home/ubuntu/RavenOS/akatsuki_dawn_wallpaper.png").convert("RGBA")
bg = bg.resize((1920, 1080), Image.LANCZOS)

# Storm darkening — sky gets heavy
bg = ImageEnhance.Brightness(bg).enhance(0.45)
bg = ImageEnhance.Contrast(bg).enhance(1.3)
bg = ImageEnhance.Color(bg).enhance(0.7)  # desaturate slightly in storm

# ── Storm overlay layer ───────────────────────────────────────────────────────
storm = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
sdraw = ImageDraw.Draw(storm)

# Dark storm clouds across the top third
for i in range(60):
    x = random.randint(-100, 1920)
    y = random.randint(-20, 340)
    w = random.randint(200, 600)
    h = random.randint(60, 180)
    alpha = random.randint(30, 90)
    sdraw.ellipse([x, y, x + w, y + h], fill=(8, 4, 12, alpha))

storm = storm.filter(ImageFilter.GaussianBlur(18))
bg = Image.alpha_composite(bg, storm)

# ── Rain particles ────────────────────────────────────────────────────────────
rain_layer = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
rdraw = ImageDraw.Draw(rain_layer)

for _ in range(1800):
    x = random.randint(0, 1920)
    y = random.randint(0, 1080)
    length = random.randint(8, 22)
    angle = random.uniform(-0.25, -0.15)  # slight wind angle
    x2 = int(x + length * math.sin(angle))
    y2 = int(y + length)
    alpha = random.randint(60, 140)
    rdraw.line([x, y, x2, y2], fill=(140, 165, 210, alpha), width=1)

bg = Image.alpha_composite(bg, rain_layer)

# ── Lightning bolts ───────────────────────────────────────────────────────────
lightning_layer = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
ldraw = ImageDraw.Draw(lightning_layer)

def midpoint_lightning(x1, y1, x2, y2, roughness=2.2, depth=7):
    """Fractal midpoint displacement — how real lightning actually forms."""
    if depth == 0:
        return [(x1, y1), (x2, y2)]
    mx = (x1 + x2) / 2 + random.gauss(0, roughness * abs(x2 - x1 + y2 - y1) * 0.04)
    my = (y1 + y2) / 2 + random.gauss(0, roughness * abs(y2 - y1) * 0.04)
    left  = midpoint_lightning(x1, y1, mx, my, roughness * 0.62, depth - 1)
    right = midpoint_lightning(mx, my, x2, y2, roughness * 0.62, depth - 1)
    return left + right[1:]

def draw_realistic_lightning(img, x1, y1, x2, y2, alpha=220, branch_prob=0.38, depth=7):
    """Draw a photorealistic lightning bolt with layered glow and organic branches."""
    pts = midpoint_lightning(x1, y1, x2, y2, depth=depth)

    # Layer 1 — wide outer glow (purple-blue, very soft)
    glow3 = Image.new("RGBA", img.size, (0, 0, 0, 0))
    g3d = ImageDraw.Draw(glow3)
    for i in range(len(pts) - 1):
        g3d.line([pts[i], pts[i+1]], fill=(160, 120, 255, max(8, alpha // 14)), width=18)
    glow3 = glow3.filter(ImageFilter.GaussianBlur(12))
    img.alpha_composite(glow3)

    # Layer 2 — mid glow (white-blue)
    glow2 = Image.new("RGBA", img.size, (0, 0, 0, 0))
    g2d = ImageDraw.Draw(glow2)
    for i in range(len(pts) - 1):
        g2d.line([pts[i], pts[i+1]], fill=(200, 200, 255, max(18, alpha // 6)), width=7)
    glow2 = glow2.filter(ImageFilter.GaussianBlur(4))
    img.alpha_composite(glow2)

    # Layer 3 — bright core (white-yellow, sharp)
    core = Image.new("RGBA", img.size, (0, 0, 0, 0))
    cd = ImageDraw.Draw(core)
    for i in range(len(pts) - 1):
        cd.line([pts[i], pts[i+1]], fill=(255, 252, 220, alpha), width=2)
    img.alpha_composite(core)

    # Organic branches — spawn from random points along the bolt
    if depth > 3:
        for i in range(2, len(pts) - 2, max(1, len(pts) // 5)):
            if random.random() < branch_prob:
                bx = pts[i][0] + random.randint(30, 100) * random.choice([-1, 1])
                by = pts[i][1] + random.randint(60, 180)
                draw_realistic_lightning(img, int(pts[i][0]), int(pts[i][1]),
                                         int(bx), int(by),
                                         alpha=alpha // 2,
                                         branch_prob=branch_prob * 0.5,
                                         depth=depth - 3)

# Three photorealistic lightning bolts
draw_realistic_lightning(lightning_layer, 360, 0,   310, 520, alpha=230)
draw_realistic_lightning(lightning_layer, 1080, 0,  1160, 560, alpha=210)
draw_realistic_lightning(lightning_layer, 1560, 0,  1500, 420, alpha=200)

# Ambient lightning flash — subtle screen-wide glow
flash = Image.new("RGBA", (1920, 1080), (180, 160, 255, 18))
bg = Image.alpha_composite(bg, flash)
bg = Image.alpha_composite(bg, lightning_layer)

# ── Overlay UI layer ──────────────────────────────────────────────────────────
overlay = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# Vignette
for i in range(60):
    alpha = int(90 * (1 - i / 60))
    draw.rectangle([i, i, 1920 - i, 1080 - i], outline=(0, 0, 0, alpha))

# ── Bottom status bar ─────────────────────────────────────────────────────────
BAR_H = 52
BAR_Y = 1080 - BAR_H

draw.rectangle([0, BAR_Y, 1920, 1080], fill=(8, 4, 18, 210))
draw.line([0, BAR_Y, 1920, BAR_Y], fill=(100, 20, 20, 200), width=1)
draw.line([0, BAR_Y + 1, 1920, BAR_Y + 1], fill=(60, 10, 10, 80), width=1)

# ── Fonts ─────────────────────────────────────────────────────────────────────
try:
    font_reg  = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    font_bold = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 13)
    font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 10)
    font_sm   = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 11)
except:
    font_reg = font_bold = font_tiny = font_sm = ImageFont.load_default()

bar_cy = BAR_Y + BAR_H // 2

# ── NASA logo ─────────────────────────────────────────────────────────────────
try:
    nasa = Image.open("/home/ubuntu/RavenOS/logo_nasa.png").convert("RGBA")
    nasa_h = 26
    nasa_w = int(nasa.width * nasa_h / nasa.height)
    nasa = nasa.resize((nasa_w, nasa_h), Image.LANCZOS)
    nasa_x = 14
    nasa_y = bar_cy - nasa_h // 2
    overlay.paste(nasa, (nasa_x, nasa_y), nasa)
    nasa_end_x = nasa_x + nasa_w + 10
except:
    nasa_end_x = 14

# ── Google logo ───────────────────────────────────────────────────────────────
try:
    goog = Image.open("/home/ubuntu/RavenOS/logo_google.png").convert("RGBA")
    goog_h = 18
    goog_w = int(goog.width * goog_h / goog.height)
    goog = goog.resize((goog_w, goog_h), Image.LANCZOS)
    goog_x = nasa_end_x + 6
    goog_y = bar_cy - goog_h // 2
    overlay.paste(goog, (goog_x, goog_y), goog)
    goog_end_x = goog_x + goog_w + 14
except:
    goog_end_x = nasa_end_x + 80

draw = ImageDraw.Draw(overlay)

# ── Opera GX style contributor icon bubbles ───────────────────────────────────
# Each contributor gets their own glowing icon slot — no text, just presence
ICON_SLOT_SIZE = 36
ICON_SLOT_PAD  = 8
SLOT_Y = bar_cy - ICON_SLOT_SIZE // 2

contributors = [
    ('logo_nasa',   (255, 80,  80),  'NASA'),
    ('logo_google', (80,  160, 255), 'Google'),
    ('logo_owm',    (80,  200, 255), 'OpenWeatherMap'),
    ('logo_qt',     (100, 220, 100), 'Qt'),
    ('logo_epic',   (200, 200, 200), 'Epic'),
    ('logo_opera',  (255, 60,  60),  'Opera GX'),
]

slot_x = 10
for (logo_name, glow_color, label) in contributors:
    sx = slot_x
    sy = SLOT_Y
    sw = ICON_SLOT_SIZE
    sh = ICON_SLOT_SIZE

    # Glow halo behind the bubble
    halo = Image.new('RGBA', (1920, 1080), (0,0,0,0))
    hd = ImageDraw.Draw(halo)
    hd.rounded_rectangle([sx-4, sy-4, sx+sw+4, sy+sh+4],
                          radius=10,
                          fill=(*glow_color, 40))
    halo = halo.filter(ImageFilter.GaussianBlur(6))
    overlay.alpha_composite(halo)
    draw = ImageDraw.Draw(overlay)

    # Dark glass bubble
    draw.rounded_rectangle([sx, sy, sx+sw, sy+sh],
                            radius=8,
                            fill=(14, 8, 22, 210),
                            outline=(*glow_color, 130), width=1)

    # Logo inside the bubble
    try:
        logo = Image.open(f'/home/ubuntu/RavenOS/{logo_name}.png').convert('RGBA')
        pad = 5
        logo = logo.resize((sw - pad*2, sh - pad*2), Image.LANCZOS)
        overlay.paste(logo, (sx + pad, sy + pad), logo)
    except:
        # Fallback: colored dot
        draw.ellipse([sx+10, sy+10, sx+sw-10, sy+sh-10], fill=(*glow_color, 200))

    draw = ImageDraw.Draw(overlay)
    slot_x += sw + ICON_SLOT_PAD

# ── Right side: clock only (digits, no labels) ────────────────────────────────
now = datetime.now()
time_str = now.strftime("%H:%M")
try:
    font_clock = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16)
except:
    font_clock = font_bold
time_w = draw.textlength(time_str, font=font_clock)
draw.text((1920 - time_w - 16, bar_cy - 9), time_str,
          fill=(240, 240, 255, 200), font=font_clock)

# Storm pulse dot — center of bar, Boo is working
draw.ellipse([(1920//2 - 4), bar_cy - 4, (1920//2 + 4), bar_cy + 4],
             fill=(220, 60, 60, 255))

# ── CROW dock — floating pill ─────────────────────────────────────────────────
DOCK_W = 200
DOCK_H = 78
DOCK_X = (1920 - DOCK_W) // 2
DOCK_Y = BAR_Y - DOCK_H - 18
DOCK_R = 36

shadow = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
sdraw2 = ImageDraw.Draw(shadow)
sdraw2.rounded_rectangle(
    [DOCK_X + 6, DOCK_Y + 8, DOCK_X + DOCK_W + 6, DOCK_Y + DOCK_H + 8],
    radius=DOCK_R, fill=(0, 0, 0, 160)
)
shadow = shadow.filter(ImageFilter.GaussianBlur(16))
overlay = Image.alpha_composite(overlay, shadow)
draw = ImageDraw.Draw(overlay)

# Dock glows red in storm
draw.rounded_rectangle(
    [DOCK_X, DOCK_Y, DOCK_X + DOCK_W, DOCK_Y + DOCK_H],
    radius=DOCK_R, fill=(14, 6, 22, 225),
    outline=(160, 20, 20, 240), width=2
)

# CROW icon
ICON_SIZE = 52
ICON_X = DOCK_X + (DOCK_W - ICON_SIZE) // 2
ICON_Y = DOCK_Y + (DOCK_H - ICON_SIZE) // 2

draw.rounded_rectangle(
    [ICON_X, ICON_Y, ICON_X + ICON_SIZE, ICON_Y + ICON_SIZE],
    radius=12, fill=(22, 6, 6, 245), outline=(160, 20, 20, 240), width=1
)

cx = ICON_X + ICON_SIZE // 2
cy = ICON_Y + ICON_SIZE // 2

draw.ellipse([cx - 18, cy - 18, cx + 18, cy + 18], fill=(139, 0, 0, 50))
draw.ellipse([cx - 12, cy - 12, cx + 12, cy + 12], fill=(139, 0, 0, 70))
draw.ellipse([cx - 8, cy - 2, cx + 8, cy + 10], fill=(210, 20, 20, 255))
draw.ellipse([cx - 5, cy - 12, cx + 5, cy - 2], fill=(210, 20, 20, 255))
draw.polygon([(cx + 4, cy - 8), (cx + 13, cy - 7), (cx + 4, cy - 6)], fill=(230, 40, 40, 255))
draw.polygon([(cx - 8, cy + 2), (cx - 19, cy - 5), (cx - 6, cy + 7)], fill=(190, 10, 10, 255))
draw.polygon([(cx + 8, cy + 2), (cx + 17, cy - 3), (cx + 6, cy + 7)], fill=(190, 10, 10, 255))
draw.polygon([(cx - 5, cy + 10), (cx - 9, cy + 19), (cx + 9, cy + 19), (cx + 5, cy + 10)], fill=(190, 10, 10, 255))
# Storm pulse dot
draw.ellipse([cx - 3, DOCK_Y + DOCK_H - 10, cx + 3, DOCK_Y + DOCK_H - 4],
             fill=(220, 60, 60, 255))

# ── Composite and save ────────────────────────────────────────────────────────
final = Image.alpha_composite(bg, overlay)
final = final.convert("RGB")
final.save("/home/ubuntu/RavenOS/bubo_desktop_storm.png", quality=96)
print(f"Storm desktop composited: {final.size}")
