import re
import sys

TOL = 0.1

# attributes that commonly contain coordinates
ATTRS = {
    "x", "y", "cx", "cy",
    "x1", "y1", "x2", "y2",
    "r", "rx", "ry",
    "width", "height",
    "dx", "dy"
}

num_re = re.compile(r"-?\d+(?:\.\d+)?")

def snap_number(n: float) -> str:
    nearest = round(n)
    if abs(n - nearest) < TOL:
        return str(nearest)
    return str(n)

def snap_value(val: str) -> str:
    def repl(match):
        return snap_number(float(match.group(0)))
    return num_re.sub(repl, val)

def process_svg(text: str) -> str:
    # snap attributes
    def attr_repl(match):
        name = match.group(1)
        value = match.group(2)

        if name in ATTRS:
            return f'{name}="{snap_value(value)}"'
        return match.group(0)

    text = re.sub(r'(\w+)\s*=\s*"([^"]+)"', attr_repl, text)

    # snap polygon/polyline points
    def points_repl(match):
        return f'points="{snap_value(match.group(1))}"'

    text = re.sub(r'points="([^"]+)"', points_repl, text)

    # snap path data (d attribute)
    def d_repl(match):
        return f'd="{snap_value(match.group(1))}"'

    text = re.sub(r'd="([^"]+)"', d_repl, text)

    return text

def main():
    if len(sys.argv) != 3:
        print("usage: python snap_svg.py input.svg output.svg")
        return

    inp, outp = sys.argv[1], sys.argv[2]

    with open(inp, "r", encoding="utf-8") as f:
        svg = f.read()

    cleaned = process_svg(svg)

    with open(outp, "w", encoding="utf-8") as f:
        f.write(cleaned)

if __name__ == "__main__":
    main()