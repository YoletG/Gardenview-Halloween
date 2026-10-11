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
        ("#2", "Breakout", "Retro arcade brick breaker", C_ORANGE),
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
    
    # Canvas artwork: smooth colorful strokes
    # Draw a stylized mountain sunset with clean anti-aliasing and solid overlapping paths
    sun_cx, sun_cy = S(120), S(140)
    draw.ellipse([sun_cx - S(32), sun_cy - S(32), sun_cx + S(32), sun_cy + S(32)], fill=C_YELLOW)
    
    # Smooth mountain layers
    mountain_pts = [
        (S(10), S(240)), (S(70), S(160)), (S(130), S(220)), 
        (S(180), S(150)), (S(230), S(240))
    ]
    draw.line([(S(10), S(240)), (S(70), S(160)), (S(130), S(220))], fill=C_CYAN, width=S(6), joint="curve")
    draw.line([(S(130), S(220)), (S(180), S(150)), (S(230), S(240))], fill=(168, 85, 247), width=S(6), joint="curve")
    
    # Caligraphy ribbon / signature stroke
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
    
    # Active HSV color wheel swatch
    draw.ellipse([S(6), S(294), S(30), S(318)], fill=C_GREEN, outline=C_WHITE, width=SCALE)
    
    # Swatch pills
    pal = [C_WHITE, C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN]
    for i, col in enumerate(pal):
        px = S(38 + i * 33)
        draw.rounded_rectangle([px, S(297), px + S(28), S(315)], radius=S(4), fill=col, outline=(col == C_GREEN and C_WHITE or (70, 80, 100)), width=1)
        
    return img

# ==============================================================================
# 3. COLOR PICKER MODAL (IbisPaint-style)
# ==============================================================================
def render_color_picker():
    img = render_drawpad()
    draw = ImageDraw.Draw(img)
    
    # Modal background
    modal_bg = (18, 22, 30)
    draw.rectangle([0, S(26), W, H], fill=modal_bg)
    draw.rectangle([0, S(26), W, H], outline=C_CYAN, width=1)
    
    draw.text((S(48), S(32)), "COLOR PALETTE (HSV)", fill=C_WHITE, font=font_bold)
    draw.text((S(220), S(30)), "X", fill=C_RED, font=font_bold)
    
    # 1. Rainbow Hue Slider
    hue_y = S(56)
    hue_h = S(20)
    for x in range(S(16), S(224)):
        hue = (x - S(16)) / float(S(208))
        r, g, b = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
        draw.line([x, hue_y, x, hue_y + hue_h], fill=(int(r*255), int(g*255), int(b*255)))
    draw.rectangle([S(16), hue_y, S(224), hue_y + hue_h], outline=C_WHITE, width=1)
    # Hue thumb
    thumb_x = S(16 + int(0.33 * 208))
    draw.rectangle([thumb_x - S(3), hue_y - S(2), thumb_x + S(3), hue_y + hue_h + S(2)], outline=C_WHITE, fill=(0, 0, 0), width=1)
    
    # 2. Saturation / Value Gradient Box
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
    # Cursor crosshair
    draw.ellipse([box_x + S(160) - S(5), box_y + S(20) - S(5), box_x + S(160) + S(5), box_y + S(20) + S(5)], outline=C_WHITE, width=SCALE)
    
    # 3. Preview Swatch
    draw.rounded_rectangle([S(10), S(194), S(90), S(234)], radius=S(5), fill=C_GREEN, outline=C_WHITE, width=1)
    draw.text((S(22), S(206)), "#22C55E", fill=(0, 0, 0), font=font_mono)
    
    # 4. Preset pills
    presets = [C_WHITE, C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN, C_BLUE, (236, 72, 153)]
    for i, p_col in enumerate(presets):
        px = S(100 + (i % 4) * 32)
        py = S(194 if i < 4 else 216)
        draw.rounded_rectangle([px, py, px + S(28), py + S(18)], radius=S(3), fill=p_col, outline=(p_col == C_GREEN and C_WHITE or (70, 80, 100)), width=1)
        
    # 5. Apply Button
    draw.rounded_rectangle([S(16), S(248), S(224), S(286)], radius=S(6), fill=C_GREEN, outline=C_WHITE, width=1)
    draw.text((S(70), S(258)), "APPLY COLOR", fill=(0, 0, 0), font=font_bold)
    return img

# ==============================================================================
# 4. BREAKOUT GAME
# ==============================================================================
def render_breakout():
    img = Image.new("RGB", (W, H), C_BLACK)
    draw = ImageDraw.Draw(img)
    draw_status_bar(draw, "Breakout", is_home=False)
    
    # Score Header
    draw.rectangle([0, S(26), W, S(50)], fill=C_TOOLBAR)
    draw.line([0, S(50), W, S(50)], fill=(45, 55, 75), width=1)
    draw.text((S(8), S(31)), "Score: 180", fill=C_WHITE, font=font_bold)
    draw.text((S(95), S(31)), "Hi: 420", fill=C_WHITE, font=font_bold)
    
    # Lives (hearts)
    for i in range(3):
        hx = S(180 + i * 16)
        draw.ellipse([hx, S(33), hx + S(10), S(43)], fill=C_RED, outline=C_WHITE, width=1)
        
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
# 5. SYSTEM MONITOR (SoC TAB)
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
# 6. SYSTEM MONITOR (SENSORS TAB)
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
# 7. SHOWCASE MONTAGE
# ==============================================================================
def render_showcase(screens):
    # 4 screens side by side with bezels and dropshadows
    margin = S(16)
    spacing = S(16)
    total_w = margin * 2 + W * 4 + spacing * 3
    total_h = margin * 2 + H + S(36)
    
    bg = Image.new("RGB", (total_w, total_h), (11, 15, 23))
    draw = ImageDraw.Draw(bg)
    
    # Title Header in showcase
    font_hero = get_font(16, mono=False)
    draw.text((margin, S(10)), "LilyGO T-HMI (ESP32-S3) Multi-App System Showcase", fill=C_CYAN, font=font_hero)
    
    labels = ["1. App Launcher", "2. Drawing Pad (Solid Strokes)", "3. Breakout Arcade Game", "4. System & Sensors Monitor"]
    
    for i, (scr, lbl) in enumerate(zip(screens, labels)):
        x = margin + i * (W + spacing)
        y = margin + S(30)
        # Drop shadow / frame
        draw.rounded_rectangle([x - S(4), y - S(4), x + W + S(4), y + H + S(4)], radius=S(10), fill=(22, 28, 40), outline=(50, 65, 90), width=SCALE)
        bg.paste(scr, (x, y))
        draw.text((x + S(8), y + H + S(8)), lbl, fill=C_WHITE, font=font_sm)
        
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
    
    sc_breakout = render_breakout()
    sc_breakout.save(f"{out_dir}/screen_breakout.png")
    print("Saved screen_breakout.png")
    
    sc_sysmonitor = render_sysmonitor()
    sc_sysmonitor.save(f"{out_dir}/screen_sysmonitor.png")
    print("Saved screen_sysmonitor.png")
    
    sc_sensors = render_sensors()
    sc_sensors.save(f"{out_dir}/screen_sensors.png")
    print("Saved screen_sensors.png")
    
    showcase = render_showcase([sc_launcher, sc_drawpad, sc_breakout, sc_sysmonitor])
    showcase.save(f"{out_dir}/showcase.png")
    print("Saved showcase.png")
