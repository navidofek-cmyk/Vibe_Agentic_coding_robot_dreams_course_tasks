#!/usr/bin/env bash
# Demo: spustí review na všech třech ukázkových souborech za sebou.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "════════════════════════════════════════"
echo " DEMO — Code Review Multi-Agent Pipeline"
echo "════════════════════════════════════════"
echo ""

for file in budget_tracker.py vulnerable_app.py clean_service.py; do
    echo ">>> Reviewuji: examples/$file"
    echo ""
    bash "$SCRIPT_DIR/review.sh" "examples/$file"
    echo ""
    echo "Stiskni Enter pro další..."
    read -r
done

echo "Demo dokončeno."
