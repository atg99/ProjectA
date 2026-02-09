
import os
import random
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm, inch
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Flowable, Image
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY

# Configuration
FONT_DIR = r"C:\Users\Monstera\.gemini\skills\canvas-design\canvas-fonts"
FONT_HEADER = "Tektur"
FONT_BODY = "JetBrainsMono"

# Colors (Neural Grid Systems Palette)
COLOR_BG = colors.Color(0.05, 0.05, 0.05) # Almost Black
COLOR_TEXT = colors.Color(0.9, 0.9, 0.9) # Off-white
COLOR_ACCENT_1 = colors.Color(0.0, 0.94, 1.0) # Cyan
COLOR_ACCENT_2 = colors.Color(1.0, 0.34, 0.2) # Orange/Red
COLOR_GRID = colors.Color(0.2, 0.2, 0.2) # Dark Grey

def register_fonts():
    try:
        pdfmetrics.registerFont(TTFont(FONT_HEADER, os.path.join(FONT_DIR, "Tektur-Medium.ttf")))
        pdfmetrics.registerFont(TTFont(FONT_BODY, os.path.join(FONT_DIR, "JetBrainsMono-Regular.ttf")))
        return True
    except Exception as e:
        print(f"Font loading failed: {e}. Using defaults.")
        return False

class BackgroundCanvas(canvas.Canvas):
    def __init__(self, filename, **kwargs):
        super().__init__(filename, **kwargs)
        self.pagesize = A4
        self.width, self.height = self.pagesize
        self.draw_hud_overlay() # Draw background for the first page

    def draw_hud_overlay(self):
        # Draw Dark Background
        self.saveState()
        self.setFillColor(COLOR_BG)
        self.rect(0, 0, self.width, self.height, fill=1, stroke=0)
        
        # Draw Grid Lines
        self.setStrokeColor(COLOR_GRID)
        self.setLineWidth(0.5)
        step = 20 * mm
        for x in range(0, int(self.width), int(step)):
            self.line(x, 0, x, self.height)
        for y in range(0, int(self.height), int(step)):
            self.line(0, y, self.width, y)

        # Draw Corner Markers (HUD style)
        self.setStrokeColor(COLOR_ACCENT_1)
        self.setLineWidth(2)
        m = 10 * mm # margin
        l = 10 * mm # length
        
        # Top Left
        self.line(m, self.height - m, m + l, self.height - m)
        self.line(m, self.height - m, m, self.height - m - l)
        
        # Top Right
        self.line(self.width - m, self.height - m, self.width - m - l, self.height - m)
        self.line(self.width - m, self.height - m, self.width - m, self.height - m - l)

        # Bottom Left
        self.line(m, m, m + l, m)
        self.line(m, m, m, m + l)

        # Bottom Right
        self.line(self.width - m, m, self.width - m - l, m)
        self.line(self.width - m, m, self.width - m, m + l)

        # Page Number / System ID
        self.setFont(FONT_BODY, 8)
        self.setFillColor(COLOR_ACCENT_2)
        # Use simple page number tracking or logic
        self.drawString(self.width - 40*mm, 12*mm, f"SYS.LOG.0{self._pageNumber}")
        self.drawString(15*mm, 12*mm, "CONFIDENTIAL // PROJECT_A")

        self.restoreState()

    def showPage(self):
        # Finish the current page
        super().showPage()
        # Start the next page by drawing the background immediately
        self.draw_hud_overlay()

class InventoryDiagram(Flowable):
    def __init__(self, width=400, height=200):
        Flowable.__init__(self)
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        c.saveState()
        c.translate(0, 0)
        
        # Grid settings
        cols = 10
        rows = 5
        cell_size = 30
        
        # Draw container
        c.setStrokeColor(COLOR_ACCENT_1)
        c.setLineWidth(1)
        c.rect(0, 0, cols * cell_size, rows * cell_size, stroke=1, fill=0)
        
        # Draw grid cells
        c.setStrokeColor(COLOR_GRID)
        c.setLineWidth(0.5)
        for x in range(cols + 1):
            c.line(x * cell_size, 0, x * cell_size, rows * cell_size)
        for y in range(rows + 1):
            c.line(0, y * cell_size, cols * cell_size, y * cell_size)
            
        # Draw "Items" (Tetris blocks)
        items = [
            {'color': COLOR_ACCENT_2, 'coords': [(0,0), (0,1), (0,2), (1,2)]}, # L-shape
            {'color': colors.Color(0.5, 1.0, 0.5), 'coords': [(3,3), (4,3), (3,4), (4,4)]}, # Square
            {'color': colors.Color(1.0, 0.2, 0.8), 'coords': [(6,0), (7,0), (8,0), (9,0)]}, # Line
        ]
        
        for item in items:
            c.setFillColor(item['color'])
            for (gx, gy) in item['coords']:
                # Invert Y for drawing from bottom up if needed, but here 0,0 is bottom left relative to flowable
                c.rect(gx * cell_size, gy * cell_size, cell_size, cell_size, fill=1, stroke=1)
                
        # Label
        c.setFont(FONT_BODY, 10)
        c.setFillColor(COLOR_TEXT)
        c.drawString(0, -15, "FIG 01.1: SPATIAL INVENTORY MATRIX")
        
        c.restoreState()

