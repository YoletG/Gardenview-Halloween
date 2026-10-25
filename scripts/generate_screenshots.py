import os
import math
import colorsys
from PIL import Image, ImageDraw, ImageFont

# Canvas dimensions (2x scaling for ultra-crisp display in GitHub markdown: 480x640)
SCALE = 2
W = 240 * SCALE
H = 320 * SCALE

def S(val):
    return int(val * SCALE)

def rgb565_to_rgb(val):
    r = ((val >> 11) & 0x1F) * 255 // 31
    g = ((val >> 5) & 0x3F) * 255 // 63
    b = (val & 0x1F) * 255 // 31
    return (r, g, b)

# Colors
C_BLACK = (10, 10, 12)
C_WHITE = (255, 255, 255)
C_CYAN = (0, 240, 255)
C_GREEN = (34, 197, 94)
C_ORANGE = (249, 115, 22)
C_YELLOW = (234, 179, 8)
C_RED = (239, 68, 68)
C_PURPLE = (168, 85, 247)
C_ICHOR = (216, 31, 240)
C_BLUE = (59, 130, 246)
C_NAVY = (12, 18, 30)
C_CARD_BG = (24, 28, 36)
C_TOOLBAR = (18, 22, 30)
C_GRAY_TEXT = (156, 163, 175)
C_MUTED = (75, 85, 99)

FONT_REG = "/System/Library/Fonts/Helvetica.ttc"
FONT_MONO = "/System/Library/Fonts/Menlo.ttc"

def get_font(size, mono=False):
    path = FONT_MONO if mono else FONT_REG
    try:
        return ImageFont.truetype(path, S(size))
    except Exception:
        return ImageFont.load_default()

font_hero = get_font(15, mono=False)
font_title = get_font(12, mono=False)
font_large = get_font(14, mono=False)
font_bold = get_font(11, mono=False)
font_sm = get_font(9, mono=False)
font_xs = get_font(7, mono=False)
font_mono = get_font(9, mono=True)
font_mono_lg = get_font(11, mono=True)

def draw_status_bar(draw, title="LILYGO T-HMI", is_home=True):
    # Top bar background
    draw.rectangle([0, 0, W, S(25)], fill=C_NAVY)
    draw.line([0, S(25), W, S(25)], fill=(40, 50, 70), width=SCALE)
    
    if is_home:
        draw.text((S(8), S(5)), title, fill=C_CYAN, font=font_bold)
    else:
        # Back button
        draw.rounded_rectangle([S(4), S(2), S(58), S(23)], radius=S(4), fill=(30, 40, 55), outline=C_CYAN, width=1)
        draw.text((S(10), S(5)), "< HOME", fill=C_CYAN, font=font_sm)
        # Title
        draw.text((S(95), S(5)), title, fill=C_WHITE, font=font_bold)
    
    # Battery info
    draw.text((S(175), S(5)), "USB [=]", fill=C_GREEN, font=font_mono)

# ==============================================================================
# 1. APPLICATION LAUNCHER
# ==============================================================================
def render_launcher():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "LILYGO T-HMI", is_home=True)
    
    # Header
    draw.rectangle([0, S(26), W, S(62)], fill=(20, 24, 32))
    draw.line([0, S(62), W, S(62)], fill=(45, 55, 75), width=1)
    draw.text((S(44), S(31)), "APPLICATION LAUNCHER", fill=C_WHITE, font=font_bold)
    draw.text((S(48), S(46)), "Tap any application to launch", fill=C_GRAY_TEXT, font=font_xs)
    
    apps = [
        ("#1", "Draw Pad", "Solid brush & anti-aliased UI", C_GREEN),
        ("#2", "Arcade", "Gardenview Halloween & Breakout", C_ORANGE),
        ("#3", "SysMonitor", "System health, sensors & Wi-Fi", C_BLUE)
    ]
    
    card_y = 68
    card_h = 68
    spacing = 10
    
    for i, (num, name, desc, color) in enumerate(apps):
        y = S(card_y + i * (card_h + spacing))
        h = S(card_h)
        # Card body
        draw.rounded_rectangle([S(8), y, S(232), y + h], radius=S(8), fill=C_CARD_BG, outline=color, width=SCALE)
        # Icon accent box
        draw.rounded_rectangle([S(14), y + S(8), S(56), y + S(56)], radius=S(6), fill=color)
        draw.text((S(23), y + S(22)), num, fill=(0, 0, 0), font=font_large)
        # Text
        draw.text((S(66), y + S(12)), name, fill=C_WHITE, font=font_large)
        draw.text((S(66), y + S(38)), desc, fill=C_GRAY_TEXT, font=font_xs)
        # Chevron
        draw.text((S(214), y + S(24)), ">", fill=color, font=font_large)
        
    # Footer
    draw.rectangle([0, S(304), W, H], fill=C_NAVY)
    draw.text((S(22), S(307)), "Press BOOT button anytime to return Home", fill=(100, 116, 139), font=font_xs)
    return img

