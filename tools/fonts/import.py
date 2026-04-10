import subprocess
import os

subprocess.run(["python3", "convert_font.py", "--name", "block", "font_block.png"])
subprocess.run(["python3", "convert_font.py", "--name", "light", "font_light.png"])
subprocess.run(["python3", "convert_font.py", "--name", "bold", "font_bold.png"])
subprocess.run(["python3", "convert_font.py", "--name", "special", "--start", "128", "--end", "255", "font_bold.png"])

files = ["font_block.h", "font_light.h", "font_bold.h", "font_special.h"]

with open("font.h", "w") as out:
    out.write("/* This file is part of the copingTracker firmware. It is auto-generated and should no be edited. */\n\n")
    out.write("#ifndef FONT_H\n")
    out.write("#define FONT_H\n\n")
    out.write("#include <stdint.h>\n\n")
    out.write("typedef uint16_t font_t[96][10];\n")
    out.write("\n")
    
    for f in files:
        with open(f, "r") as inp:
            out.write(inp.read())

    out.write("#define FONT_COUNT 3\n")
    out.write("static const font_t *fonts[FONT_COUNT] = { &FONT_LIGHT_BITMAP, &FONT_BOLD_BITMAP, &FONT_BLOCK_BITMAP };\n")

    out.write("\n#endif // FONT_H\n")

# clean up intermediate files
os.rename("font.h", "../../sources/Adapters/picoTracker/display/font.h")
os.remove("font_block.h")
os.remove("font_light.h")
os.remove("font_bold.h")
os.remove("font_special.h")