class ArchitectureDiagram(Flowable):
    def __init__(self, width=400, height=200):
        Flowable.__init__(self)
        self.width = width
        self.height = height

    def draw(self):
        c = self.canv
        c.saveState()
        
        # Center Node (Server)
        cx, cy = self.width / 2, self.height / 2
        
        # Draw Connections
        clients = [
            (cx - 100, cy + 50), (cx + 100, cy + 50),
            (cx - 100, cy - 50), (cx + 100, cy - 50)
        ]
        
        c.setStrokeColor(COLOR_ACCENT_1)
        c.setLineWidth(1)
        for (x, y) in clients:
            c.line(cx, cy, x, y)
            
        # Draw Server
        c.setFillColor(COLOR_ACCENT_2)
        c.circle(cx, cy, 30, fill=1, stroke=0)
        c.setFillColor(colors.black)
        c.drawCentredString(cx, cy - 3, "SERVER")
        
        # Draw Clients
        c.setFillColor(COLOR_ACCENT_1)
        for i, (x, y) in enumerate(clients):
            c.circle(x, y, 20, fill=1, stroke=0)
            c.setFillColor(colors.black)
            c.drawCentredString(x, y - 3, f"CLI_0{i+1}")
            c.setFillColor(COLOR_ACCENT_1)

        # Label
        c.setFont(FONT_BODY, 10)
        c.setFillColor(COLOR_TEXT)
        c.drawString(0, 10, "FIG 02.0: CLIENT-SERVER REPLICATION TOPOLOGY")

        c.restoreState()

def create_pdf(filename, content):
    fonts_loaded = register_fonts()
    header_font = FONT_HEADER if fonts_loaded else "Helvetica-Bold"
    body_font = FONT_BODY if fonts_loaded else "Courier"

    doc = SimpleDocTemplate(filename, pagesize=A4, 
                            rightMargin=20*mm, leftMargin=20*mm, 
                            topMargin=20*mm, bottomMargin=20*mm)
    
    styles = getSampleStyleSheet()
    
    # Custom Styles
    style_h1 = ParagraphStyle('H1', parent=styles['Heading1'], fontName=header_font, fontSize=24, leading=28, textColor=COLOR_ACCENT_1, spaceAfter=20)
    style_h2 = ParagraphStyle('H2', parent=styles['Heading2'], fontName=header_font, fontSize=18, leading=22, textColor=COLOR_ACCENT_2, spaceBefore=15, spaceAfter=10)
    style_h3 = ParagraphStyle('H3', parent=styles['Heading3'], fontName=header_font, fontSize=14, leading=18, textColor=colors.white, spaceBefore=10, spaceAfter=5)
    style_body = ParagraphStyle('Body', parent=styles['Normal'], fontName=body_font, fontSize=10, leading=14, textColor=COLOR_TEXT)
    style_bullet = ParagraphStyle('Bullet', parent=style_body, leftIndent=20, firstLineIndent=0, bulletIndent=10)

    story = []

    # Title Page content
    story.append(Spacer(1, 2*inch))
    story.append(Paragraph("PROJECT_A", style_h1))
    story.append(Paragraph("TECHNICAL SPECIFICATION // CLASSIFIED", style_h3))
    story.append(Spacer(1, 1*inch))
    story.append(Paragraph("GENERATED BY: GEMINI CLI", style_body))
    story.append(Paragraph("DATE: 2026-02-05", style_body))
    story.append(PageBreak())

    # Content Parsing
    lines = content.splitlines()
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        if line.startswith('### '):
            story.append(Paragraph(line.replace('### ', '').upper(), style_h3))
            
            # Inject diagrams based on keywords
            if "Inventory" in line:
                story.append(Spacer(1, 10))
                story.append(InventoryDiagram())
                story.append(Spacer(1, 20))
            if "Architecture" in line or "Networking" in line:
                story.append(Spacer(1, 10))
                story.append(ArchitectureDiagram())
                story.append(Spacer(1, 20))

        elif line.startswith('## '):
            story.append(Spacer(1, 0.2*inch))
            story.append(Paragraph(line.replace('## ', '').upper(), style_h2))
            story.append(Spacer(1, 10)) # Concrete spacer instead of abstract Flowable
        elif line.startswith('# '):
            # Already handled title manually, maybe ignore or treat as H1
            pass
        elif line.startswith('- '):
            # Convert color to hex string #RRGGBB
            hex_color = COLOR_ACCENT_1.hexval()
            if isinstance(hex_color, int):
                hex_str = f"#{hex_color:06x}"
            else:
                hex_str = str(hex_color).replace('0x', '#')
            
            story.append(Paragraph(f"<font color='{hex_str}'>></font> " + line[2:], style_bullet))
        else:
            story.append(Paragraph(line, style_body))

    # Build using custom canvas
    doc.build(story, canvasmaker=BackgroundCanvas)
    print(f"Designed PDF created: {filename}")

if __name__ == '__main__':
    source_file = 'GEMINI.md'
    if os.path.exists(source_file):
        with open(source_file, 'r', encoding='utf-8') as f:
            markdown_content = f.read()
        create_pdf('ProjectA_Design_Doc.pdf', markdown_content)
    else:
        print(f"Error: {source_file} not found.")
