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

# Zkopíruj .claude.json z hostu jako writable soubor.
# Bind mount přes různé filesystémy neumí atomic rename – Claude by crashoval.
if [ -f /tmp/host-claude.json ]; then
    cp /tmp/host-claude.json /root/.claude.json
fi

# Odstraň marketplace/plugins z kontejneru – vše definujeme sami v .claude/
# Host soubory se nemění, jen kontejnerový pohled je čistý.
rm -rf /root/.claude/plugins 2>/dev/null || true

print_header() {
    echo ""
    echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${BLUE}║     Python Learning Assistant – Agent Setup     ║${NC}"
    echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
    echo ""
}

check_keys() {
    # Claude Code – přihlášení přes `claude login` (OAuth, bez API klíče)
    if [ -f "/root/.claude.json" ]; then
        echo -e "${GREEN}✓ Claude Code – přihlášení nalezeno${NC}"
    else
        echo -e "${YELLOW}⚠ Claude Code – přihlášení nenalezeno. Spusť: claude login${NC}"
    fi

    [ -n "$OPENAI_API_KEY" ]  && echo -e "${GREEN}✓ OPENAI_API_KEY nastaven (Codex dostupný)${NC}"
    [ -n "$GITHUB_TOKEN" ]    && echo -e "${GREEN}✓ GITHUB_TOKEN nastaven${NC}"
    [ -n "$BRAVE_API_KEY" ]   && echo -e "${GREEN}✓ BRAVE_API_KEY nastaven${NC}"

    echo ""
    return 0
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
    echo -e "  docker-compose run --rm claude"
    echo ""
    echo -e "  ${YELLOW}# Simulace mentor↔student (2 terminály)${NC}"
    echo -e "  docker-compose run --rm debug    # terminál 1, pak /simulate faktorial"
    echo -e "  ./watch-debug.sh                 # terminál 2"
    echo ""
    echo -e "  ${YELLOW}# Jednorázový příkaz${NC}"
    echo -e "  docker-compose run --rm claude claude \"/lesson rekurze\""
    echo ""
    echo -e "  ${YELLOW}# Bash shell pro ruční práci${NC}"
    echo -e "  docker-compose run --rm claude bash"
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
    debug)
        print_header
        check_keys || true
        echo -e "${GREEN}Spouštím Claude Code v debug módu (logy → /workspace/debug.log)...${NC}"
        echo -e "${YELLOW}⏱  Automatické ukončení po 5 minutách nečinnosti${NC}"
        echo ""

        IDLE_TIMEOUT=480  # 8 minut v sekundách
        LOG=/workspace/debug.log

        claude --debug-file "$LOG" &
        CLAUDE_PID=$!

        # Watchdog – každých 30s zkontroluje kdy byl log naposledy změněn
        (
            sleep 60  # první kontrola až po minutě
            while kill -0 $CLAUDE_PID 2>/dev/null; do
                sleep 30
                if [ -f "$LOG" ]; then
                    last_mod=$(stat -c %Y "$LOG" 2>/dev/null || echo 0)
                    now=$(date +%s)
                    idle=$((now - last_mod))
                    if [ $idle -gt $IDLE_TIMEOUT ]; then
                        echo -e "\n${YELLOW}⏱  Session neaktivní ${IDLE_TIMEOUT}s – ukončuji.${NC}" >&2
                        kill $CLAUDE_PID 2>/dev/null
                        break
                    fi
                fi
            done
        ) &

        wait $CLAUDE_PID
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
        exec "$@"
        ;;
esac
