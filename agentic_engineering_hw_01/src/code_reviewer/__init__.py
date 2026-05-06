"""Multi-Agent Code Review Supervisor."""

from __future__ import annotations

import os


def sdk_env() -> dict[str, str]:
    """Předá ANTHROPIC_API_KEY z prostředí do Claude Code CLI subprocesu."""
    env = {}
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if api_key:
        env["ANTHROPIC_API_KEY"] = api_key
    return env
