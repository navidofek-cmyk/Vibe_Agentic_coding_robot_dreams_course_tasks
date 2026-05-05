"""
Security Sub-Swarm — 5 autonomních checkerů (Swarm pattern).

Každý checker je nezávislý agent hledající jednu kategorii zranitelností.
Agenti nekomunikují navzájem — čistý Swarm bez koordinace.
"""

from __future__ import annotations

import asyncio
import os

from claude_agent_sdk import (
    AgentDefinition,
    AssistantMessage,
    ClaudeAgentOptions,
    ClaudeSDKClient,
    ResultMessage,
    TextBlock,
)

MODEL = "claude-sonnet-4-6"
MAX_BUDGET_PER_CHECKER_USD = 0.15   # pojistka na jedno API volání checkeru
SWARM_STAGGER_SECONDS = 1.5         # zpoždění mezi spuštěním checkerů (proti rate limitingu)

# ---------------------------------------------------------------------------
# System prompty — každý checker má úzký fokus
# ---------------------------------------------------------------------------

_SQL_SYSTEM = """\
Jsi specialista na SQL injection. Načti zadaný soubor nástrojem Read a hledej výhradně
SQL injection zranitelnosti.

## Co hledat
- f-stringy přímo v SQL dotazech: f"SELECT ... {user_input} ..."
- %-formátování v SQL: "SELECT ... %s" % variable (bez parametrizace)
- .format() v SQL: "SELECT ... {}".format(variable)
- Přímá konkatenace řetězců do SQL dotazů

## Ignoruj
Vše ostatní — hashování, pickle, hesla, kvalitu kódu. Pouze SQL injection.

## Formát každého nálezu
**Závažnost:** KRITICKÁ
**Název:** SQL Injection v <název_funkce>()
**Řádek(y):** číslo
**Důkaz:** citace kódu
**Oprava:** parametrizovaný dotaz s placeholderem (?, %s nebo :param)

Pokud nenajdeš žádnou SQL injection, napiš: "SQL Injection: žádné nálezy."
"""

_SECRETS_SYSTEM = """\
Jsi specialista na detekci hardcoded secrets. Načti zadaný soubor nástrojem Read
a hledej výhradně citlivá data zakódovaná přímo v kódu.

## Co hledat
- Hardcoded hesla: password = "tajne123", passwd = "...", pwd = "..."
- API klíče a tokeny: api_key = "sk-...", token = "Bearer ..."
- Connection stringy s credentials: "postgresql://user:pass@host/db"
- Hardcoded kryptografické klíče nebo IV

## Ignoruj
SQL dotazy, pickle, kvalitu kódu. Pouze hardcoded secrets.

## Formát každého nálezu
**Závažnost:** KRITICKÁ
**Název:** Hardcoded <typ_secretu>
**Řádek(y):** číslo
**Důkaz:** citace kódu (zkrať hodnotu: "tajn...")
**Oprava:** použij os.environ nebo python-dotenv

Pokud nenajdeš žádné secrets, napiš: "Hardcoded Secrets: žádné nálezy."
"""

_DESERIALIZATION_SYSTEM = """\
Jsi specialista na nebezpečnou deserializaci. Načti zadaný soubor nástrojem Read
a hledej výhradně zranitelnosti v deserializaci dat.

## Co hledat (OWASP A08)
- pickle.loads() nebo pickle.load() — umožňuje RCE při nedůvěryhodném vstupu
- yaml.load() bez explicitního Loader=yaml.SafeLoader nebo Loader=yaml.FullLoader
- marshal.loads() s externími daty
- shelve otevřený na externích datech

## Ignoruj
SQL, secrets, hashování, kvalitu kódu. Pouze deserializace.

## Formát každého nálezu
**Závažnost:** KRITICKÁ
**Název:** Nebezpečná deserializace — <funkce>
**Řádek(y):** číslo
**Důkaz:** citace kódu
**Oprava:** bezpečná alternativa (yaml.safe_load, json místo pickle, ...)

Pokud nenajdeš žádnou nebezpečnou deserializaci, napiš: "Deserializace: žádné nálezy."
"""

_PATH_TRAVERSAL_SYSTEM = """\
Jsi specialista na path traversal útoky. Načti zadaný soubor nástrojem Read
a hledej výhradně zranitelnosti umožňující přístup mimo zamýšlený adresář.

## Co hledat
- open() volaný přímo s uživatelským vstupem bez normalizace cesty
- os.path.join() s uživatelským vstupem bez ověření, že výsledná cesta je v povoleném adresáři
- Chybějící os.path.realpath() nebo Path.resolve() před otevřením souboru
- Chybějící kontrola, že cesta začíná povoleným prefixem (str.startswith())

## Ignoruj
SQL, pickle, secrets, hashování. Pouze path traversal.

## Formát každého nálezu
**Závažnost:** VYSOKÁ
**Název:** Path Traversal v <funkce>()
**Řádek(y):** číslo
**Důkaz:** citace kódu
**Oprava:** os.path.realpath() + ověření prefixu nebo pathlib s .resolve()

Pokud nenajdeš žádný path traversal, napiš: "Path Traversal: žádné nálezy."
"""

