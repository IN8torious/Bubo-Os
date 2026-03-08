#!/usr/bin/env python3
# BUBO OS Desktop Compositor v4
# Real Pacific Ocean satellite wallpaper — 8°46'10"S 124°34'04"W — 4,209m deep
# Google + NASA logos built into taskbar — they earned it

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageEnhance
from datetime import datetime

# ── Load real satellite wallpaper ─────────────────────────────────────────────
bg = Image.open("/home/ubuntu/RavenOS/akatsuki_dawn_wallpaper.png").convert("RGBA")
bg = bg.resize((1920, 1080), Image.LANCZOS)
bg = ImageEnhance.Brightness(bg).enhance(0.82)
bg = ImageEnhance.Contrast(bg).enhance(1.15)
bg = ImageEnhance.Color(bg).enhance(1.1)

# ── Overlay layer ─────────────────────────────────────────────────────────────
overlay = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# Subtle vignette
for i in range(50):
    alpha = int(70 * (1 - i / 50))
    draw.rectangle([i, i, 1920 - i, 1080 - i], outline=(0, 0, 0, alpha))

# ── Bottom status bar ─────────────────────────────────────────────────────────
BAR_H = 52
BAR_Y = 1080 - BAR_H

draw.rectangle([0, BAR_Y, 1920, 1080], fill=(8, 8, 18, 190))
draw.line([0, BAR_Y, 1920, BAR_Y], fill=(60, 15, 15, 160), width=1)

# ── Fonts ─────────────────────────────────────────────────────────────────────
try:
    font_reg  = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
    font_bold = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 13)
    font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 10)
except:
    font_reg = font_bold = font_tiny = ImageFont.load_default()

bar_cy = BAR_Y + BAR_H // 2  # vertical center of bar

# ── Load and place NASA logo (left side) ──────────────────────────────────────
try:
    nasa = Image.open("/home/ubuntu/RavenOS/logo_nasa.png").convert("RGBA")
    # Scale to 28px tall, keep aspect ratio
    nasa_h = 26
    nasa_w = int(nasa.width * nasa_h / nasa.height)
    nasa = nasa.resize((nasa_w, nasa_h), Image.LANCZOS)
    nasa_x = 14
    nasa_y = bar_cy - nasa_h // 2
    overlay.paste(nasa, (nasa_x, nasa_y), nasa)
    nasa_end_x = nasa_x + nasa_w + 10
except Exception as e:
    print(f"NASA logo error: {e}")
    nasa_end_x = 14

# ── Load and place Google logo (after NASA) ───────────────────────────────────
try:
    goog = Image.open("/home/ubuntu/RavenOS/logo_google.png").convert("RGBA")
    # Scale to 18px tall — Google wordmark is wide
    goog_h = 18
    goog_w = int(goog.width * goog_h / goog.height)
    goog = goog.resize((goog_w, goog_h), Image.LANCZOS)
    goog_x = nasa_end_x + 6
    goog_y = bar_cy - goog_h // 2
    overlay.paste(goog, (goog_x, goog_y), goog)
    goog_end_x = goog_x + goog_w + 14
except Exception as e:
    print(f"Google logo error: {e}")
    goog_end_x = nasa_end_x + 80

# Separator dot
draw = ImageDraw.Draw(overlay)
draw.text((goog_end_x, bar_cy - 7), "·", fill=(80, 80, 100, 150), font=font_reg)

# Coordinates — subtle blue-white
draw.text((goog_end_x + 14, bar_cy - 7), "8°46'10\"S  124°34'04\"W  ·  -4,209 m", fill=(130, 170, 210, 170), font=font_reg)

