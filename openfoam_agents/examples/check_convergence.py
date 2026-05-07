"""
Analýza konvergence OpenFOAM simulace ze log souboru.
Použití: python3 check_convergence.py log.icoFoam
         python3 check_convergence.py log.simpleFoam
"""

import re
import sys
from pathlib import Path


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


def check_convergence(residuals: dict[str, list[float]], threshold: float = 1e-4) -> dict[str, bool]:
    return {
        field: values[-1] < threshold if values else False
        for field, values in residuals.items()
    }


def main():
    if len(sys.argv) < 2:
        print("Použití: python3 check_convergence.py <log_soubor>")
        sys.exit(1)

    log_path = sys.argv[1]
    if not Path(log_path).exists():
        print(f"Log soubor nenalezen: {log_path}")
        sys.exit(1)

    residuals = parse_residuals(log_path)
    if not residuals:
        print("V log souboru nebyla nalezena žádná data o residuálech.")
        sys.exit(1)

    converged = check_convergence(residuals)

    print(f"\n{'Pole':<12} {'Počáteční res.':<18} {'Finální res.':<18} {'Konvergence'}")
    print("-" * 60)
    for field, values in residuals.items():
        status = "OK" if converged[field] else "NEDOKONVERGOVÁNO"
        print(f"{field:<12} {values[0]:<18.4e} {values[-1]:<18.4e} {status}")

    all_ok = all(converged.values())
    print(f"\nCelková konvergence: {'ANO' if all_ok else 'NE'}")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
