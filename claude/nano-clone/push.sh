#!/bin/bash
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
TOKEN=$(cat "$HERE/token" | tr -d '[:space:]')
OWNER="navidofek-cmyk"
REPO="Vibe_Agentic_coding_robot_dreams_course_tasks"
FILE="nano-clone/README.md"
API="https://api.github.com/repos/$OWNER/$REPO/contents/$FILE"

# Načti aktuální SHA souboru na GitHubu
SHA=$(curl -s -H "Authorization: token $TOKEN" "$API" \
    | grep -o '"sha": *"[^"]*"' | head -1 | grep -o '[a-f0-9]\{40\}')

[ -z "$SHA" ] && echo "Chyba: nepodařilo se načíst SHA." && exit 1

# Pushni přes API
RESULT=$(curl -s -X PUT \
    -H "Authorization: token $TOKEN" \
    -H "Content-Type: application/json" \
    "$API" \
    -d "{\"message\":\"docs: update nano-clone README\",\"content\":\"$(base64 -w0 "$HERE/README.md")\",\"sha\":\"$SHA\"}")

echo "$RESULT" | grep -q '"commit"' \
    && echo "Hotovo — README pushnutý na GitHub." \
    || { echo "Chyba:"; echo "$RESULT" | grep -o '"message":"[^"]*"'; }