# ── Center: NO MAS DISADVANTAGED ─────────────────────────────────────────────
nmd_text = "NO MAS DISADVANTAGED"
nmd_w = draw.textlength(nmd_text, font=font_bold)
draw.text(((1920 - nmd_w) // 2, bar_cy - 8), nmd_text, fill=(255, 215, 0, 190), font=font_bold)

# ── Right: date + time ────────────────────────────────────────────────────────
now = datetime.now()
time_str = now.strftime("%I:%M %p")
date_str = now.strftime("%a  %b %d")
time_w = draw.textlength(time_str, font=font_bold)
date_w = draw.textlength(date_str, font=font_reg)

draw.text((1920 - time_w - 16, bar_cy - 8), time_str, fill=(240, 240, 255, 210), font=font_bold)
draw.text((1920 - time_w - date_w - 28, bar_cy - 7), date_str, fill=(160, 160, 180, 160), font=font_reg)

# ── Floating pill dock — centered, above status bar ──────────────────────────
DOCK_W = 200
DOCK_H = 78
DOCK_X = (1920 - DOCK_W) // 2
DOCK_Y = BAR_Y - DOCK_H - 18
DOCK_R = 36

shadow = Image.new("RGBA", (1920, 1080), (0, 0, 0, 0))
sdraw = ImageDraw.Draw(shadow)
sdraw.rounded_rectangle(
    [DOCK_X + 6, DOCK_Y + 8, DOCK_X + DOCK_W + 6, DOCK_Y + DOCK_H + 8],
    radius=DOCK_R, fill=(0, 0, 0, 140)
)
shadow = shadow.filter(ImageFilter.GaussianBlur(16))
overlay = Image.alpha_composite(overlay, shadow)
draw = ImageDraw.Draw(overlay)

draw.rounded_rectangle(
    [DOCK_X, DOCK_Y, DOCK_X + DOCK_W, DOCK_Y + DOCK_H],
    radius=DOCK_R, fill=(12, 12, 22, 215),
    outline=(80, 15, 15, 200), width=1
)
draw.rounded_rectangle(
    [DOCK_X + 1, DOCK_Y + 1, DOCK_X + DOCK_W - 1, DOCK_Y + DOCK_H - 1],
    radius=DOCK_R - 1, outline=(139, 0, 0, 80), width=1
)

# CROW icon
ICON_SIZE = 52
ICON_X = DOCK_X + (DOCK_W - ICON_SIZE) // 2
ICON_Y = DOCK_Y + (DOCK_H - ICON_SIZE) // 2

draw.rounded_rectangle(
    [ICON_X, ICON_Y, ICON_X + ICON_SIZE, ICON_Y + ICON_SIZE],
    radius=12, fill=(22, 6, 6, 245), outline=(120, 0, 0, 220), width=1
)

cx = ICON_X + ICON_SIZE // 2
cy = ICON_Y + ICON_SIZE // 2

draw.ellipse([cx - 18, cy - 18, cx + 18, cy + 18], fill=(139, 0, 0, 35))
draw.ellipse([cx - 12, cy - 12, cx + 12, cy + 12], fill=(139, 0, 0, 55))
draw.ellipse([cx - 8, cy - 2, cx + 8, cy + 10], fill=(190, 10, 10, 255))
draw.ellipse([cx - 5, cy - 12, cx + 5, cy - 2], fill=(190, 10, 10, 255))
draw.polygon([(cx + 4, cy - 8), (cx + 13, cy - 7), (cx + 4, cy - 6)], fill=(210, 30, 30, 255))
draw.polygon([(cx - 8, cy + 2), (cx - 19, cy - 5), (cx - 6, cy + 7)], fill=(170, 0, 0, 255))
draw.polygon([(cx + 8, cy + 2), (cx + 17, cy - 3), (cx + 6, cy + 7)], fill=(170, 0, 0, 255))
draw.polygon([(cx - 5, cy + 10), (cx - 9, cy + 19), (cx + 9, cy + 19), (cx + 5, cy + 10)], fill=(170, 0, 0, 255))
draw.ellipse([cx - 2, DOCK_Y + DOCK_H - 9, cx + 2, DOCK_Y + DOCK_H - 5], fill=(180, 0, 0, 230))

# ── Composite and save ────────────────────────────────────────────────────────
final = Image.alpha_composite(bg, overlay)
final = final.convert("RGB")
final.save("/home/ubuntu/RavenOS/bubo_desktop_final.png", quality=96)
print(f"Desktop composited: {final.size}")