# ==============================================================================
# 2. DRAWING PAD
# ==============================================================================
def render_drawpad():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Draw Pad", is_home=False)
    
    # Top Toolbar
    draw.rectangle([0, S(26), W, S(50)], fill=C_TOOLBAR)
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    
    # PEN button (active)
    draw.rounded_rectangle([S(4), S(29), S(44), S(47)], radius=S(4), fill=C_GREEN)
    draw.text((S(14), S(32)), "PEN", fill=(0, 0, 0), font=font_sm)
    
    # ERASE button
    draw.rounded_rectangle([S(48), S(29), S(96), S(47)], radius=S(4), fill=(30, 36, 48), outline=(60, 70, 90), width=1)
    draw.text((S(54), S(32)), "ERASE", fill=C_WHITE, font=font_xs)
    
    # Size badge
    draw.rounded_rectangle([S(100), S(29), S(144), S(47)], radius=S(4), fill=(20, 24, 32), outline=C_CYAN, width=1)
    draw.text((S(108), S(32)), "14px", fill=C_CYAN, font=font_sm)
    
    # [-] and [+]
    draw.rounded_rectangle([S(148), S(29), S(172), S(47)], radius=S(4), fill=(30, 36, 48), outline=C_WHITE, width=1)
    draw.text((S(157), S(31)), "-", fill=C_WHITE, font=font_sm)
    draw.rounded_rectangle([S(176), S(29), S(200), S(47)], radius=S(4), fill=(30, 36, 48), outline=C_WHITE, width=1)
    draw.text((S(184), S(31)), "+", fill=C_WHITE, font=font_sm)
    
    # CLR
    draw.rounded_rectangle([S(204), S(29), S(236), S(47)], radius=S(4), fill=(40, 20, 20), outline=C_RED, width=1)
    draw.text((S(210), S(32)), "CLR", fill=C_RED, font=font_xs)
    
    # Stylized drawing
    sun_cx, sun_cy = S(120), S(140)
    draw.ellipse([sun_cx - S(32), sun_cy - S(32), sun_cx + S(32), sun_cy + S(32)], fill=C_YELLOW)
    
    draw.line([(S(10), S(240)), (S(70), S(160)), (S(130), S(220))], fill=C_CYAN, width=S(6), joint="curve")
    draw.line([(S(130), S(220)), (S(180), S(150)), (S(230), S(240))], fill=(168, 85, 247), width=S(6), joint="curve")
    
    ribbon = [
        (S(30), S(260)), (S(60), S(250)), (S(100), S(270)), 
        (S(150), S(255)), (S(190), S(270)), (S(220), S(260))
    ]
    draw.line(ribbon, fill=C_GREEN, width=S(7), joint="curve")
    draw.text((S(28), S(80)), "LilyGO T-HMI", fill=(244, 114, 182), font=font_large)
    draw.text((S(28), S(102)), "Multi-App System", fill=C_CYAN, font=font_bold)
    
    # Bottom Toolbar
    draw.rectangle([0, S(289), W, H], fill=C_TOOLBAR)
    draw.line([0, S(289), W, S(289)], fill=(45, 55, 75), width=1)
    
    # Swatches
    draw.ellipse([S(6), S(294), S(30), S(318)], fill=C_GREEN, outline=C_WHITE, width=SCALE)
    pal = [C_WHITE, C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN]
    for i, col in enumerate(pal):
        px = S(38 + i * 33)
        draw.rounded_rectangle([px, S(297), px + S(28), S(315)], radius=S(4), fill=col, outline=(col == C_GREEN and C_WHITE or (70, 80, 100)), width=1)
        
    return img

# ==============================================================================
# 3. COLOR PICKER MODAL
# ==============================================================================
def render_color_picker():
    img = render_drawpad()
    draw = ImageDraw.Draw(img)
    
    modal_bg = (18, 22, 30)
    draw.rectangle([0, S(26), W, H], fill=modal_bg)
    draw.rectangle([0, S(26), W, H], outline=C_CYAN, width=1)
    
    draw.text((S(48), S(32)), "COLOR PALETTE (HSV)", fill=C_WHITE, font=font_bold)
    draw.text((S(220), S(30)), "X", fill=C_RED, font=font_bold)
    
    # Hue slider
    hue_y = S(56)
    hue_h = S(20)
    for x in range(S(16), S(224)):
        hue = (x - S(16)) / float(S(208))
        r, g, b = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
        draw.line([x, hue_y, x, hue_y + hue_h], fill=(int(r*255), int(g*255), int(b*255)))
    draw.rectangle([S(16), hue_y, S(224), hue_y + hue_h], outline=C_WHITE, width=1)
    thumb_x = S(16 + int(0.33 * 208))
    draw.rectangle([thumb_x - S(3), hue_y - S(2), thumb_x + S(3), hue_y + hue_h + S(2)], outline=C_WHITE, fill=(0, 0, 0), width=1)
    
    # Sat/Val box
    box_x = S(16)
    box_y = S(84)
    box_w = S(208)
    box_h = S(100)
    for y_step in range(0, box_h, SCALE):
        val = 1.0 - (y_step / float(box_h))
        for x_step in range(0, box_w, SCALE):
            sat = x_step / float(box_w)
            r, g, b = colorsys.hsv_to_rgb(0.33, sat, val)
            draw.rectangle([box_x + x_step, box_y + y_step, box_x + x_step + SCALE, box_y + y_step + SCALE], fill=(int(r*255), int(g*255), int(b*255)))
    draw.rectangle([box_x, box_y, box_x + box_w, box_y + box_h], outline=C_WHITE, width=1)
    draw.ellipse([box_x + S(160) - S(5), box_y + S(20) - S(5), box_x + S(160) + S(5), box_y + S(20) + S(5)], outline=C_WHITE, width=SCALE)
    
    # Preview Swatch
    draw.rounded_rectangle([S(10), S(194), S(90), S(234)], radius=S(5), fill=C_GREEN, outline=C_WHITE, width=1)
    draw.text((S(22), S(206)), "#22C55E", fill=(0, 0, 0), font=font_mono)
    
    # Presets
    presets = [C_WHITE, C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN, C_BLUE, (236, 72, 153)]
    for i, p_col in enumerate(presets):
        px = S(100 + (i % 4) * 32)
        py = S(194 if i < 4 else 216)
        draw.rounded_rectangle([px, py, px + S(28), py + S(18)], radius=S(3), fill=p_col, outline=(p_col == C_GREEN and C_WHITE or (70, 80, 100)), width=1)
        
    # Apply button
    draw.rounded_rectangle([S(16), S(248), S(224), S(286)], radius=S(6), fill=C_GREEN, outline=C_WHITE, width=1)
    draw.text((S(70), S(258)), "APPLY COLOR", fill=(0, 0, 0), font=font_bold)
    return img

