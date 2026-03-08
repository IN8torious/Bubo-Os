#!/usr/bin/env python3
# BUBO OS Desktop Compositor v3
# Real Pacific Ocean satellite wallpaper — 8°46'10"S 124°34'04"W — 4,209m deep
# Translucent bottom status bar absorbs watermarks, shows time/date/coords

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageEnhance
from datetime import datetime

# ── Load real satellite wallpaper (zoom 3 — full Pacific view) ────────────────
bg = Image.open("/home/ubuntu/RavenOS/pacific_z3.png").convert("RGBA")
bg = bg.resize((1920, 1080), Image.LANCZOS)

# Subtle enhancement
bg = ImageEnhance.Brightness(bg).enhance(0.82)
bg = ImageEnhance.Contrast(bg).enhance(1.15)
bg = ImageEnhance.Color(bg).enhance(1.1)

# ── Overlay layer ─────────────────────────────────────────────────────────────
overlay = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# ── Subtle vignette ───────────────────────────────────────────────────────────
for i in range(50):
    alpha = int(70 * (1 - i / 50))
    draw.rectangle([i, i, 1920 - i, 1080 - i], outline=(0, 0, 0, alpha))

# ── Bottom status bar — translucent, sits over watermarks ────────────────────
BAR_H = 36
BAR_Y = 1080 - BAR_H

# Frosted glass bar — dark translucent, absorbs the Google/NASA watermarks
draw.rectangle(
    [0, BAR_Y, 1920, 1080],
    fill=(8, 8, 18, 185)
)
# Subtle top border line
draw.line([0, BAR_Y, 1920, BAR_Y], fill=(60, 15, 15, 160), width=1)

# ── Fonts ─────────────────────────────────────────────────────────────────────
try:
    font_reg  = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    font_bold = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 13)
    font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 10)
except:
    font_reg = font_bold = font_tiny = ImageFont.load_default()

# ── Status bar content ────────────────────────────────────────────────────────
bar_text_y = BAR_Y + 11  # vertically centered in bar

# Left: coordinates + depth
coord_text = "8°46'10\"S  124°34'04\"W  ·  -4,209 m"
draw.text((16, bar_text_y), coord_text, fill=(140, 180, 220, 180), font=font_reg)

# Left separator
draw.text((260, bar_text_y), "·", fill=(80, 80, 100, 150), font=font_reg)

# Imagery credit — subtle, integrated
draw.text((276, bar_text_y), "Imagery ©2026 NASA / Google", fill=(80, 100, 120, 140), font=font_tiny)

# Center: NO MAS DISADVANTAGED
nmd_text = "NO MAS DISADVANTAGED"
nmd_w = draw.textlength(nmd_text, font=font_bold)
draw.text(((1920 - nmd_w) // 2, bar_text_y - 1), nmd_text, fill=(255, 215, 0, 180), font=font_bold)

# Right: time and date
now = datetime.now()
time_str = now.strftime("%I:%M %p")
date_str = now.strftime("%a  %b %d")
time_w = draw.textlength(time_str, font=font_bold)
date_w = draw.textlength(date_str, font=font_reg)

draw.text((1920 - time_w - 16, bar_text_y - 1), time_str, fill=(240, 240, 255, 200), font=font_bold)
draw.text((1920 - time_w - date_w - 28, bar_text_y), date_str, fill=(160, 160, 180, 160), font=font_reg)

# ── Floating pill dock — centered, above status bar ──────────────────────────
DOCK_W = 200
DOCK_H = 78
DOCK_X = (1920 - DOCK_W) // 2
DOCK_Y = BAR_Y - DOCK_H - 18
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

# Dock body
draw.rounded_rectangle(
    [DOCK_X, DOCK_Y, DOCK_X + DOCK_W, DOCK_Y + DOCK_H],
    radius=DOCK_R,
    fill=(12, 12, 22, 215),
    outline=(80, 15, 15, 200),
    width=1
)
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
draw.ellipse([cx - 8, cy - 2, cx + 8, cy + 10], fill=(190, 10, 10, 255))
draw.ellipse([cx - 5, cy - 12, cx + 5, cy - 2], fill=(190, 10, 10, 255))
draw.polygon([(cx + 4, cy - 8), (cx + 13, cy - 7), (cx + 4, cy - 6)], fill=(210, 30, 30, 255))
draw.polygon([(cx - 8, cy + 2), (cx - 19, cy - 5), (cx - 6, cy + 7)], fill=(170, 0, 0, 255))
draw.polygon([(cx + 8, cy + 2), (cx + 17, cy - 3), (cx + 6, cy + 7)], fill=(170, 0, 0, 255))
draw.polygon([(cx - 5, cy + 10), (cx - 9, cy + 19), (cx + 9, cy + 19), (cx + 5, cy + 10)], fill=(170, 0, 0, 255))

# Active dot
draw.ellipse([cx - 2, DOCK_Y + DOCK_H - 9, cx + 2, DOCK_Y + DOCK_H - 5], fill=(180, 0, 0, 230))

# ── Composite and save ────────────────────────────────────────────────────────
final = Image.alpha_composite(bg, overlay)
final = final.convert("RGB")
final.save("/home/ubuntu/RavenOS/bubo_desktop_final.png", quality=96)
print(f"Desktop composited: bubo_desktop_final.png  {final.size}")
