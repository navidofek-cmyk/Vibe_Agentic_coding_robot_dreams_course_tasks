#!/bin/bash
# Sleduj debug log v reálném čase – ukazuje komunikaci agentů i technické detaily
# Terminál 1: docker-compose run --rm debug   (spustí Claude s debug logem)
# Terminál 2: ./watch-debug.sh                (sleduj co se děje)

LOG=workspace/debug.log

echo "Čekám na $LOG ..."
until [ -f "$LOG" ]; do sleep 0.5; done
echo "Debug log nalezen, sleduji..."
echo "════════════════════════════════════════"

tail -f "$LOG" | while IFS= read -r line; do
    # Volání sub-agentů
    if echo "$line" | grep -q "subagent\|Agent(\|student\|simulate"; then
        echo -e "\033[1;35m[AGENT]  $line\033[0m"
    # Tool use (Bash, Write, Read, Edit)
    elif echo "$line" | grep -q "Tool\|tool_use\|tool_result"; then
        echo -e "\033[0;36m[TOOL]   $line\033[0m"
    # Hooks
    elif echo "$line" | grep -q "Hook\|hooks"; then
        echo -e "\033[0;33m[HOOK]   $line\033[0m"
    # Skills načítání
    elif echo "$line" | grep -q "skill\|Loaded\|commands"; then
        echo -e "\033[0;32m[SKILL]  $line\033[0m"
    # Chyby a varování
    elif echo "$line" | grep -q "\[ERROR\]\|\[WARN\]"; then
        echo -e "\033[0;31m[!]      $line\033[0m"
    # MCP servery
    elif echo "$line" | grep -q "MCP\|mcp"; then
        echo -e "\033[0;34m[MCP]    $line\033[0m"
    fi
done
