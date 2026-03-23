"""Fraction formatting utilities compatible with the limo binary's rational output.

The limo binary represents all numbers as exact fractions: {"num": int, "den": int}.
These helpers convert between that wire format and human-readable strings.
"""

from fractions import Fraction
from math import gcd


def parse_fraction(value) -> dict:
    """Convert a string fraction (e.g. '3/4', '-5', '0') to {"num": n, "den": d}."""
    s = str(value).strip()
    if not s:
        return {"num": 0, "den": 1}
    f = Fraction(s)
    return {"num": f.numerator, "den": f.denominator}


def format_fraction(frac: dict) -> str:
    """Render a {"num": n, "den": d} dict as a human-readable string."""
    n, d = frac["num"], frac["den"]
    if d == 0:
        return "undef"
    if n == 0:
        return "0"
    if d < 0:
        n, d = -n, -d
    g = gcd(abs(n), abs(d))
    n, d = n // g, d // g
    return str(n) if d == 1 else f"{n}/{d}"


def convert_output(obj):
    """Recursively replace all {"num", "den"} fraction dicts with readable strings.

    Used to post-process the raw JSON output from the limo binary before sending
    it to the browser.
    """
    if isinstance(obj, dict):
        if (
            set(obj.keys()) == {"num", "den"}
            and isinstance(obj["num"], int)
            and isinstance(obj["den"], int)
        ):
            return format_fraction(obj)
        return {k: convert_output(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [convert_output(item) for item in obj]
    return obj
