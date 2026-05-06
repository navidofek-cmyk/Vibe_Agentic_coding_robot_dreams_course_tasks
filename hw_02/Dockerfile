FROM node:22-slim

# Python + základní nástroje
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    git \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Python nástroje pro výuku (pytest, ruff)
RUN pip3 install --break-system-packages pytest ruff

# Claude Code CLI + Codex CLI
RUN npm install -g \
    @anthropic-ai/claude-code \
    @openai/codex

# MCP servery (stažené předem – agent je nemusí stahovat za běhu)
RUN npm install -g \
    @modelcontextprotocol/server-filesystem \
    @modelcontextprotocol/server-github \
    @modelcontextprotocol/server-brave-search \
    @modelcontextprotocol/server-sqlite \
    @modelcontextprotocol/server-memory

# Pracovní adresář = projekt studenta
WORKDIR /workspace

# Konfigurace Claude Code (zkopíruje .claude/ a CLAUDE.md)
COPY claude-code/.claude /root/.claude
COPY claude-code/CLAUDE.md /workspace/CLAUDE.md

# Konfigurace Codex CLI
RUN mkdir -p /root/.codex
COPY codex/config.yaml /root/.codex/config.yaml
COPY codex/AGENTS.md /workspace/AGENTS.md

# Ukázkové skripty a dokumentace
COPY claude-code/examples /workspace/examples/claude-code
COPY codex/examples       /workspace/examples/codex
COPY docs                 /workspace/docs

# Entrypoint skript
COPY docker-entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# Výchozí příkaz – interaktivní shell se zobrazenou nápovědou
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["help"]
