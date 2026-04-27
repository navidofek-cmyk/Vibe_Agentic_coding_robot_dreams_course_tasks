"""
Specialized review agents and supervisor agent — OpenAI edition.

Each agent calls the OpenAI Chat Completions API with function calling
so structured JSON is returned directly, without post-processing.
"""

from __future__ import annotations

import asyncio
import json

from openai import AsyncOpenAI

client = AsyncOpenAI()          # reads OPENAI_API_KEY from environment
MODEL = "gpt-4o-mini"

# ── Tool (function) schemas ──────────────────────────────────────────────────

REPORT_FUNCTION = {
    "name": "report_findings",
    "description": "Report findings from a focused code review.",
    "parameters": {
        "type": "object",
        "properties": {
            "issues": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "severity": {
                            "type": "string",
                            "enum": ["critical", "high", "medium", "low", "info"],
                        },
                        "title":       {"type": "string"},
                        "description": {"type": "string"},
                        "suggestion":  {"type": "string"},
                    },
                    "required": ["severity", "title", "description", "suggestion"],
                },
            },
            "summary": {"type": "string"},
            "score": {
                "type": "number",
                "description": "Code quality score 0–10 (higher is better).",
            },
        },
        "required": ["issues", "summary", "score"],
    },
}

VERDICT_FUNCTION = {
    "name": "final_verdict",
    "description": "Synthesize all specialist reports into a final verdict.",
    "parameters": {
        "type": "object",
        "properties": {
            "verdict": {
                "type": "string",
                "enum": ["APPROVED", "NEEDS_WORK", "BLOCKED"],
            },
            "overall_score":  {"type": "number"},
            "critical_count": {"type": "integer"},
            "high_count":     {"type": "integer"},
            "top_issues": {
                "type": "array",
                "items": {"type": "string"},
                "description": "Up to 5 most important issues across all reports.",
            },
            "recommendation": {"type": "string"},
        },
        "required": [
            "verdict", "overall_score", "critical_count",
            "high_count", "top_issues", "recommendation",
        ],
    },
}

# ── Specialized agent system prompts ────────────────────────────────────────

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


# ── Specialized agents ───────────────────────────────────────────────────────


async def run_specialized_agent(agent_type: str, code: str, filename: str) -> dict:
    """Run one specialized review agent and return its structured report."""
    response = await client.chat.completions.create(
        model=MODEL,
        messages=[
            {"role": "system", "content": AGENT_SYSTEMS[agent_type]},
            {
                "role": "user",
                "content": (
                    f"Review the code from file '{filename}':\n\n"
                    f"```python\n{code}\n```\n\n"
                    "Call report_findings with your findings."
                ),
            },
        ],
        tools=[{"type": "function", "function": REPORT_FUNCTION}],
        tool_choice={"type": "function", "function": {"name": "report_findings"}},
    )

    tool_call = response.choices[0].message.tool_calls[0]
    data = json.loads(tool_call.function.arguments)
    return {"agent": agent_type, **data}


# ── Supervisor agent ─────────────────────────────────────────────────────────


def _format_reports(agent_reports: list[dict]) -> str:
    sections = []
    for r in agent_reports:
        issues_text = "\n".join(
            f"  - [{i['severity'].upper()}] {i['title']}: {i['description']}"
            for i in r.get("issues", [])
        ) or "  (none)"
        sections.append(
            f"### {r['agent'].upper()} AGENT  (score {r['score']}/10)\n"
            f"{r['summary']}\n{issues_text}"
        )
    return "\n\n".join(sections)


async def run_supervisor(agent_reports: list[dict]) -> dict:
    """
    Supervisor agent: synthesizes specialist reports into the final verdict.
    Demonstrates the Supervisor multi-agent pattern.
    """
    reports_text = _format_reports(agent_reports)

    response = await client.chat.completions.create(
        model=MODEL,
        messages=[
            {
                "role": "system",
                "content": (
                    "You are a principal engineer making the final merge decision. "
                    "BLOCK only when critical security or crash risks are present. "
                    "NEEDS_WORK for significant but fixable issues. "
                    "APPROVED when only minor/cosmetic issues remain."
                ),
            },
            {
                "role": "user",
                "content": (
                    f"Specialist review reports:\n\n{reports_text}\n\n"
                    "Call final_verdict with your decision."
                ),
            },
        ],
        tools=[{"type": "function", "function": VERDICT_FUNCTION}],
        tool_choice={"type": "function", "function": {"name": "final_verdict"}},
    )

    tool_call = response.choices[0].message.tool_calls[0]
    return json.loads(tool_call.function.arguments)
