import re, pdfplumber, os, base64

PDF = r"D:\projects\assignment\k-kick_tree_hashing\article\article.pdf"
title = None
with pdfplumber.open(PDF) as pdf:
    head_text = "\n".join((p.extract_text() or "") for p in pdf.pages[:10])

m = re.search(r'题目[：:]\s*([\s\S]{1,200}?)(?=\n院系|\n学院|\n专业|\n本科生姓名|\n学\s*号|\n指导教师)', head_text)
if m:
    title = re.sub(r'\s+', '', m.group(1)).strip()

if not title:
    m = re.search(r'毕业论文题目[：:]\s*([\s\S]{1,200}?)(?=(?:本科|学士学位|学生姓名|指导教师|学\s*校\s*代\s*码|软件工程专业|计算机|级\s*本科生))', head_text)
    if m:
        title = re.sub(r'\s+', '', m.group(1)).strip()

if not title:
    m = re.search(r'论\s*文\s*题\s*目\s*([\s\S]{0,200}?)(?=作\s*者\s*姓\s*名|学位类别|研\s*究\s*方\s*向|学\s*生\s*姓\s*名)', head_text)
    if m:
        title = re.sub(r'\s+', '', m.group(1)).strip()

if not title or len(title) < 5:
    title = os.path.splitext(os.path.basename(PDF))[0]

title_clean = re.sub(r'[\\/:*?"<>|\r\n\t]', '', title).strip()
if len(title_clean) > 120:
    title_clean = title_clean[:120]

# Write raw and base64
with open(os.path.join(os.path.dirname(PDF), "tmp", "title_out.txt"), "w", encoding="utf-8") as f:
    f.write(title_clean)
print(base64.b64encode(title_clean.encode('utf-8')).decode('ascii'))
