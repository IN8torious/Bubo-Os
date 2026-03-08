"""
render_desktops.py — Render both calm and storm BUBO OS desktop previews
using the new voxel dock module.
"""

from PIL import Image, ImageDraw, ImageFilter, ImageEnhance, ImageFont
from datetime import datetime
from bubo_dock import draw_dock, DOCK_W, DOCK_H
import random, math, sys

random.seed(42)

# ── Load wallpaper ─────────────────────────────────────────────────────────────
wall = Image.open("/home/ubuntu/RavenOS/akatsuki_dawn_wallpaper.png").convert("RGBA")
wall = wall.resize((1920, 1080), Image.LANCZOS)

# ── Shared: contributor taskbar ────────────────────────────────────────────────
def draw_taskbar(img, storm=False):
    W, H = img.size
    BAR_H = 46
    BAR_Y = H - BAR_H
    overlay = Image.new("RGBA", (W, H), (0,0,0,0))
    d = ImageDraw.Draw(overlay)

    bar_color = (8, 4, 18, 215)
    top_line  = (160, 20, 20, 200) if storm else (60, 20, 80, 200)
    d.rectangle([0, BAR_Y, W, H], fill=bar_color)
    d.line([0, BAR_Y, W, BAR_Y], fill=top_line, width=1)

    bar_cy = BAR_Y + BAR_H // 2

    ICON_SLOT_SIZE = 32
    ICON_SLOT_PAD  = 6
    slot_x = 10

    contributors = [
        ('logo_nasa',     (255, 80,  80)),
        ('logo_google_g', (80,  160, 255)),
        ('logo_owm',      (80,  200, 255)),
        ('logo_qt',       (100, 220, 100)),
        ('logo_epic',     (200, 200, 200)),
        ('logo_opera',    (255, 60,  60)),
    ]

    for logo_name, glow_color in contributors:
        sx, sy = slot_x, bar_cy - ICON_SLOT_SIZE // 2
        sw = sh = ICON_SLOT_SIZE

        # Glow halo
        halo = Image.new('RGBA', (W, H), (0,0,0,0))
        hd = ImageDraw.Draw(halo)
        hd.rounded_rectangle([sx-3, sy-3, sx+sw+3, sy+sh+3],
                              radius=8, fill=(*glow_color, 35))
        halo = halo.filter(ImageFilter.GaussianBlur(5))
        overlay.alpha_composite(halo)
        d = ImageDraw.Draw(overlay)

        # Bubble
        d.rounded_rectangle([sx, sy, sx+sw, sy+sh],
                             radius=6, fill=(14, 8, 22, 210),
                             outline=(*glow_color, 120), width=1)

        # Logo
        try:
            logo = Image.open(f'/home/ubuntu/RavenOS/{logo_name}.png').convert('RGBA')
            pad = 4
            logo = logo.resize((sw-pad*2, sh-pad*2), Image.LANCZOS)
            overlay.paste(logo, (sx+pad, sy+pad), logo)
        except:
            d.ellipse([sx+8, sy+8, sx+sw-8, sy+sh-8], fill=(*glow_color, 200))

        d = ImageDraw.Draw(overlay)
        slot_x += sw + ICON_SLOT_PAD

    # Clock — right side, digits only
    now = datetime.now()
    time_str = now.strftime("%H:%M")
    try:
        font_clock = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 15)
    except:
        font_clock = ImageFont.load_default()
    tw = d.textlength(time_str, font=font_clock)
    d.text((W - tw - 14, bar_cy - 9), time_str, fill=(240, 240, 255, 200), font=font_clock)

    # Center pulse dot
    dot_color = (220, 60, 60, 255) if storm else (100, 60, 160, 180)
    d.ellipse([W//2 - 3, bar_cy - 3, W//2 + 3, bar_cy + 3], fill=dot_color)

    img.alpha_composite(overlay)
    return img


# ══════════════════════════════════════════════════════════════════════════════
# CALM DESKTOP
# ══════════════════════════════════════════════════════════════════════════════
calm = wall.copy()

# Vignette
vov = Image.new("RGBA", (1920, 1080), (0,0,0,0))
vd  = ImageDraw.Draw(vov)
for i in range(50):
    alpha = int(70 * (1 - i/50))
    vd.rectangle([i, i, 1920-i, 1080-i], outline=(0,0,0,alpha))
calm = Image.alpha_composite(calm, vov)

calm = draw_taskbar(calm, storm=False)
calm = draw_dock(calm, storm=False)
calm.convert("RGB").save("/home/ubuntu/RavenOS/bubo_desktop_final.png", quality=96)
print("Calm desktop saved.")


# ══════════════════════════════════════════════════════════════════════════════
# STORM DESKTOP
# ══════════════════════════════════════════════════════════════════════════════
storm_bg = wall.copy()
storm_bg = ImageEnhance.Brightness(storm_bg).enhance(0.45)
storm_bg = ImageEnhance.Contrast(storm_bg).enhance(1.3)
storm_bg = ImageEnhance.Color(storm_bg).enhance(0.7)

# Storm clouds
sc = Image.new("RGBA", (1920,1080),(0,0,0,0))
scd = ImageDraw.Draw(sc)
for _ in range(60):
    x = random.randint(-100,1920); y = random.randint(-20,340)
    w = random.randint(200,600);   h = random.randint(60,180)
    scd.ellipse([x,y,x+w,y+h], fill=(8,4,12,random.randint(30,90)))
sc = sc.filter(ImageFilter.GaussianBlur(18))
storm_bg = Image.alpha_composite(storm_bg, sc)

# Rain
rl = Image.new("RGBA",(1920,1080),(0,0,0,0))
rd = ImageDraw.Draw(rl)
for _ in range(1800):
    x=random.randint(0,1920); y=random.randint(0,1080)
    ln=random.randint(8,22); ang=random.uniform(-0.25,-0.15)
    rd.line([x,y,int(x+ln*math.sin(ang)),int(y+ln)],
            fill=(140,165,210,random.randint(60,140)),width=1)
storm_bg = Image.alpha_composite(storm_bg, rl)

# Fractal lightning
def midpoint_lightning(x1,y1,x2,y2,roughness=2.2,depth=7):
    if depth==0: return [(x1,y1),(x2,y2)]
    mx=(x1+x2)/2+random.gauss(0,roughness*abs(x2-x1+y2-y1)*0.04)
    my=(y1+y2)/2+random.gauss(0,roughness*abs(y2-y1)*0.04)
    return midpoint_lightning(x1,y1,mx,my,roughness*0.62,depth-1) + \
           midpoint_lightning(mx,my,x2,y2,roughness*0.62,depth-1)[1:]

def draw_bolt(img,x1,y1,x2,y2,alpha=220,branch_prob=0.38,depth=7):
    pts=midpoint_lightning(x1,y1,x2,y2,depth=depth)
    for width,fill,blur in [
        (18,(160,120,255,max(8,alpha//14)),12),
        (7,(200,200,255,max(18,alpha//6)),4),
        (2,(255,252,220,alpha),0)]:
        layer=Image.new("RGBA",img.size,(0,0,0,0))
        ld=ImageDraw.Draw(layer)
        for i in range(len(pts)-1): ld.line([pts[i],pts[i+1]],fill=fill,width=width)
        if blur: layer=layer.filter(ImageFilter.GaussianBlur(blur))
        img.alpha_composite(layer)
    if depth>3:
        for i in range(2,len(pts)-2,max(1,len(pts)//5)):
            if random.random()<branch_prob:
                bx=pts[i][0]+random.randint(30,100)*random.choice([-1,1])
                by=pts[i][1]+random.randint(60,180)
                draw_bolt(img,int(pts[i][0]),int(pts[i][1]),int(bx),int(by),
                          alpha=alpha//2,branch_prob=branch_prob*0.5,depth=depth-3)

ll=Image.new("RGBA",(1920,1080),(0,0,0,0))
draw_bolt(ll,360,0,310,520,alpha=230)
draw_bolt(ll,1080,0,1160,560,alpha=210)
draw_bolt(ll,1560,0,1500,420,alpha=200)
flash=Image.new("RGBA",(1920,1080),(180,160,255,18))
ll=Image.alpha_composite(ll,flash)
storm_bg=Image.alpha_composite(storm_bg,ll)

# Vignette
vov2=Image.new("RGBA",(1920,1080),(0,0,0,0))
vd2=ImageDraw.Draw(vov2)
for i in range(60):
    vd2.rectangle([i,i,1920-i,1080-i],outline=(0,0,0,int(90*(1-i/60))))
storm_bg=Image.alpha_composite(storm_bg,vov2)

storm_bg = draw_taskbar(storm_bg, storm=True)
storm_bg = draw_dock(storm_bg, storm=True)
storm_bg.convert("RGB").save("/home/ubuntu/RavenOS/bubo_desktop_storm.png", quality=96)
print("Storm desktop saved.")
