"""
Specialized review agents and supervisor agent.

All agents run the `claude` CLI in non-interactive print mode (`claude -p`).
No API key is required — authentication uses the active Claude Code session.

Structured output is requested via explicit JSON schema in each prompt.
"""

from __future__ import annotations

import asyncio
import json
import re

# ── JSON schema templates embedded in prompts ───────────────────────────────

REPORT_SCHEMA = """{
  "issues": [
    {
      "severity": "critical | high | medium | low | info",
      "title": "short title",
      "description": "what the problem is",
      "suggestion": "how to fix it"
    }
  ],
  "summary": "one-sentence overall assessment",
  "score": 7.5
}"""

VERDICT_SCHEMA = """{
  "verdict": "APPROVED | NEEDS_WORK | BLOCKED",
  "overall_score": 6.0,
  "critical_count": 0,
  "high_count": 2,
  "top_issues": ["issue 1", "issue 2"],
  "recommendation": "what to do next"
}"""

# ── Specialized agent system instructions ───────────────────────────────────

AGENT_SYSTEMS = {
    "security": (
        "You are a security engineer reviewing code for vulnerabilities. "
        "Focus on: SQL/command injection, hardcoded secrets, path traversal, "
        "insecure deserialization, broken authentication, sensitive data exposure, "
        "XSS, CSRF, and cryptographic weaknesses."
    ),
    "bugs": (
        "You are a QA engineer hunting logic errors and runtime bugs. "
        "Focus on: null/index errors, off-by-one, unhandled exceptions, race conditions, "
        "resource leaks, incorrect algorithms, and missing edge-case handling."
    ),
    "performance": (
        "You are a performance engineer identifying bottlenecks and inefficiencies. "
        "Focus on: O(n²)+ algorithms, N+1 queries, blocking I/O in async contexts, "
        "memory leaks, redundant computations, and missing caching."
    ),
    "style": (
        "You are a senior engineer reviewing code quality and maintainability. "
        "Focus on: naming, function length, cyclomatic complexity, dead code, "
        "duplication, SOLID violations, missing error handling, and clarity."
    ),
}


# ── CLI runner ───────────────────────────────────────────────────────────────


async def _run_claude_cli(prompt: str) -> str:
    """
    Run `claude -p --output-format json` with prompt on stdin.
    Returns the text in the `result` field of the CLI JSON envelope.
    Raises RuntimeError if the CLI exits with a non-zero code.
    """
    proc = await asyncio.create_subprocess_exec(
        "claude", "-p", "--output-format", "json",
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    stdout, stderr = await proc.communicate(input=prompt.encode())

    if proc.returncode != 0:
        raise RuntimeError(
            f"claude CLI exited {proc.returncode}: {stderr.decode().strip()}"
        )

    envelope = json.loads(stdout.decode())
    return envelope.get("result", "")


def _extract_json(text: str) -> dict:
    """
    Extract the first JSON object from a text response.
    Handles markdown ```json ... ``` fences and bare JSON.
    """
    # Strip markdown fences if present
    fenced = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.DOTALL)
    if fenced:
        return json.loads(fenced.group(1))

    # Try to find a bare JSON object
    match = re.search(r"\{.*\}", text, re.DOTALL)
    if match:
        return json.loads(match.group(0))

    raise ValueError(f"No JSON object found in response:\n{text[:300]}")


# ── Specialized agents ───────────────────────────────────────────────────────


async def run_specialized_agent(agent_type: str, code: str, filename: str) -> dict:
    """
    Run one specialized review agent via the Claude CLI.
    Returns a structured report dict.
    """
    system = AGENT_SYSTEMS[agent_type]

    prompt = f"""{system}

Review the code from file '{filename}' below and respond ONLY with a valid JSON object
matching this schema (no extra text, no markdown explanation outside the JSON):

{REPORT_SCHEMA}

Severity guide:
  critical = security hole or crash, must fix immediately
  high     = significant bug or performance problem
  medium   = notable issue, should fix
  low      = minor improvement
  info     = informational note

Code to review:
```python
{code}
```"""

    try:
        result = await _run_claude_cli(prompt)
        data = _extract_json(result)
        return {"agent": agent_type, **data}
    except Exception as exc:
        return {
            "agent": agent_type,
            "issues": [],
            "summary": f"Agent error: {exc}",
            "score": 5.0,
        }


# ── Supervisor agent ─────────────────────────────────────────────────────────


def _format_reports_for_supervisor(agent_reports: list[dict]) -> str:
    sections = []
    for r in agent_reports:
        issues_text = "\n".join(
            f"  - [{i['severity'].upper()}] {i['title']}: {i['description']}"
            for i in r.get("issues", [])
        ) or "  (none)"
        sections.append(
            f"### {r['agent'].upper()} AGENT  (score {r['score']}/10)\n"
            f"{r['summary']}\n"
            f"{issues_text}"
        )
    return "\n\n".join(sections)


async def run_supervisor(agent_reports: list[dict]) -> dict:
    """
    Supervisor agent: synthesizes all specialist reports into a final verdict.
    Demonstrates the Supervisor multi-agent pattern.
    """
    reports_text = _format_reports_for_supervisor(agent_reports)

    prompt = f"""You are a principal engineer making the final merge decision.
Synthesize the specialist reports below into a final verdict.

Rules:
- BLOCK only when critical security or crash risks are present.
- NEEDS_WORK when significant issues exist but no critical ones.
- APPROVE when only cosmetic or low-severity issues remain.

Respond ONLY with a valid JSON object matching this schema (no extra text):

{VERDICT_SCHEMA}

Specialist reports:

{reports_text}"""

    try:
        result = await _run_claude_cli(prompt)
        return _extract_json(result)
    except Exception as exc:
        return {
            "verdict": "NEEDS_WORK",
            "overall_score": 5.0,
            "critical_count": 0,
            "high_count": 0,
            "top_issues": [],
            "recommendation": f"Manual review required (supervisor error: {exc})",
        }