# ==============================================================================
# 4. GAMES ARCADE SELECTION MENU
# ==============================================================================
def render_arcade_menu():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Arcade", is_home=False)
    
    # Header Sub-bar
    draw.rectangle([0, S(26), W, S(62)], fill=(16, 20, 32))
    draw.line([0, S(62), W, S(62)], fill=(45, 55, 75), width=1)
    draw.text((S(65), S(31)), "GAMES ARCADE", fill=C_WHITE, font=font_bold)
    draw.text((S(48), S(46)), "Choose an arcade game to play", fill=C_GRAY_TEXT, font=font_xs)
    
    # Card 1: Gardenview Halloween (Dandy's World)
    card1_y = S(72)
    card_h = S(100)
    draw.rounded_rectangle([S(8), card1_y, S(232), card1_y + card_h], radius=S(8), fill=C_CARD_BG, outline=C_ORANGE, width=SCALE)
    
    # Pumpkin Accent Box
    draw.rounded_rectangle([S(14), card1_y + S(12), S(68), card1_y + S(88)], radius=S(6), fill=(40, 25, 15), outline=C_ORANGE, width=1)
    
    # Mini Gourdy Illustration in Accent Box
    px = S(41)
    py = card1_y + S(50)
    # Lobes
    draw.ellipse([px - S(14), py - S(10), px - S(1), py + S(10)], fill=(249, 115, 22))
    draw.ellipse([px + S(1), py - S(10), px + S(14), py + S(10)], fill=(249, 115, 22))
    draw.ellipse([px - S(9), py - S(11), px + S(9), py + S(11)], fill=(253, 186, 116))
    # Stem & Leaf
    draw.rectangle([px - S(2), py - S(16), px + S(2), py - S(10)], fill=(105, 154, 33))
    draw.ellipse([px + S(2), py - S(16), px + S(8), py - S(12)], fill=(34, 197, 94))
    # Eyes & Smile
    draw.rectangle([px - S(6), py - S(4), px - S(2), py], fill=(10, 10, 10))
    draw.rectangle([px + S(2), py - S(4), px + S(6), py], fill=(10, 10, 10))
    draw.line([(px - S(5), py + S(5)), (px + S(5), py + S(5))], fill=(10, 10, 10), width=SCALE)
    draw.point((px - S(2), py + S(5)), fill=C_WHITE)
    draw.point((px + S(2), py + S(5)), fill=C_WHITE)
    
    # Text
    draw.text((S(76), card1_y + S(12)), "Gardenview", fill=C_ORANGE, font=font_large)
    draw.text((S(76), card1_y + S(34)), "Halloween", fill=C_YELLOW, font=font_large)
    draw.text((S(76), card1_y + S(58)), "Dandy's World Survival", fill=(216, 120, 255), font=font_sm)
    draw.text((S(76), card1_y + S(76)), "Play as Gourdy! Collect Ichor,", fill=C_GRAY_TEXT, font=font_xs)
    draw.text((S(76), card1_y + S(87)), "extract & escape Twisteds!", fill=C_GRAY_TEXT, font=font_xs)
    draw.text((S(214), card1_y + S(40)), ">", fill=C_ORANGE, font=font_large)
    
    # Card 2: Breakout
    card2_y = S(186)
    draw.rounded_rectangle([S(8), card2_y, S(232), card2_y + card_h], radius=S(8), fill=C_CARD_BG, outline=C_CYAN, width=SCALE)
    
    # Breakout Accent Box
    draw.rounded_rectangle([S(14), card2_y + S(12), S(68), card2_y + S(88)], radius=S(6), fill=(10, 30, 45), outline=C_CYAN, width=1)
    
    # Mini Breakout illustration
    bx = S(41)
    by = card2_y + S(50)
    draw.rounded_rectangle([bx - S(18), by - S(24), bx - S(3), by - S(16)], radius=S(2), fill=C_RED)
    draw.rounded_rectangle([bx + S(3), by - S(24), bx + S(18), by - S(16)], radius=S(2), fill=C_GREEN)
    draw.rounded_rectangle([bx - S(18), by - S(13), bx - S(3), by - S(5)], radius=S(2), fill=C_ORANGE)
    draw.rounded_rectangle([bx + S(3), by - S(13), bx + S(18), by - S(5)], radius=S(2), fill=C_CYAN)
    # Ball
    draw.ellipse([bx + S(2) - S(3), by + S(5) - S(3), bx + S(2) + S(3), by + S(5) + S(3)], fill=C_YELLOW)
    # Paddle
    draw.rounded_rectangle([bx - S(16), by + S(20), bx + S(16), by + S(26)], radius=S(3), fill=C_CYAN)
    
    # Text
    draw.text((S(76), card2_y + S(14)), "Breakout", fill=C_CYAN, font=font_large)
    draw.text((S(76), card2_y + S(40)), "Retro Brick Breaker", fill=C_WHITE, font=font_sm)
    draw.text((S(76), card2_y + S(60)), "Paddle physics, bouncing ball", fill=C_GRAY_TEXT, font=font_xs)
    draw.text((S(76), card2_y + S(74)), "and high score records", fill=C_GRAY_TEXT, font=font_xs)
    draw.text((S(214), card2_y + S(40)), ">", fill=C_CYAN, font=font_large)
    
    # Footer
    draw.rectangle([0, S(304), W, H], fill=C_NAVY)
    draw.text((S(20), S(307)), "Choose game to begin or < HOME to exit", fill=(100, 116, 139), font=font_xs)
    return img

