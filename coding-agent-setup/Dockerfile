FROM node:22-slim

# Systémové závislosti (bez python3 z apt – uv obstará Python sám)
RUN apt-get update && apt-get install -y \
    git curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# uv – rychlý správce Pythonu a balíčků
COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv

# Python 3.12 + virtuální prostředí s výukovými nástroji
RUN uv python install 3.12
RUN uv venv /opt/venv --python 3.12
ENV VIRTUAL_ENV=/opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN uv pip install pytest ruff

# Claude Code CLI + Codex CLI
RUN npm install -g \
    @anthropic-ai/claude-code \
    @openai/codex

# MCP servery – TypeScript (npm)
RUN npm install -g \
    @modelcontextprotocol/server-filesystem \
    @modelcontextprotocol/server-github \
    @modelcontextprotocol/server-brave-search \
    @modelcontextprotocol/server-memory

# MCP server SQLite – Python (uv), spouštěn přes uvx
RUN uv pip install mcp-server-sqlite

WORKDIR /workspace

# Konfigurace Claude Code – projektová nastavení (skills, permissions, hooks)
# /root/.claude je vyhrazeno pro auth (mountováno z hostu za běhu)
COPY .claude /workspace/.claude
COPY CLAUDE.md /workspace/CLAUDE.md

# Konfigurace Codex CLI
RUN mkdir -p /root/.codex
COPY codex/config.yaml /root/.codex/config.yaml
COPY codex/AGENTS.md /workspace/AGENTS.md

# Ukázkové skripty a dokumentace
COPY examples       /workspace/examples/claude-code
COPY codex/examples /workspace/examples/codex
COPY docs           /workspace/docs

# Entrypoint skript
COPY docker-entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["help"]
