#!/usr/bin/env python3
# BUBO OS Desktop Compositor
# Composites the CROW dock UI over the real Pacific Ocean satellite wallpaper
# Coordinates: 8°46'10.48"S 124°34'04.93"W — 4,209m deep Pacific

from PIL import Image, ImageDraw, ImageFilter, ImageFont
import math

# ── Load the real satellite wallpaper ────────────────────────────────────────
bg = Image.open("/home/ubuntu/RavenOS/pacific_satellite_z4.png").convert("RGBA")
bg = bg.resize((1920, 1080), Image.LANCZOS)

# ── Enhance the ocean — slightly darken and add depth ────────────────────────
from PIL import ImageEnhance
bg = ImageEnhance.Brightness(bg).enhance(0.75)
bg = ImageEnhance.Contrast(bg).enhance(1.2)

# ── Create the overlay layer ──────────────────────────────────────────────────
overlay = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# ── Floating pill dock — centered bottom ──────────────────────────────────────
DOCK_W = 120
DOCK_H = 72
DOCK_X = (1920 - DOCK_W) // 2
DOCK_Y = 1080 - DOCK_H - 24
DOCK_R = 28  # corner radius

# Dock shadow (soft blur)
shadow = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
sdraw = ImageDraw.Draw(shadow)
sdraw.rounded_rectangle(
    [DOCK_X + 4, DOCK_Y + 6, DOCK_X + DOCK_W + 4, DOCK_Y + DOCK_H + 6],
    radius=DOCK_R, fill=(0, 0, 0, 120)
)
shadow = shadow.filter(ImageFilter.GaussianBlur(12))
overlay = Image.alpha_composite(overlay, shadow)
draw = ImageDraw.Draw(overlay)

# Dock body — dark frosted glass pill
draw.rounded_rectangle(
    [DOCK_X, DOCK_Y, DOCK_X + DOCK_W, DOCK_Y + DOCK_H],
    radius=DOCK_R,
    fill=(15, 15, 28, 200),
    outline=(60, 20, 20, 180),
    width=1
)

# Subtle inner glow on dock
draw.rounded_rectangle(
    [DOCK_X + 1, DOCK_Y + 1, DOCK_X + DOCK_W - 1, DOCK_Y + DOCK_H - 1],
    radius=DOCK_R - 1,
    outline=(139, 0, 0, 60),
    width=1
)

# ── CROW icon inside dock ─────────────────────────────────────────────────────
ICON_SIZE = 44
ICON_X = DOCK_X + (DOCK_W - ICON_SIZE) // 2
ICON_Y = DOCK_Y + (DOCK_H - ICON_SIZE) // 2

# Icon tile — dark rounded square
draw.rounded_rectangle(
    [ICON_X, ICON_Y, ICON_X + ICON_SIZE, ICON_Y + ICON_SIZE],
    radius=10,
    fill=(25, 8, 8, 240),
    outline=(100, 0, 0, 200),
    width=1
)

# Crow silhouette — draw a simple crow shape with circles and polygons
cx = ICON_X + ICON_SIZE // 2
cy = ICON_Y + ICON_SIZE // 2

# Glow behind crow
draw.ellipse([cx - 14, cy - 14, cx + 14, cy + 14], fill=(139, 0, 0, 40))
draw.ellipse([cx - 10, cy - 10, cx + 10, cy + 10], fill=(139, 0, 0, 60))

# Crow body (simplified silhouette)
# Body
draw.ellipse([cx - 7, cy - 2, cx + 7, cy + 9], fill=(180, 0, 0, 255))
# Head
draw.ellipse([cx - 4, cy - 10, cx + 4, cy - 2], fill=(180, 0, 0, 255))
# Beak
draw.polygon([(cx + 3, cy - 7), (cx + 10, cy - 6), (cx + 3, cy - 5)], fill=(200, 20, 20, 255))
# Left wing
draw.polygon([(cx - 7, cy + 2), (cx - 16, cy - 4), (cx - 5, cy + 6)], fill=(160, 0, 0, 255))
# Right wing
draw.polygon([(cx + 7, cy + 2), (cx + 14, cy - 2), (cx + 5, cy + 6)], fill=(160, 0, 0, 255))
# Tail
draw.polygon([(cx - 4, cy + 9), (cx - 8, cy + 16), (cx + 8, cy + 16), (cx + 4, cy + 9)], fill=(160, 0, 0, 255))

# Active dot below icon
draw.ellipse([cx - 2, DOCK_Y + DOCK_H - 8, cx + 2, DOCK_Y + DOCK_H - 4], fill=(139, 0, 0, 220))

# ── NO MAS DISADVANTAGED — bottom right, tiny and quiet ──────────────────────
try:
    font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 11)
except:
    font_small = ImageFont.load_default()

text = "NO MAS DISADVANTAGED"
draw.text((1920 - 170, 1080 - 22), text, fill=(255, 215, 0, 140), font=font_small)

# ── Coordinates watermark — ultra subtle ─────────────────────────────────────
coord_text = "8°46'10\"S  124°34'04\"W  -4,209m"
draw.text((1920 - 220, 1080 - 38), coord_text, fill=(255, 255, 255, 50), font=font_small)

# ── Composite and save ────────────────────────────────────────────────────────
final = Image.alpha_composite(bg, overlay)
final = final.convert("RGB")
final.save("/home/ubuntu/RavenOS/bubo_desktop_final.png", quality=95)
print("Desktop composited: bubo_desktop_final.png")
