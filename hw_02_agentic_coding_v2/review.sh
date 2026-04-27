#!/usr/bin/env bash
# Spustí code review pipeline na zadaném souboru.
# Použití: ./review.sh examples/budget_tracker.py
#          ./review.sh examples/vulnerable_app.py --output report.md

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"

if [ ! -f "$ENV_FILE" ]; then
    echo "Chybí .env soubor. Zkopíruj .env.example a doplň OPENAI_API_KEY."
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "Použití: $0 <soubor_nebo_adresář> [--output report.md]"
    echo ""
    echo "Příklady:"
    echo "  $0 examples/budget_tracker.py"
    echo "  $0 examples/vulnerable_app.py"
    echo "  $0 examples/clean_service.py"
    echo "  $0 examples/ --output report.md"
    exit 1
fi

cd "$SCRIPT_DIR"
uv run --env-file "$ENV_FILE" python main.py "$@"
