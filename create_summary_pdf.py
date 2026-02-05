
from reportlab.lib.pagesizes import letter
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.lib.units import inch
import os

def create_pdf(filename, content):
    doc = SimpleDocTemplate(filename, pagesize=letter)
    styles = getSampleStyleSheet()
    story = []

    # Handle different newline formats and split
    lines = content.splitlines()

    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        if line.startswith('### '):
            story.append(Spacer(1, 0.2*inch))
            story.append(Paragraph(line.replace('### ', ''), styles['Heading3']))
            story.append(Spacer(1, 0.1*inch))
        elif line.startswith('## '):
            story.append(Spacer(1, 0.2*inch))
            story.append(Paragraph(line.replace('## ', ''), styles['Heading2']))
            story.append(Spacer(1, 0.1*inch))
        elif line.startswith('# '):
            story.append(Paragraph(line.replace('# ', ''), styles['Heading1']))
            story.append(Spacer(1, 0.2*inch))
        elif line.startswith('- '):
            # Use 'Bullet' style if available, or simulate with Normal
            story.append(Paragraph(line, styles['Normal'], bulletText='•'))
        else:
            story.append(Paragraph(line, styles['Normal']))

    doc.build(story)
    print(f"PDF created successfully: {filename}")

if __name__ == '__main__':
    source_file = 'GEMINI.md'
    if os.path.exists(source_file):
        with open(source_file, 'r', encoding='utf-8') as f:
            markdown_content = f.read()
        create_pdf('ProjectA_Summary.pdf', markdown_content)
    else:
        print(f"Error: {source_file} not found.")
