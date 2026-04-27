#!/usr/bin/env python3
"""
Code Review Multi-Agent Pipeline
=================================
Entry point — parses CLI arguments and runs the pipeline.

Usage:
    python main.py examples/vulnerable_app.py
    python main.py examples/clean_service.py
    python main.py examples/              # review all .py files in a directory
    python main.py examples/ --output report.md
"""

from __future__ import annotations

import argparse
import asyncio
import sys
from pathlib import Path

from pipeline import CodeReviewPipeline


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Multi-agent code review pipeline powered by Claude.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "target",
        type=Path,
        help="Python file or directory to review.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Save the report to a markdown file (optional).",
    )
    return parser.parse_args()


async def async_main() -> int:
    args = parse_args()

    if not args.target.exists():
        print(f"Error: '{args.target}' does not exist.", file=sys.stderr)
        return 1

    pipeline = CodeReviewPipeline()
    report, exit_code = await pipeline.run(args.target)

    print(report)

    if args.output:
        args.output.write_text(report, encoding="utf-8")
        print(f"Report saved to: {args.output}")

    return exit_code


def main() -> None:
    sys.exit(asyncio.run(async_main()))


if __name__ == "__main__":
    main()
