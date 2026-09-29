# HTML manual

Renders the in-app documentation as a single standalone `index.html`.

The page is built from the same sources the firmware uses, so it cannot drift
from what the device shows:

| source | used for |
| --- | --- |
| `tools/manual/raw_data/Documentation.rc` | page list and tab titles |
| `tools/manual/raw_data/*.copingDoc` | page content (colours + text) |
| `tools/fonts/font_light.png` | glyph atlas |

## Building

```sh
python3 tools/manual/webapp/build_webapp.py
```

Or as part of a normal build:

```sh
python3 build.py --manual
```

Requires Pillow (as does the font tooling).

## How it renders

Glyphs are drawn to a canvas from a 1-bit atlas extracted from the font PNG, so
the output is pixel identical to the device including the ANSI-style box drawing and
other icon and UI glyphs, which exist only in the font and have no Unicode equivalent.

A transparent `<pre>` sits on top of the canvas so the text stays selectable,
searchable and readable by screen readers. Its letter spacing and font size are
measured at runtime and adjusted so each character's selection box lines up with
the 10x10 cell drawn underneath.

Content is 30 columns wide, matching `DOC_COLUMNS` in the firmware; the page
scrolls to whatever height the document needs rather than paginating to the
device's 20 visible rows.

Everything (glyph atlas, page data, script) is inlined into `index.html`, so it
works when opened directly from disk with no server.