# ==============================================================================
# 5. GARDENVIEW HALLOWEEN (DANDY'S WORLD SURVIVAL)
# ==============================================================================
def render_gourdy():
    # Arena background: Gardenview slate floor (8, 16, 22)
    img = Image.new("RGB", (W, H), (8, 16, 22))
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Gardenview", is_home=False)
    
    # Header bar
    draw.rectangle([0, S(26), W, S(50)], fill=(16, 20, 34))
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    
    # < ARC button
    draw.rounded_rectangle([S(4), S(28), S(46), S(48)], radius=S(3), fill=(30, 40, 60), outline=C_CYAN, width=1)
    draw.text((S(8), S(32)), "< ARC", fill=C_CYAN, font=font_xs)
    
    # Floor badge
    draw.text((S(52), S(31)), "F:1", fill=C_YELLOW, font=font_bold)
    
    # Ichor wallet
    draw.text((S(76), S(31)), "ICHOR:14", fill=C_ICHOR, font=font_bold)
    
    # Score
    draw.text((S(132), S(31)), "00650", fill=C_CYAN, font=font_xs)
    
    # Lives (Jack-o'-lantern hearts)
    for i in range(3):
        hx = S(184 + i * 14)
        draw.ellipse([hx, S(33), hx + S(9), S(42)], fill=C_ORANGE, outline=C_YELLOW, width=1)
        draw.point((hx + S(4), S(31)), fill=(34, 197, 94))
        
    # Arena Border
    draw.rectangle([S(4), S(52), S(236), S(282)], outline=(35, 45, 65), width=1)
    
    # Elevator Door (Top Center)
    elev_x = S(120)
    elev_y = S(56)
    elev_w = S(46)
    elev_h = S(14)
    draw.rounded_rectangle([elev_x - elev_w//2, elev_y, elev_x + elev_w//2, elev_y + elev_h], radius=S(3), fill=(40, 45, 55), outline=C_YELLOW, width=1)
    draw.text((elev_x - S(19), elev_y + S(2)), "ELEVATOR", fill=C_YELLOW, font=font_xs)
    
    # Event Banner
    draw.rounded_rectangle([S(38), S(73), S(202), S(93)], radius=S(4), fill=(42, 21, 10), outline=C_YELLOW, width=1)
    draw.text((S(44), S(77)), "SKILL CHECK: GREAT HIT!", fill=C_WHITE, font=font_sm)
    
    # Pillars / Obstacles (Gardenview Room architecture)
    obstacles = [
        (S(34), S(105), S(24), S(24)),
        (S(182), S(105), S(24), S(24)),
        (S(48), S(195), S(28), S(20)),
        (S(164), S(195), S(28), S(20))
    ]
    for ox, oy, ow, oh in obstacles:
        draw.rounded_rectangle([ox, oy, ox + ow, oy + oh], radius=S(4), fill=(30, 36, 48), outline=(60, 75, 100), width=SCALE)
        draw.rectangle([ox + S(4), oy + S(4), ox + ow - S(4), oy + oh - S(4)], fill=(20, 24, 32))
        
    # Halloween Trick-or-Treat Buckets
    # Bucket 1: Unopened (top-left)
    b1x, b1y = S(18), S(75)
    draw.ellipse([b1x - S(8), b1y - S(8), b1x + S(8), b1y + S(8)], fill=C_ORANGE, outline=C_YELLOW, width=1)
    draw.arc([b1x - S(7), b1y - S(11), b1x + S(7), b1y - S(3)], 0, 180, fill=C_BLACK, width=1)
    draw.text((b1x - S(10), b1y + S(9)), "TREAT", fill=C_YELLOW, font=font_xs)
    
    # Bucket 2: Empty (bottom-right)
    b2x, b2y = S(212), S(230)
    draw.ellipse([b2x - S(7), b2y - S(7), b2x + S(7), b2y + S(7)], outline=(99, 50, 44), width=1)
    draw.text((b2x - S(10), b2y + S(9)), "EMPTY", fill=(99, 50, 44), font=font_xs)
    
    # Machine 1 (Left Generator): Extracting with Skill Check Dial
    m1x, m1y = S(75), S(145)
    draw.rounded_rectangle([m1x - S(16), m1y - S(14), m1x + S(16), m1y + S(14)], radius=S(4), fill=(24, 30, 42), outline=C_CYAN, width=SCALE)
    draw.rectangle([m1x - S(12), m1y - S(10), m1x + S(12), m1y + S(3)], fill=(5, 20, 35))
    draw.text((m1x - S(10), m1y - S(8)), "M1: 85%", fill=C_CYAN, font=font_xs)
    draw.ellipse([m1x - S(3), m1y + S(7), m1x + S(3), m1y + S(13)], fill=C_YELLOW)
    
    # Circular Skill Check QTE Dial over Machine 1
    sc_r = S(20)
    draw.ellipse([m1x - sc_r, m1y - sc_r, m1x + sc_r, m1y + sc_r], outline=C_WHITE, width=SCALE)
    # Hit Zone (Green Arc)
    draw.arc([m1x - sc_r, m1y - sc_r, m1x + sc_r, m1y + sc_r], 40, 95, fill=C_GREEN, width=S(3))
    # Needle
    draw.line([(m1x, m1y), (m1x + S(14), m1y + S(14))], fill=C_YELLOW, width=SCALE)
    draw.text((m1x - S(10), m1y - S(25)), "TAP!", fill=C_YELLOW, font=font_xs)
    
    # Machine 2 (Right Generator): 100% Completed!
    m2x, m2y = S(165), S(145)
    draw.rounded_rectangle([m2x - S(16), m2y - S(14), m2x + S(16), m2y + S(14)], radius=S(4), fill=(24, 30, 42), outline=C_GREEN, width=SCALE)
    draw.rectangle([m2x - S(12), m2y - S(10), m2x + S(12), m2y + S(3)], fill=(5, 35, 15))
    draw.text((m2x - S(10), m2y - S(8)), "M2: 100%", fill=C_GREEN, font=font_xs)
    draw.ellipse([m2x - S(3), m2y + S(7), m2x + S(3), m2y + S(13)], fill=C_GREEN)
    
    # Ichor Drops (Glowing Purple Inky essence)
    ichor_pts = [(S(32), S(160)), (S(208), S(160)), (S(120), S(180))]
    for ix, iy in ichor_pts:
        draw.ellipse([ix - S(8), iy - S(8), ix + S(8), iy + S(8)], fill=(60, 15, 75))
        draw.ellipse([ix - S(5), iy - S(5), ix + S(5), iy + S(5)], fill=C_ICHOR)
        draw.ellipse([ix - S(2), iy - S(3), ix + S(2), iy + S(1)], fill=C_WHITE)
        
    # Twisted Dandy (Inky Monster with Corrupted Pumpkin Petals)
    tx, ty = S(216), S(82)
    draw.text((tx - S(3), ty - S(24)), "!", fill=C_RED, font=font_bold)
    # Petals
    for p in range(6):
        rad = p * 1.047
        px = tx + int(math.cos(rad) * S(12))
        py = ty + int(math.sin(rad) * S(12))
        draw.ellipse([px - S(4), py - S(4), px + S(4), py + S(4)], fill=C_ORANGE)
    # Head & Red Eyes
    draw.ellipse([tx - S(10), ty - S(10), tx + S(10), ty + S(10)], fill=(15, 15, 20), outline=C_BLACK, width=1)
    draw.ellipse([tx - S(6), ty - S(3), tx - S(1), ty + S(2)], fill=C_RED)
    draw.ellipse([tx + S(1), ty - S(3), tx + S(6), ty + S(2)], fill=C_RED)
    
    # Gourdy Holloway (Player)
    gx, gy = S(85), S(185)
    # Vine limbs
    draw.line([(gx - S(8), gy + S(6)), (gx - S(12), gy + S(16))], fill=(105, 154, 33), width=SCALE)
    draw.line([(gx + S(8), gy + S(6)), (gx + S(12), gy + S(16))], fill=(105, 154, 33), width=SCALE)
    # Multi-lobed Pumpkin Body
    draw.ellipse([gx - S(13), gy - S(10), gx, gy + S(10)], fill=(249, 115, 22))
    draw.ellipse([gx, gy - S(10), gx + S(13), gy + S(10)], fill=(249, 115, 22))
    draw.ellipse([gx - S(8), gy - S(11), gx + S(8), gy + S(11)], fill=(251, 146, 60))
    # Stem & Leaf
    draw.rectangle([gx - S(2), gy - S(17), gx + S(2), gy - S(11)], fill=(105, 154, 33))
    draw.ellipse([gx + S(2), gy - S(17), gx + S(9), gy - S(13)], fill=(34, 197, 94))
    # Eyes & Smile
    draw.rectangle([gx - S(6), gy - S(3), gx - S(1), gy + S(2)], fill=(15, 15, 15))
    draw.rectangle([gx + S(2), gy - S(3), gx + S(7), gy + S(2)], fill=(15, 15, 15))
    draw.point((gx - S(3), gy - S(1)), fill=C_WHITE)
    draw.point((gx + S(5), gy - S(1)), fill=C_WHITE)
    draw.line([(gx - S(6), gy + S(5)), (gx + S(6), gy + S(5))], fill=(15, 15, 15), width=SCALE)
    
    # Bottom Controls Toolbar (y: 284..320)
    draw.rectangle([0, S(284), W, H], fill=C_TOOLBAR)
    draw.line([0, S(284), W, S(284)], fill=(45, 55, 75), width=1)
    
    # 1. Flashlight Stun Button (Left)
    draw.ellipse([S(10), S(286), S(42), S(318)], fill=C_YELLOW, outline=C_WHITE, width=1)
    draw.text((S(14), S(298)), "STUN(2)", fill=(0, 0, 0), font=font_xs)
    
    # 2. Stamina Meter (Center)
    draw.text((S(100), S(288)), "STAMINA", fill=C_GRAY_TEXT, font=font_xs)
    draw.rectangle([S(58), S(301), S(182), S(309)], outline=C_WHITE, fill=(10, 15, 20), width=1)
    draw.rectangle([S(59), S(302), S(160), S(308)], fill=C_GREEN)
    
    # 3. Sugar Rush Candy Button (Right)
    draw.ellipse([S(198), S(286), S(230), S(318)], fill=C_ORANGE, outline=C_WHITE, width=1)
    draw.text((S(201), S(298)), "RUSH(1)", fill=C_WHITE, font=font_xs)
    return img

# ==============================================================================
# 5b. DANDY'S HALLOWEEN ELEVATOR SHOP
# ==============================================================================
def render_dandy_shop():
    img = Image.new("RGB", (W, H), (12, 16, 25))
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Dandy's Shop", is_home=False)
    
    # Header bar
    draw.rectangle([0, S(26), W, S(50)], fill=(16, 20, 34))
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    draw.text((S(45), S(31)), "DANDY'S ELEVATOR SHOP", fill=C_YELLOW, font=font_bold)
    
    # Dandy Character with Halloween Witch Hat
    dx, dy = S(120), S(85)
    for i in range(6):
        a = i * 1.047
        px = dx + int(math.cos(a) * S(16))
        py = dy + int(math.sin(a) * S(16))
        draw.ellipse([px - S(8), py - S(8), px + S(8), py + S(8)], fill=C_YELLOW)
    draw.ellipse([dx - S(12), dy - S(12), dx + S(12), dy + S(12)], fill=C_WHITE)
    draw.point((dx - S(4), dy - S(2)), fill=C_BLACK)
    draw.point((dx + S(4), dy - S(2)), fill=C_BLACK)
    draw.line([(dx - S(5), dy + S(4)), (dx + S(5), dy + S(4))], fill=C_BLACK, width=1)
    
    # Witch / Top Hat
    draw.rectangle([dx - S(14), dy - S(18), dx + S(14), dy - S(14)], fill=(45, 20, 60))
    draw.rectangle([dx - S(8), dy - S(28), dx + S(8), dy - S(18)], fill=(45, 20, 60))
    draw.line([(dx - S(8), dy - S(19)), (dx + S(8), dy - S(19))], fill=C_ORANGE, width=1)
    
    # Speech Bubble
    draw.text((S(52), S(116)), "\"Happy Halloween, Gourdy!\"", fill=C_WHITE, font=font_sm)
    draw.text((S(76), S(132)), "Your Ichor: 14", fill=C_ICHOR, font=font_bold)
    
    # Shop Items
    items = [
        ("1. CHOCOLATE BAR (+1 Heart)", "Cost: 8 Ichor [BUY]", C_GREEN),
        ("2. CANDY CORN (+1 Speed Boost)", "Cost: 5 Ichor [BUY]", C_ORANGE),
        ("3. FLASHLIGHT BATTERY (+2 Stun)", "Cost: 6 Ichor [BUY]", C_YELLOW)
    ]
    for i, (title, cost, outline_col) in enumerate(items):
        iy = S(150 + i * 40)
        draw.rounded_rectangle([S(14), iy, S(226), iy + S(34)], radius=S(4), fill=C_CARD_BG, outline=outline_col, width=1)
        draw.text((S(22), iy + S(4)), title, fill=C_WHITE, font=font_sm)
        draw.text((S(22), iy + S(18)), cost, fill=C_ICHOR, font=font_xs)
        
    # Continue Button
    draw.rounded_rectangle([S(25), S(274), S(215), S(310)], radius=S(6), fill=C_CYAN, outline=C_WHITE, width=1)
    draw.text((S(60), S(285)), "ENTER FLOOR 2 >", fill=(0, 0, 0), font=font_large)
    return img

# ==============================================================================
# 6. BREAKOUT GAME
# ==============================================================================
def render_breakout():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Breakout", is_home=False)
    
    # Score Header with < ARC button
    draw.rectangle([0, S(26), W, S(50)], fill=C_TOOLBAR)
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    
    # < ARC button
    draw.rounded_rectangle([S(4), S(28), S(46), S(48)], radius=S(3), fill=(30, 40, 60), outline=C_CYAN, width=1)
    draw.text((S(8), S(32)), "< ARC", fill=C_CYAN, font=font_xs)
    
    draw.text((S(52), S(31)), "Score: 180", fill=C_WHITE, font=font_bold)
    draw.text((S(122), S(31)), "Hi: 420", fill=C_WHITE, font=font_bold)
    
    # Lives (hearts)
    for i in range(3):
        hx = S(190 + i * 14)
        draw.ellipse([hx, S(33), hx + S(9), S(42)], fill=C_RED, outline=C_WHITE, width=1)
        
    # Bricks: 4 rows x 6 columns
    rows = [C_RED, C_ORANGE, C_GREEN, C_CYAN]
    brick_w = 35
    brick_h = 12
    gap = 3
    start_x = 7
    start_y = 56
    
    # Some bricks broken for realistic live game view
    broken = {(0, 2), (0, 3), (1, 2), (1, 4), (2, 1), (3, 3)}
    
    for r, col in enumerate(rows):
        for c in range(6):
            if (r, c) in broken:
                continue
            bx = S(start_x + c * (brick_w + gap))
            by = S(start_y + r * (brick_h + gap))
            draw.rounded_rectangle([bx, by, bx + S(brick_w), by + S(brick_h)], radius=S(2), fill=col, outline=C_WHITE, width=1)
            
    # Ball
    ball_x, ball_y = S(135), S(215)
    draw.ellipse([ball_x - S(4), ball_y - S(4), ball_x + S(4), ball_y + S(4)], fill=C_YELLOW)
    
    # Paddle
    pad_x = S(95)
    pad_y = S(285)
    draw.rounded_rectangle([pad_x, pad_y, pad_x + S(50), pad_y + S(9)], radius=S(3), fill=C_CYAN, outline=C_WHITE, width=1)
    return img

# ==============================================================================
# 7. SYSTEM MONITOR (SoC TAB)
# ==============================================================================
def render_sysmonitor():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "SysMonitor", is_home=False)
    
    # Tab Bar
    draw.rectangle([0, S(26), W, S(50)], fill=C_TOOLBAR)
    tabs = ["SoC", "Power", "Wi-Fi", "Sensors"]
    tab_w = S(60)
    for i, t in enumerate(tabs):
        tx = i * tab_w
        if i == 0:  # Active
            draw.rectangle([tx, S(26), tx + tab_w, S(50)], fill=(15, 60, 110))
            draw.line([tx, S(49), tx + tab_w, S(49)], fill=C_CYAN, width=SCALE)
            draw.text((tx + S(18), S(32)), t, fill=C_WHITE, font=font_bold)
        else:
            draw.text((tx + S(12 if len(t) > 3 else 18), S(32)), t, fill=C_GRAY_TEXT, font=font_sm)
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    
    # Card 1: ESP32-S3 Dual-Core
    draw.rounded_rectangle([S(6), S(56), S(234), S(132)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(56), S(234), S(74)], radius=S(4), fill=(30, 50, 80))
    draw.text((S(12), S(58)), "ESP32-S3 Dual-Core", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(78)), "Clock Speed: 240 MHz", fill=C_WHITE, font=font_sm)
    draw.text((S(14), S(94)), "Internal Temp: 41.2 C (106.2 F)", fill=(200, 210, 230), font=font_sm)
    draw.text((S(14), S(110)), "Uptime: 0h 14m 32s", fill=(200, 210, 230), font=font_sm)
    
    # Card 2: Memory & Storage
    draw.rounded_rectangle([S(6), S(138), S(234), S(230)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(138), S(234), S(156)], radius=S(4), fill=(40, 30, 80))
    draw.text((S(12), S(140)), "Memory & Storage", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(160)), "SRAM Heap: 45 / 320 KB", fill=(200, 210, 230), font=font_sm)
    # Progress bar
    draw.rectangle([S(14), S(178), S(214), S(184)], outline=(60, 80, 110), fill=(20, 25, 35), width=1)
    draw.rectangle([S(15), S(179), S(50), S(183)], fill=C_GREEN)
    draw.text((S(14), S(188)), "PSRAM: 1 / 8 MB", fill=(200, 210, 230), font=font_sm)
    draw.text((S(14), S(206)), "Flash Memory: 16 MB QIO", fill=(200, 210, 230), font=font_sm)
    
    # Card 3: Hardware Model
    draw.rounded_rectangle([S(6), S(236), S(234), S(310)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(236), S(234), S(254)], radius=S(4), fill=(25, 45, 65))
    draw.text((S(12), S(238)), "Hardware Model", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(258)), "LilyGO T-HMI ESP32-S3", fill=C_WHITE, font=font_sm)
    draw.text((S(14), S(274)), "Display: 2.8\" ST7789 i80 Parallel", fill=(200, 210, 230), font=font_xs)
    draw.text((S(14), S(290)), "Touch: XPT2046 12-bit SPI", fill=(200, 210, 230), font=font_xs)
    return img

