#!/usr/bin/env bash
# Spustí code review pipeline a automaticky uloží report do reports/.
# Použití: ./review.sh examples/budget_tracker.py

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"

if [ ! -f "$ENV_FILE" ]; then
    echo "Chybí .env soubor. Zkopíruj .env.example a doplň OPENAI_API_KEY."
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "Použití: $0 <soubor_nebo_adresář>"
    echo ""
    echo "Příklady:"
    echo "  $0 examples/budget_tracker.py"
    echo "  $0 examples/vulnerable_app.py"
    echo "  $0 examples/clean_service.py"
    exit 1
fi

TARGET="$1"
BASENAME="$(basename "$TARGET" .py)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
REPORTS_DIR="$SCRIPT_DIR/reports"
REPORT_FILE="$REPORTS_DIR/${BASENAME}_${TIMESTAMP}.md"

mkdir -p "$REPORTS_DIR"

cd "$SCRIPT_DIR"
uv run --env-file "$ENV_FILE" python main.py "$TARGET" --output "$REPORT_FILE"

echo ""
echo "Report uložen: $REPORT_FILE"
