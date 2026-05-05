"""CLI entry point pro Code Review Supervisor."""

from __future__ import annotations

import asyncio
import sys
from pathlib import Path

from dotenv import load_dotenv

load_dotenv()

from .supervisor import CodeReviewSupervisor


def main() -> None:
    if len(sys.argv) < 2:
        print("Použití: code-reviewer <soubor.py>")
        print("Příklad: code-reviewer examples/buggy_app.py")
        sys.exit(1)

    target = Path(sys.argv[1])
    if not target.exists():
        print(f"Chyba: soubor '{target}' neexistuje")
        sys.exit(1)

    output_file = target.with_suffix(".review.md")

    async def run():
        supervisor = CodeReviewSupervisor(target)
        report = await supervisor.review()

        output_file.write_text(report, encoding="utf-8")
        print(f"✅ Report uložen: {output_file}")
        print("\n" + "─" * 60 + "\n")
        print(report)

    asyncio.run(run())


if __name__ == "__main__":
    main()
