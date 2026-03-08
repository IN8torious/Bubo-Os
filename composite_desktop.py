#!/usr/bin/env python3
# BUBO OS Desktop Compositor v2
# Real Pacific Ocean satellite wallpaper — 8°46'10"S 124°34'04"W — 4,209m deep
# Zoom 3: full eastern Pacific, North America, South America visible

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageEnhance

# ── Load real satellite wallpaper (zoom 3 — full Pacific view) ────────────────
bg = Image.open("/home/ubuntu/RavenOS/pacific_z3.png").convert("RGBA")
bg = bg.resize((1920, 1080), Image.LANCZOS)

# Subtle enhancement — deepen the ocean colors
bg = ImageEnhance.Brightness(bg).enhance(0.82)
bg = ImageEnhance.Contrast(bg).enhance(1.15)
bg = ImageEnhance.Color(bg).enhance(1.1)

# ── Overlay layer ─────────────────────────────────────────────────────────────
overlay = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# ── Very subtle vignette — darkens edges slightly ─────────────────────────────
for i in range(60):
    alpha = int(80 * (1 - i / 60))
    draw.rectangle([i, i, 1920 - i, 1080 - i], outline=(0, 0, 0, alpha))

# ── Floating pill dock — centered bottom ──────────────────────────────────────
DOCK_W = 200
DOCK_H = 80
DOCK_X = (1920 - DOCK_W) // 2
DOCK_Y = 1080 - DOCK_H - 28
DOCK_R = 36

# Dock drop shadow
shadow = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
sdraw = ImageDraw.Draw(shadow)
sdraw.rounded_rectangle(
    [DOCK_X + 6, DOCK_Y + 8, DOCK_X + DOCK_W + 6, DOCK_Y + DOCK_H + 8],
    radius=DOCK_R, fill=(0, 0, 0, 140)
)
shadow = shadow.filter(ImageFilter.GaussianBlur(16))
overlay = Image.alpha_composite(overlay, shadow)
draw = ImageDraw.Draw(overlay)

# Dock body — dark frosted glass
draw.rounded_rectangle(
    [DOCK_X, DOCK_Y, DOCK_X + DOCK_W, DOCK_Y + DOCK_H],
    radius=DOCK_R,
    fill=(12, 12, 22, 210),
    outline=(80, 15, 15, 200),
    width=1
)
# Inner highlight
draw.rounded_rectangle(
    [DOCK_X + 1, DOCK_Y + 1, DOCK_X + DOCK_W - 1, DOCK_Y + DOCK_H - 1],
    radius=DOCK_R - 1,
    outline=(139, 0, 0, 80),
    width=1
)

# ── CROW icon ─────────────────────────────────────────────────────────────────
ICON_SIZE = 52
ICON_X = DOCK_X + (DOCK_W - ICON_SIZE) // 2
ICON_Y = DOCK_Y + (DOCK_H - ICON_SIZE) // 2

# Icon tile
draw.rounded_rectangle(
    [ICON_X, ICON_Y, ICON_X + ICON_SIZE, ICON_Y + ICON_SIZE],
    radius=12,
    fill=(22, 6, 6, 245),
    outline=(120, 0, 0, 220),
    width=1
)

cx = ICON_X + ICON_SIZE // 2
cy = ICON_Y + ICON_SIZE // 2

# Glow
draw.ellipse([cx - 18, cy - 18, cx + 18, cy + 18], fill=(139, 0, 0, 35))
draw.ellipse([cx - 12, cy - 12, cx + 12, cy + 12], fill=(139, 0, 0, 55))

# Crow silhouette
draw.ellipse([cx - 8, cy - 2, cx + 8, cy + 10], fill=(190, 10, 10, 255))   # body
draw.ellipse([cx - 5, cy - 12, cx + 5, cy - 2], fill=(190, 10, 10, 255))   # head
draw.polygon([(cx + 4, cy - 8), (cx + 13, cy - 7), (cx + 4, cy - 6)], fill=(210, 30, 30, 255))  # beak
draw.polygon([(cx - 8, cy + 2), (cx - 19, cy - 5), (cx - 6, cy + 7)], fill=(170, 0, 0, 255))    # left wing
draw.polygon([(cx + 8, cy + 2), (cx + 17, cy - 3), (cx + 6, cy + 7)], fill=(170, 0, 0, 255))    # right wing
draw.polygon([(cx - 5, cy + 10), (cx - 9, cy + 19), (cx + 9, cy + 19), (cx + 5, cy + 10)], fill=(170, 0, 0, 255))  # tail

# Active dot
draw.ellipse([cx - 2, DOCK_Y + DOCK_H - 9, cx + 2, DOCK_Y + DOCK_H - 5], fill=(180, 0, 0, 230))

# ── Fonts ─────────────────────────────────────────────────────────────────────
try:
    font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 11)
    font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13)
except:
    font_tiny = ImageFont.load_default()
    font_small = font_tiny

# ── NO MAS DISADVANTAGED — bottom right, quiet gold ──────────────────────────
draw.text((1920 - 192, 1080 - 24), "NO MAS DISADVANTAGED", fill=(255, 215, 0, 160), font=font_small)

# ── Coordinates — ultra subtle white ─────────────────────────────────────────
draw.text((1920 - 228, 1080 - 42), "8°46'10\"S  124°34'04\"W  -4,209m", fill=(255, 255, 255, 55), font=font_tiny)

# ── Composite and save ────────────────────────────────────────────────────────
final = Image.alpha_composite(bg, overlay)
final = final.convert("RGB")
final.save("/home/ubuntu/RavenOS/bubo_desktop_final.png", quality=96)
print(f"Desktop composited: bubo_desktop_final.png  {final.size}")