_AUTH_SYSTEM = """\
Jsi specialista na autentizaci a autorizaci. Načti zadaný soubor nástrojem Read
a hledej výhradně slabé hashování hesel a chybějící autorizaci.

## Co hledat

### Slabé hashování hesel (OWASP A02)
- hashlib.md5(), hashlib.sha1(), hashlib.sha256() použité pro hashování hesel
  (tyto funkce jsou rychlé — pro hesla použij bcrypt, argon2 nebo PBKDF2)
- Přímé porovnání hashů pomocí == místo hmac.compare_digest() — timing attack

### Chybějící autorizace (OWASP A01)
- Operace mazání nebo úpravy dat volané bez předchozí kontroly oprávnění
- Funkce přijímající user_id nebo podobný parametr bez ověření, zda volající má právo

## Ignoruj
SQL, pickle, secrets, path traversal. Pouze autentizace/autorizace.

## Formát každého nálezu
**Závažnost:** VYSOKÁ | KRITICKÁ
**Název:** popis problému
**Řádek(y):** číslo
**Důkaz:** citace kódu
**Oprava:** konkrétní bezpečná alternativa

Pokud nenajdeš žádné problémy, napiš: "Auth/Autorizace: žádné nálezy."
"""

# AgentDefinition pro každý checker
_SQL_AGENT = AgentDefinition(
    description="SQL injection specialist — hledá f-stringy, %-formát a .format() v SQL dotazech",
    prompt=_SQL_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_SECRETS_AGENT = AgentDefinition(
    description="Secrets specialist — hledá hardcoded hesla, API klíče a connection stringy",
    prompt=_SECRETS_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_DESERIALIZATION_AGENT = AgentDefinition(
    description="Deserialization specialist — hledá pickle.loads(), yaml.load() bez SafeLoader",
    prompt=_DESERIALIZATION_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_PATH_TRAVERSAL_AGENT = AgentDefinition(
    description="Path traversal specialist — hledá open() s uživatelským vstupem bez normalizace cesty",
    prompt=_PATH_TRAVERSAL_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_AUTH_AGENT = AgentDefinition(
    description="Auth specialist — hledá slabé hashování hesel (MD5/SHA1) a chybějící autorizaci",
    prompt=_AUTH_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

# Mapa jméno → AgentDefinition (pořadí určuje pořadí sekcí v reportu)
CHECKERS: dict[str, AgentDefinition] = {
    "SQL Injection":     _SQL_AGENT,
    "Hardcoded Secrets": _SECRETS_AGENT,
    "Deserializace":     _DESERIALIZATION_AGENT,
    "Path Traversal":    _PATH_TRAVERSAL_AGENT,
    "Auth & Autorizace": _AUTH_AGENT,
}


# ---------------------------------------------------------------------------
# Swarm logika
# ---------------------------------------------------------------------------

def _sdk_env() -> dict[str, str]:
    env = {}
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if api_key:
        env["ANTHROPIC_API_KEY"] = api_key
    return env


async def _run_checker(name: str, agent_def: AgentDefinition, task: str, cwd: str) -> str:
    """Spustí jednoho security checkera a vrátí jeho výstup."""
    options = ClaudeAgentOptions(
        system_prompt=agent_def.prompt,
        allowed_tools=agent_def.tools or [],
        max_turns=3,
        model=agent_def.model,
        cwd=cwd,
        permission_mode="acceptEdits",
        max_budget_usd=MAX_BUDGET_PER_CHECKER_USD,
        env=_sdk_env(),
    )
    parts: list[str] = []
    async with ClaudeSDKClient(options=options) as client:
        await client.query(task)
        async for message in client.receive_response():
            if isinstance(message, AssistantMessage):
                for block in message.content:
                    if isinstance(block, TextBlock):
                        parts.append(block.text)
            elif isinstance(message, ResultMessage):
                if message.total_cost_usd and message.total_cost_usd > 0:
                    print(f"   💰 {name}: ${message.total_cost_usd:.4f} ({message.duration_ms}ms)")
    return "\n".join(parts) or f"[{name}: žádný výstup]"


async def run_security_swarm(task: str, cwd: str) -> str:
    """
    Spustí všechny security checkery jako Swarm — paralelně, bez vzájemné koordinace.
    Každý checker autonomně analyzuje jeden typ zranitelnosti.
    Checkery jsou staggeovány (SWARM_STAGGER_SECONDS) aby se snížil rate limiting.
    Vrátí agregovaný text se sekcí pro každý checker.
    """
    checker_tasks = []
    for i, (name, agent_def) in enumerate(CHECKERS.items()):
        if i > 0:
            await asyncio.sleep(SWARM_STAGGER_SECONDS)
        checker_tasks.append(asyncio.create_task(_run_checker(name, agent_def, task, cwd)))
    results = await asyncio.gather(*checker_tasks)

    sections = [
        f"#### {name}\n{result}"
        for name, result in zip(CHECKERS.keys(), results)
    ]
    return "\n\n".join(sections)