# ==============================================================================
# 8. SYSTEM MONITOR (SENSORS TAB)
# ==============================================================================
def render_sensors():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "SysMonitor", is_home=False)
    
    # Tab Bar
    draw.rectangle([0, S(26), W, S(50)], fill=C_TOOLBAR)
    tabs = ["SoC", "Power", "Wi-Fi", "Sensors"]
    tab_w = S(60)
    for i, t in enumerate(tabs):
        tx = i * tab_w
        if i == 3:  # Active
            draw.rectangle([tx, S(26), tx + tab_w, S(50)], fill=(15, 60, 110))
            draw.line([tx, S(49), tx + tab_w, S(49)], fill=C_CYAN, width=SCALE)
            draw.text((tx + S(8), S(32)), t, fill=C_WHITE, font=font_bold)
        else:
            draw.text((tx + S(12 if len(t) > 3 else 18), S(32)), t, fill=C_GRAY_TEXT, font=font_sm)
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    
    # Card 1: Magnetic Reed Switch
    draw.rounded_rectangle([S(6), S(56), S(234), S(132)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(56), S(234), S(74)], radius=S(4), fill=(30, 50, 80))
    draw.text((S(12), S(58)), "Magnetic Reed Switch", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(78)), "Pin: GPIO 21 (Internal)", fill=C_GRAY_TEXT, font=font_xs)
    # LED indicator
    draw.ellipse([S(18), S(100), S(30), S(112)], fill=C_GREEN, outline=C_WHITE, width=1)
    draw.text((S(38), S(101)), "MAGNET DETECTED / CLOSED", fill=C_GREEN, font=font_sm)
    
    # Card 2: Hardware BOOT Key
    draw.rounded_rectangle([S(6), S(138), S(234), S(200)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(138), S(234), S(156)], radius=S(4), fill=(40, 45, 60))
    draw.text((S(12), S(140)), "Hardware BOOT Key", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(158)), "Pin: GPIO 0 (Boot Key)", fill=C_GRAY_TEXT, font=font_xs)
    # LED indicator
    draw.ellipse([S(18), S(174), S(30), S(186)], fill=(80, 85, 95), outline=(120, 125, 135), width=1)
    draw.text((S(38), S(175)), "Key Released", fill=C_GRAY_TEXT, font=font_sm)
    
    # Card 3: Touchscreen & Calibration
    draw.rounded_rectangle([S(6), S(206), S(234), S(312)], radius=S(6), fill=C_CARD_BG, outline=(50, 70, 100), width=1)
    draw.rounded_rectangle([S(6), S(206), S(234), S(224)], radius=S(4), fill=(35, 40, 50))
    draw.text((S(12), S(208)), "Touchscreen & Calibration", fill=C_WHITE, font=font_bold)
    draw.text((S(14), S(226)), "State: PRESSED", fill=C_CYAN, font=font_xs)
    draw.text((S(14), S(242)), "Screen: X:120 Y:185", fill=C_WHITE, font=font_mono)
    draw.text((S(14), S(258)), "Raw ADC: X:2048 Y:1950", fill=C_GRAY_TEXT, font=font_mono)
    
    # Calibrate Button
    draw.rounded_rectangle([S(14), S(276), S(226), S(304)], radius=S(5), fill=(161, 98, 7), outline=C_YELLOW, width=1)
    draw.text((S(30), S(284)), "START TOUCH CALIBRATION", fill=C_WHITE, font=font_sm)
    return img

