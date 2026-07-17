# copingDoc File Format

The `.copingDoc` files in `tools/manual/raw_data/` define the on-screen documentation pages for the copingTracker firmware. They are converted to C++ byte arrays by `convert-documentation.py` and compiled into the firmware.

## File Structure

Each `.copingDoc` file is a plain text file consisting of repeating 3-line blocks. Each lines is exactly 30-characters wide. Each block represents one row of text on the display:

```
Line 1: foreground color (fg) — hex digits per character position
Line 2: background color (bg) — hex digits per character position
Line 3: text content — the actual characters to display
```

All three lines in a block must have the **same length** (number of characters). Lines are terminated by `\n` or `\r\n`.

### Color Encoding

Each character position on screen has a 4-bit foreground color and a 4-bit background color, packed into a single byte as `(fg << 4) | bg`.

In the `.copingDoc` file, the fg and bg lines use hex digits (`0`–`F`) to specify colors. A space character (` `) means "keep the previous color value for this position" (inherited from the last explicitly set position in the same line).

The color values map to the 16-color palette defined in the firmware's `GUIColor` constants.

### Text Content

The text line contains the actual characters to display. Any 8-bit byte value is valid (the firmware uses a custom bitmap font).

### Example

```
F   7                  CAE98F
0
ARPeggio              abcd|ARP
7                         87F
0
Cycle through the relative|
7       C7A7E797          87F7
0
offsets a,b,c&d. If a step|
7                         87
0
is 0 the arp starts over. |
8
0
---------------------------'´
```

This renders as 4 text rows followed by a separator line. The first row shows "ARPeggio" with a colored foreground, the `ł` marks a line break, and the text continues on subsequent rows.

## Registration

Each `.copingDoc` file is registered in `Documentation.rc` with a title:

```
filename.copingDoc;Display Title
```

The `convert-documentation.py` script reads this resource file, processes all listed `.copingDoc` files, and generates `Documentation.h` containing:

- An enum `DocumentationId` with one entry per document
- A `uint8_t` array per document with the packed (fg<<4 | bg, char) byte pairs
- A `DocumentationPage` lookup table mapping titles to data arrays

## Tooling

The format is designed to be human-readable and easy to generate programmatically. To create or modify documentation pages:

1. Create a `.copingDoc` file with 3-line blocks (fg, bg, text) of equal line lengths
2. Add an entry to `Documentation.rc`
3. Run `convert-documentation.py Documentation.rc ../../sources/Foundation/Constants/Documentation.h`
4. Rebuild the firmware