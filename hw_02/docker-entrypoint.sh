#!/bin/bash
set -e

# Barvy pro výstup
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${BLUE}║     Python Learning Assistant – Agent Setup     ║${NC}"
    echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
    echo ""
}

check_keys() {
    local ok=true

    if [ -z "$ANTHROPIC_API_KEY" ]; then
        echo -e "${RED}✗ ANTHROPIC_API_KEY není nastaven${NC}"
        ok=false
    else
        echo -e "${GREEN}✓ ANTHROPIC_API_KEY nastaven${NC}"
    fi

    if [ -z "$OPENAI_API_KEY" ]; then
        echo -e "${YELLOW}⚠ OPENAI_API_KEY není nastaven (Codex nebude fungovat)${NC}"
    else
        echo -e "${GREEN}✓ OPENAI_API_KEY nastaven${NC}"
    fi

    if [ -z "$GITHUB_TOKEN" ]; then
        echo -e "${YELLOW}⚠ GITHUB_TOKEN není nastaven (GitHub MCP server bude omezen)${NC}"
    else
        echo -e "${GREEN}✓ GITHUB_TOKEN nastaven${NC}"
    fi

    if [ -z "$BRAVE_API_KEY" ]; then
        echo -e "${YELLOW}⚠ BRAVE_API_KEY není nastaven (webové vyhledávání nebude fungovat)${NC}"
    else
        echo -e "${GREEN}✓ BRAVE_API_KEY nastaven${NC}"
    fi

    echo ""
    $ok
}

show_help() {
    print_header
    check_keys || true

    echo -e "${BOLD}Dostupné příkazy:${NC}"
    echo ""
    echo -e "  ${CYAN}claude${NC}                    Spustí Claude Code (interaktivní)"
    echo -e "  ${CYAN}claude \"/lesson rekurze\"${NC}  Vygeneruje lekci o rekurzi"
    echo -e "  ${CYAN}claude \"/check soubor\"${NC}    Zkontroluje cvičení studenta"
    echo -e "  ${CYAN}claude \"/explain lambda\"${NC}  Vysvětlí Python koncept"
    echo -e "  ${CYAN}claude \"/progress\"${NC}         Zobrazí pokrok studenta"
    echo ""
    echo -e "  ${CYAN}codex${NC}                     Spustí Codex CLI (interaktivní)"
    echo -e "  ${CYAN}codex \"dotaz\"${NC}             Jednorázový dotaz na Codex"
    echo ""
    echo -e "  ${CYAN}pytest tests/ -v${NC}           Spustí všechny testy"
    echo -e "  ${CYAN}ruff check .${NC}               Zkontroluje kvalitu kódu"
    echo ""
    echo -e "${BOLD}Příklady spuštění z hostitele:${NC}"
    echo ""
    echo -e "  ${YELLOW}# Interaktivní Claude Code${NC}"
    echo -e "  docker compose run --rm claude"
    echo ""
    echo -e "  ${YELLOW}# Interaktivní Codex${NC}"
    echo -e "  docker compose run --rm codex"
    echo ""
    echo -e "  ${YELLOW}# Jednorázový příkaz${NC}"
    echo -e "  docker compose run --rm claude claude \"/lesson rekurze\""
    echo ""
    echo -e "  ${YELLOW}# Bash shell pro ruční práci${NC}"
    echo -e "  docker compose run --rm claude bash"
    echo ""
}

case "$1" in
    help|--help|-h|"")
        show_help
        ;;
    claude)
        print_header
        check_keys || true
        echo -e "${GREEN}Spouštím Claude Code...${NC}"
        echo ""
        shift
        exec claude "$@"
        ;;
    codex)
        print_header
        check_keys || true
        echo -e "${GREEN}Spouštím Codex CLI...${NC}"
        echo ""
        shift
        exec codex "$@"
        ;;
    bash|sh)
        print_header
        check_keys || true
        echo -e "${GREEN}Spouštím shell. Příkazy: claude, codex, pytest, ruff${NC}"
        echo ""
        exec bash
        ;;
    *)
        # Předej příkaz přímo shellu
        exec "$@"
        ;;
esac
