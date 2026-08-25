"""Entry point for ``python -m ctsb_converter``."""

import sys

from .cli import main

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
