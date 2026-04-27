"""
Code Review Pipeline — orchestration layer.

Demonstrates four workflow patterns in one pipeline:

  Stage 1  (Sequential)   Load and validate input files.
  Stage 2  (Parallel)     Run 4 specialized agents concurrently with asyncio.gather.
  Stage 3  (Supervisor)   Supervisor agent synthesizes all specialist reports.
  Stage 4  (Conditional)  Emit different output / exit code based on verdict severity.
"""

from __future__ import annotations

import asyncio
from pathlib import Path

from agents import AGENT_SYSTEMS, run_specialized_agent, run_supervisor

SEVERITY_ICON = {
    "critical": "🔴",
    "high": "🟠",
    "medium": "🟡",
    "low": "🟢",
    "info": "ℹ️ ",
}

VERDICT_STYLES = {
    "APPROVED":   ("✅", "APPROVED — code meets quality standards, ready to merge."),
    "NEEDS_WORK": ("⚠️ ", "NEEDS WORK — address the issues above before merging."),
    "BLOCKED":    ("🚫", "BLOCKED — critical issues found, merge is not allowed."),
}


class CodeReviewPipeline:
    """
    Orchestrates the full multi-agent review and returns a (report_str, exit_code) tuple.

    exit_code: 0 = approved, 1 = needs work, 2 = blocked
    """

    AGENT_TYPES = list(AGENT_SYSTEMS.keys())

    async def run(self, path: Path) -> tuple[str, int]:
        # ── Stage 1: Sequential — load input ────────────────────────────────
        code, filename = self._load(path)
        print(f"\n📂 Reviewing: {filename}  ({len(code.splitlines())} lines)\n")

        # ── Stage 2: Parallel — all specialists run concurrently ─────────────
        print("🔍 Parallel phase: launching specialized agents…")
        agent_reports: list[dict] = list(
            await asyncio.gather(
                *[
                    run_specialized_agent(agent_type, code, filename)
                    for agent_type in self.AGENT_TYPES
                ]
            )
        )
        print(f"   ↳ {len(agent_reports)} agents completed.\n")

        # ── Stage 3: Supervisor — synthesize reports ─────────────────────────
        print("🧠 Supervisor phase: synthesizing reports…")
        verdict = await run_supervisor(agent_reports)
        print("   ↳ Verdict ready.\n")

        # ── Stage 4: Conditional — format output based on verdict ─────────────
        report = self._build_report(filename, agent_reports, verdict)
        exit_code = {"APPROVED": 0, "NEEDS_WORK": 1, "BLOCKED": 2}.get(
            verdict["verdict"], 1
        )
        return report, exit_code

    # ── Private helpers ──────────────────────────────────────────────────────

    def _load(self, path: Path) -> tuple[str, str]:
        if path.is_file():
            return path.read_text(encoding="utf-8"), path.name

        py_files = sorted(path.rglob("*.py"))
        if not py_files:
            raise ValueError(f"No Python files found under {path}")
        combined = "\n\n# " + "─" * 60 + "\n\n".join(
            f"# File: {f.relative_to(path)}\n{f.read_text(encoding='utf-8')}"
            for f in py_files
        )
        return combined, path.name + "/"

    def _build_report(
        self, filename: str, agent_reports: list[dict], verdict: dict
    ) -> str:
        icon, headline = VERDICT_STYLES.get(
            verdict["verdict"], ("❓", verdict["verdict"])
        )

        lines: list[str] = [
            "",
            "═" * 62,
            f"  CODE REVIEW REPORT  ·  {filename}",
            "═" * 62,
            "",
            f"  {icon}  {headline}",
            "",
            f"  Overall score : {verdict['overall_score']:.1f} / 10",
            f"  Critical      : {verdict['critical_count']}",
            f"  High          : {verdict['high_count']}",
            "",
            f"  Recommendation:",
            f"  {verdict['recommendation']}",
        ]

        if verdict.get("top_issues"):
            lines += ["", "  Top issues:"]
            for issue in verdict["top_issues"]:
                lines.append(f"    • {issue}")

        # ── Conditional branch: extra warning for blocked reviews ────────────
        if verdict["verdict"] == "BLOCKED":
            lines += [
                "",
                "  ┌─ ACTION REQUIRED ──────────────────────────────────────┐",
                "  │  Fix all CRITICAL issues before requesting another      │",
                "  │  review. Do NOT merge this branch.                      │",
                "  └────────────────────────────────────────────────────────┘",
            ]

        lines += ["", "─" * 62, "  DETAILED FINDINGS BY AGENT", "─" * 62]

        for r in agent_reports:
            lines += [
                "",
                f"  🤖  {r['agent'].upper()} AGENT   score {r['score']}/10",
                f"      {r['summary']}",
            ]
            for issue in r.get("issues", []):
                sev = issue["severity"]
                ico = SEVERITY_ICON.get(sev, "•")
                lines += [
                    f"",
                    f"      {ico} [{sev.upper()}]  {issue['title']}",
                    f"         {issue['description']}",
                    f"         💡 {issue['suggestion']}",
                ]
            if not r.get("issues"):
                lines.append("      (no issues found)")

        lines += ["", "═" * 62, ""]
        return "\n".join(lines)
