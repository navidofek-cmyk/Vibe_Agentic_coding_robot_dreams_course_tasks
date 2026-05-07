"""
Vykreslení průběhu residuálů z OpenFOAM log souboru.
Použití: python3 plot_residuals.py log.simpleFoam
Výstup:  residuals.png v aktuálním adresáři
"""

import re
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("Chybí matplotlib. Nainstaluj: pip install matplotlib")
    sys.exit(1)


def parse_residuals(log_path: str) -> dict[str, list[float]]:
    residuals: dict[str, list[float]] = {}
    pattern = re.compile(
        r"Solving for (\w+),\s+Initial residual = ([0-9.eE+-]+)"
    )
    with open(log_path) as f:
        for line in f:
            m = pattern.search(line)
            if m:
                field, res = m.group(1), float(m.group(2))
                residuals.setdefault(field, []).append(res)
    return residuals


def plot(residuals: dict[str, list[float]], output: str = "residuals.png"):
    fig, ax = plt.subplots(figsize=(10, 6))

    for field, values in residuals.items():
        ax.semilogy(values, label=field)

    ax.axhline(1e-4, color="red", linestyle="--", linewidth=0.8, label="threshold 1e-4")
    ax.set_xlabel("Iterace")
    ax.set_ylabel("Residuál (log škála)")
    ax.set_title("Konvergence OpenFOAM simulace")
    ax.legend()
    ax.grid(True, which="both", alpha=0.3)

    fig.savefig(output, dpi=150, bbox_inches="tight")
    print(f"Graf uložen: {output}")


def main():
    if len(sys.argv) < 2:
        print("Použití: python3 plot_residuals.py <log_soubor>")
        sys.exit(1)

    log_path = sys.argv[1]
    if not Path(log_path).exists():
        print(f"Log soubor nenalezen: {log_path}")
        sys.exit(1)

    residuals = parse_residuals(log_path)
    if not residuals:
        print("V log souboru nebyla nalezena žádná data o residuálech.")
        sys.exit(1)

    output = Path(log_path).stem + "_residuals.png"
    plot(residuals, output)


if __name__ == "__main__":
    main()
