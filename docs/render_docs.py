"""Render this project's simple Markdown documents without extra dependencies."""
from html import escape
from pathlib import Path
import re


def inline(value):
    value = escape(value)
    value = re.sub(r'`([^`]+)`', r'<code>\1</code>', value)
    value = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', value)
    return re.sub(r'\[([^]]+)\]\(([^)]+)\)',
                  lambda m: f'<a href="{m[2][:-3] + ".html" if m[2].endswith(".md") else m[2]}">{m[1]}</a>', value)


def render(source):
    lines = source.splitlines()
    body, contents = [], []
    index = 0
    while index < len(lines):
        line = lines[index]
        if not line.strip():
            index += 1
            continue
        if line.startswith('```'):
            code = []
            index += 1
            while index < len(lines) and not lines[index].startswith('```'):
                code.append(lines[index])
                index += 1
            body.append('<pre><code>' + escape('\n'.join(code)) + '</code></pre>')
        elif line.startswith('#'):
            level = len(line) - len(line.lstrip('#'))
            title = line[level:].strip()
            anchor = f'section-{len(contents)}' if level == 2 else 'document-title'
            if level == 2:
                contents.append((anchor, title))
            body.append(f'<h{level} id="{anchor}">{inline(title)}</h{level}>')
        elif line.startswith('|'):
            rows = []
            while index < len(lines) and lines[index].startswith('|'):
                cells = [cell.strip() for cell in lines[index].strip('|').split('|')]
                if not all(re.fullmatch(r':?-+:?', cell) for cell in cells):
                    tag = 'th' if not rows else 'td'
                    rows.append('<tr>' + ''.join(f'<{tag}>{inline(cell)}</{tag}>' for cell in cells) + '</tr>')
                index += 1
            body.append('<div class="table-wrap" tabindex="0"><table>' + ''.join(rows) + '</table></div>')
            continue
        elif line.startswith('- '):
            items = []
            while index < len(lines) and lines[index].startswith('- '):
                items.append('<li>' + inline(lines[index][2:]) + '</li>')
                index += 1
            body.append('<ul>' + ''.join(items) + '</ul>')
            continue
        else:
            paragraph = [line]
            index += 1
            while index < len(lines) and lines[index].strip() and not lines[index].startswith(('#', '|', '- ', '```')):
                paragraph.append(lines[index])
                index += 1
            body.append('<p>' + inline(' '.join(paragraph)) + '</p>')
            continue
        index += 1
    return '\n'.join(body), contents


if __name__ == '__main__':
    folder = Path(__file__).parent
    # Keep the existing readable, responsive document presentation.
    previous = (folder / 'PRD.html').read_text(encoding='utf-8')
    style = re.search(r'<style>.*?</style>', previous, re.S).group()
    for name in ('PRD', 'SDD', 'ImplementationPlan', 'BlueprintIntegration'):
        path = folder / (name + '.md')
        if not path.exists():
            continue
        source = path.read_text(encoding='utf-8')
        title = source.splitlines()[0].lstrip('# ')
        body, headings = render(source)
        toc = ''.join(f'<li><a href="#{key}">{inline(text)}</a></li>' for key, text in headings)
        links = ' '.join(f'<a href="{item}.html">{item}</a>' for item in ('PRD', 'SDD', 'ImplementationPlan', 'BlueprintIntegration'))
        html = f'<!doctype html>\n<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{escape(title)}</title>{style}</head><body><header class="top"><span>Virtual Performer Studio</span>{links}</header><div class="layout"><nav aria-label="목차"><strong>목차</strong><ol>{toc}</ol></nav><main>{body}</main></div></body></html>\n'
        (folder / (name + '.html')).write_text(html, encoding='utf-8')