# ==============================================================================
# 9. SHOWCASE MONTAGE
# ==============================================================================
def render_showcase(screens, labels):
    # Screens side by side with bezels and dropshadows
    n = len(screens)
    margin = S(16)
    spacing = S(14)
    total_w = margin * 2 + W * n + spacing * (n - 1)
    total_h = margin * 2 + H + S(36)
    
    bg = Image.new("RGB", (total_w, total_h), (11, 15, 23))
    draw = ImageDraw.Draw(bg)
    
    # Title Header in showcase
    draw.text((margin, S(10)), "LilyGO T-HMI (ESP32-S3) Multi-App OS & Games Arcade Showcase", fill=C_CYAN, font=font_hero)
    
    for i, (scr, lbl) in enumerate(zip(screens, labels)):
        x = margin + i * (W + spacing)
        y = margin + S(30)
        # Drop shadow / frame
        draw.rounded_rectangle([x - S(4), y - S(4), x + W + S(4), y + H + S(4)], radius=S(10), fill=(22, 28, 40), outline=(50, 65, 90), width=SCALE)
        bg.paste(scr, (x, y))
        draw.text((x + S(6), y + H + S(8)), lbl, fill=C_WHITE, font=font_sm)
        
    return bg

if __name__ == "__main__":
    out_dir = "/Users/yolet/T-HMI-MultiApp/image"
    os.makedirs(out_dir, exist_ok=True)
    
    sc_launcher = render_launcher()
    sc_launcher.save(f"{out_dir}/screen_launcher.png")
    print("Saved screen_launcher.png")
    
    sc_drawpad = render_drawpad()
    sc_drawpad.save(f"{out_dir}/screen_drawpad.png")
    print("Saved screen_drawpad.png")
    
    sc_picker = render_color_picker()
    sc_picker.save(f"{out_dir}/screen_colorpicker.png")
    print("Saved screen_colorpicker.png")
    
    sc_arcade = render_arcade_menu()
    sc_arcade.save(f"{out_dir}/screen_arcade.png")
    print("Saved screen_arcade.png")
    
    sc_gourdy = render_gourdy()
    sc_gourdy.save(f"{out_dir}/screen_gourdy.png")
    print("Saved screen_gourdy.png")
    
    sc_dandyshop = render_dandy_shop()
    sc_dandyshop.save(f"{out_dir}/screen_dandyshop.png")
    print("Saved screen_dandyshop.png")
    
    sc_breakout = render_breakout()
    sc_breakout.save(f"{out_dir}/screen_breakout.png")
    print("Saved screen_breakout.png")
    
    sc_sysmonitor = render_sysmonitor()
    sc_sysmonitor.save(f"{out_dir}/screen_sysmonitor.png")
    print("Saved screen_sysmonitor.png")
    
    sc_sensors = render_sensors()
    sc_sensors.save(f"{out_dir}/screen_sensors.png")
    print("Saved screen_sensors.png")
    
    screens_for_showcase = [sc_launcher, sc_drawpad, sc_arcade, sc_gourdy, sc_dandyshop, sc_breakout, sc_sysmonitor]
    labels_for_showcase = [
        "1. App Launcher",
        "2. Drawing Pad (Solid)",
        "3. Games Arcade (Hub)",
        "4. Gardenview Halloween",
        "5. Dandy's Elevator Shop",
        "6. Breakout Arcade",
        "7. System Health & Sensors"
    ]
    showcase = render_showcase(screens_for_showcase, labels_for_showcase)
    showcase.save(f"{out_dir}/showcase.png")
    print("Saved showcase.png")
