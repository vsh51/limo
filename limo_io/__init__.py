"""limo_io — fraction formatting utilities for the LIMO linear program solver.

File format parsing (JSON, CSV, TXT) is handled by the C++ binary
(src/io/FileParser.hpp) via the --format and --parse-only CLI flags.
This package provides only the Python-side helpers needed to translate
the binary's native {num, den} fraction representation into human-readable
strings for the web frontend.

Public API
----------
format_fraction(frac)   — render a {num, den} dict as a readable string
parse_fraction(value)   — convert a string fraction to {num, den}
convert_output(obj)     — recursively replace {num, den} dicts with strings
"""

from limo_io.formatters import format_fraction, parse_fraction, convert_output

__all__ = ["format_fraction", "parse_fraction", "convert_output"]
