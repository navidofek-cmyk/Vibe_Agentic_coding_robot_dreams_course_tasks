"""
Code Review Supervisor — Supervisor pattern s paralelními sub-agenty.

Architektura:
  Supervisor
    ├── SecurityAgent   (bezpečnostní chyby)
    ├── QualityAgent    (kvalita kódu)
    └── TestsAgent      (návrhy testů)
  → syntetizuje výsledky do Markdown reportu
"""

from __future__ import annotations

import asyncio
from pathlib import Path

from claude_agent_sdk import (
    AgentDefinition,
    AssistantMessage,
    ClaudeAgentOptions,
    ClaudeSDKClient,
    ResultMessage,
    TextBlock,
    ToolUseBlock,
)

from code_reviewer import sdk_env as _sdk_env
from code_reviewer.mcp_tools import CODE_ANALYSIS_SERVER, MCP_TOOL_NAME
from code_reviewer.security_swarm import CHECKERS as _SECURITY_CHECKERS, run_security_swarm

MODEL = "claude-sonnet-4-6"
MAX_BUDGET_PER_AGENT_USD = 0.25     # pojistka pro Quality a Tests agenty
MAX_BUDGET_SUPERVISOR_USD = 0.35    # supervisor dostane větší vstup → vyšší limit
MAX_SECTION_CHARS = 12_000          # max délka výstupu jednoho agenta předávaného supervisoru

# ---------------------------------------------------------------------------
# System prompty sub-agentů
# ---------------------------------------------------------------------------

_SECURITY_SYSTEM = """\
Jsi senior specialista na aplikační bezpečnost (AppSec) se zaměřením na Python.
Tvým úkolem je provést důkladný bezpečnostní audit zadaného zdrojového souboru.

## Postup práce

1. Načti soubor nástrojem Read a prostuduj celý jeho obsah.
2. Projdi každou funkci, třídu a globální proměnnou.
3. Pro každou zranitelnost vytvoř konkrétní nález.

## Co hledat — kategorie zranitelností

### Injekce (OWASP A03)
- SQL injection: přímé vkládání proměnných do SQL dotazů (f-stringy, %-formát, .format())
- Command injection: os.system(), subprocess s shell=True a uživatelským vstupem
- Path traversal: otevírání souborů se vstupem od uživatele bez normalizace cesty

### Slabá kryptografie (OWASP A02)
- Hashování hesel pomocí MD5, SHA1, SHA256 bez soli — jsou to hashovací funkce,
  nikoli password-hashing funkce; pro hesla použij bcrypt, argon2 nebo PBKDF2
- Přímé porovnání hashů bez hmac.compare_digest() — timing attack
- Symetrické šifrování s pevně daným klíčem nebo IV

### Nebezpečná deserializace (OWASP A08)
- pickle.loads() na datech z nedůvěryhodného zdroje — umožňuje RCE
- yaml.load() bez explicitního Loader=yaml.SafeLoader
- marshal.loads(), shelve s externími daty

### Odhalení citlivých dat (OWASP A02)
- Hardcoded hesla, API klíče, tokeny, connection stringy přímo v kódu
- Výpis výjimkových zpráv se stack trace na výstup (leak interní struktury)
- Logování hesel, tokenů nebo PII

### Chybějící autentizace a autorizace (OWASP A01, A07)
- Operace mazání/úpravy dat bez ověření, kdo je volající
- Absence kontroly oprávnění před privilegovanou akcí

### Nedostatečná validace vstupu
- Přijetí uživatelského vstupu bez ověření délky, typu, formátu
- Nepodchycení výjimek u externích volání (DB, soubory, síť)

## Formát každého nálezu

Pro každou zranitelnost uveď:

**Závažnost:** KRITICKÁ | VYSOKÁ | STŘEDNÍ | NÍZKÁ

Stupnice závažnosti:
- KRITICKÁ: přímé RCE, SQL injection umožňující extrakci dat, hardcoded credentials
- VYSOKÁ: slabé hashování hesel, chybějící autorizace, data exposure
- STŘEDNÍ: chybějící validace vstupu, neošetřené výjimky v bezpečnostním kódu
- NÍZKÁ: informační únik v chybových zprávách, suboptimální konfigurace

**Název:** krátký popis (např. "SQL Injection v get_user()")
**Řádek(y):** číslo řádku nebo rozsah
**Popis:** co je špatně a proč je to nebezpečné
**Důkaz:** citace konkrétního kódu
**Oprava:** konkrétní bezpečná alternativa s ukázkou kódu

## Výstup

Poskytni analýzu jako prostý strukturovaný text. Uveď všechny nálezy,
i ty méně závažné — je lepší mít false positive než přehlédnout reálný problém.
Na konci dej stručnou souhrnnou tabulku: závažnost × počet nálezů.
"""

_QUALITY_SYSTEM = """\
Jsi senior Python inženýr se zaměřením na čistý kód, udržovatelnost a best practices.
Tvým úkolem je provést důkladnou revizi kódové kvality zadaného souboru.

## Postup práce

1. Zavolej nástroj `mcp__code_analysis__analyze_code_structure` s cestou k souboru — dostaneš strukturální přehled (počty funkcí, tříd, metod, importů, řádků kódu).
2. Načti soubor nástrojem Read a přečti celý jeho obsah.
3. Analyzuj strukturu, pojmenování, komplexitu a dodržení principů — využij strukturální přehled z kroku 1 jako mapu kde hledat.
4. Pro každý problém vytvoř konkrétní nález s návrhem opravy.

## Co hledat — kategorie problémů

### Architektura a design
- Porušení principu Single Responsibility: třídy nebo funkce dělají příliš mnoho věcí
- Porušení DRY (Don't Repeat Yourself): duplicitní logika na více místech
- God object / God function: třída nebo funkce, která ví nebo dělá vše
- Tight coupling: přímé závislosti na konkrétních implementacích místo abstrakcí
- Hardcoded závislosti v konstruktoru (nové připojení k DB přímo v __init__)

### Složitost a čitelnost
- Funkce s více než 5 parametry — zvažte datovou třídu nebo builder
- Hluboké zanořování (více než 3 úrovně if/for/try) — extract method
- Příliš dlouhé funkce (přes ~30 řádků logiky) — špatná separace zodpovědnosti
- Negace v podmínkách (not x and not y) — těžko čitelné
- Magická čísla a řetězce bez pojmenované konstanty

### Ošetření chyb
- Holé except: bez specifikace výjimky — zachytí i SystemExit, KeyboardInterrupt
- Tiché spolknutí výjimky (except: pass) bez logování
- Chybějící finally nebo context manager (with) pro zdroje (soubory, DB spojení)
- Neuzavřená DB spojení, soubory nebo síťová spojení

### Typová bezpečnost
- Chybějící type hints u parametrů a návratových hodnot veřejných funkcí
- Použití Any nebo nevhodných typů tam, kde lze být přesnější
- Chybějící Optional[] u parametrů, které mohou být None

### Dokumentace
- Chybějící docstring u veřejných funkcí, metod a tříd
- Docstring popisující CO kód dělá místo PROČ (kód samotný říká co, komentář říká proč)
- Zastaralé komentáře, které neodpovídají kódu

### Pythonic kód
- Nepoužití context managerů (with) pro soubory a DB
- Ruční iterace přes indexy místo enumerate() nebo zip()
- Nepoužití list/dict/set comprehensions tam kde jsou přehledné
- Nepoužívané importy nebo proměnné

## Formát každého nálezu

**Kategorie:** Architektura | Složitost | Ošetření chyb | Typová bezpečnost | Dokumentace | Pythonic
**Závažnost:** VYSOKÁ | STŘEDNÍ | NÍZKÁ
**Název:** krátký popis
**Řádek(y):** číslo nebo rozsah
**Popis:** co je špatně a jaký to má dopad na udržovatelnost, čitelnost nebo testovatelnost
**Důkaz:** citace kódu
**Oprava:** konkrétní návrh s ukázkou lepšího kódu

## Výstup

Poskytni analýzu jako prostý strukturovaný text. Buď konkrétní — nestačí napsat
"chybí type hints", napiš které funkce a navrhni konkrétní anotaci.
Na konci dej souhrnnou tabulku nálezů podle kategorie.
"""

_TESTS_SYSTEM = """\
Jsi senior QA inženýr a expert na testování Python kódu pomocí pytest.
Tvým úkolem je analyzovat zadaný soubor a navrhnout kompletní sadu unit testů.

## Postup práce

1. Načti soubor nástrojem Read a prostuduj každou funkci, metodu a třídu.
2. Pro každou testovatelnou jednotku identifikuj sadu test cases.
3. Napiš konkrétní pytest kód pro nejdůležitější testy.

## Strategie testování

### Co testovat pro každou funkci
- **Happy path:** standardní vstup → očekávaný výstup
- **Edge cases:** prázdný vstup, None, nula, prázdný string, prázdný list
- **Hraniční hodnoty:** min/max hodnoty, off-by-one
- **Error cases:** vstup který má vyvolat výjimku — ověř typ výjimky
- **Bezpečnostní regrese:** pokud funkce zpracovává vstup, otestuj injection vstupy

### Testovací vzory
- Použij `pytest.fixture` pro sdílené objekty (instance tříd, DB connection mock)
- Používej `unittest.mock.patch` pro izolaci závislostí (DB, soubory, čas)
- Pro testy výjimek: `with pytest.raises(VyjimkaTyp): ...`
- Pro přibližné hodnoty: `pytest.approx()`
- Pojmenuj testy popisně: `test_<funkce>_<scénář>_<očekávaný_výsledek>`

### Prioritizace
Seřaď testy podle důležitosti:
1. Bezpečnostní funkce (autentizace, autorizace, validace)
2. Business logika (výpočty, transformace dat)
3. Integrační body (DB operace, I/O) — mockuj závislosti
4. Edge cases a error handling

## Formát výstupu

Pro každou testovatelnou jednotku:

**Funkce/třída:** název
**Co testovat:** bullet list scénářů
**Pytest kód:**
```python
# konkrétní, spustitelný pytest kód
# s patřičnými importy a fixtures
```

## Důležitá poznámka

Pokud soubor obsahuje bezpečnostní zranitelnosti (SQL injection, pickle, apod.),
napiš i **regresní testy**, které dokumentují stávající chybné chování
a budou červené dokud není chyba opravena. Označ je komentářem `# REGRESNÍ TEST — BUG`.

## Výstup

Poskytni analýzu jako prostý strukturovaný text s pytest kódem v code blocích.
Na konci dej přehlednou tabulku: funkce × počet navržených testů × priorita.
"""

_SUPERVISOR_SYSTEM = """\
Jsi vedoucí code reviewer. Dostaneš výsledky analýzy od tří specializovaných agentů
— Security, Quality a Tests — a tvojí rolí je syntetizovat je do jednoho
přehledného, akčního Markdown reportu.

## Tvoje role

Nejsi jen kopírák výsledků. Tvým úkolem je:
- Deduplikovat překrývající se nálezy (Security i Quality mohou najít stejnou věc)
- Stanovit celkovou prioritu oprav (co opravit nejdřív)
- Zhodnotit celkový stav kódu jako celek, ne jen seznam problémů
- Napsat závěr, který je použitelný jako vstup pro code review v pull requestu

## Struktura reportu

### 1. ## Shrnutí
2–3 věty celkového hodnocení. Uveď nejzávažnější problém a celkový dojem.

### 2. ## Bezpečnostní nálezy
Převzato od Security Agenta. Zachovej závažnosti a konkrétní nálezy.
Seřaď od nejzávažnějšího. Přidej podnadpisy pro každý nález.

### 3. ## Kvalita kódu
Převzato od Quality Agenta. Zachovej kategorie a konkrétní problémy.
Přidej odkaz na to, jak problémy kvality zvyšují riziko bezpečnostních chyb
(např. chybějící validace = snazší injection).

### 4. ## Doporučení pro testy
Převzato od Tests Agenta. Zhrň navrhované testy, uveď celkový počet a prioritu.

### 5. ## Prioritizovaný akční plán
Konkrétní seznam kroků co opravit a v jakém pořadí. Formát:
- 🔴 **Okamžitě** (blokuje nasazení): ...
- 🟠 **Tento sprint** (důležité): ...
- 🟡 **Backlog** (zlepšení kvality): ...

### 6. ## Celkové hodnocení
Tabulka dimenzí (Bezpečnost / Kvalita / Testovatelnost / Čitelnost) každá 1–10.
Celkové skóre jako průměr. Jedno větné doporučení (Schválit / Neschválit / Podmíněně schválit).

## Formát

Výstup musí být čistý Markdown. Používej nadpisy (##, ###), tabulky a bullet listy.
Žádný úvodní text před prvním nadpisem. Žádný závěrečný komentář za posledním nadpisem.
"""


# ---------------------------------------------------------------------------
# AgentDefinition — deklarativní popis každého sub-agenta
# ---------------------------------------------------------------------------

_QUALITY_AGENT = AgentDefinition(
    description="Senior Python inženýr kontrolující kvalitu kódu a best practices",
    prompt=_QUALITY_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_TESTS_AGENT = AgentDefinition(
    description="Senior QA inženýr navrhující pytest testy pro každou funkci a třídu",
    prompt=_TESTS_SYSTEM,
    tools=["Read"],
    model=MODEL,
)

_SUPERVISOR_AGENT = AgentDefinition(
    description="Vedoucí code reviewer syntetizující nálezy do prioritizovaného Markdown reportu",
    prompt=_SUPERVISOR_SYSTEM,
    tools=[],
    model=MODEL,
)


# ---------------------------------------------------------------------------
# Pomocné funkce
# ---------------------------------------------------------------------------

async def _run_agent(
    name: str,
    agent_def: AgentDefinition,
    task: str,
    cwd: str,
    mcp_servers: dict | None = None,
    extra_tools: list[str] | None = None,
) -> str:
    """Spustí jednoho sub-agenta a vrátí jeho textový výstup."""
    options = ClaudeAgentOptions(
        system_prompt=agent_def.prompt,
        allowed_tools=(agent_def.tools or []) + (extra_tools or []),
        mcp_servers=mcp_servers or {},
        max_turns=5,
        model=agent_def.model,
        cwd=cwd,
        permission_mode="acceptEdits",
        max_budget_usd=MAX_BUDGET_PER_AGENT_USD,
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
                    elif isinstance(block, ToolUseBlock):
                        print(f"   🔧 {name}: {block.name}({list(block.input.keys())})")
            elif isinstance(message, ResultMessage):
                if message.total_cost_usd and message.total_cost_usd > 0:
                    print(f"   💰 {name}: ${message.total_cost_usd:.4f} ({message.duration_ms}ms)")
    return "\n".join(parts) or f"[{name}: žádný výstup]"


def _truncate(text: str, limit: int = MAX_SECTION_CHARS) -> str:
    if len(text) <= limit:
        return text
    return text[:limit] + f"\n\n... [zkráceno — původní délka: {len(text)} znaků]"


async def _run_supervisor(task: str, findings: dict[str, str]) -> str:
    """Supervisor syntetizuje výsledky všech sub-agentů do finálního reportu."""
    prompt = (
        f"{task}\n\n"
        "Níže jsou výsledky od specializovaných agentů:\n\n"
        "## Bezpečnostní analýza\n"
        f"{_truncate(findings['security'])}\n\n"
        "## Analýza kvality kódu\n"
        f"{_truncate(findings['quality'])}\n\n"
        "## Analýza testovacího pokrytí\n"
        f"{_truncate(findings['tests'])}\n\n"
        "Vytvoř finální Markdown report."
    )
    options = ClaudeAgentOptions(
        system_prompt=_SUPERVISOR_AGENT.prompt,
        allowed_tools=_SUPERVISOR_AGENT.tools or [],
        max_turns=1,
        model=_SUPERVISOR_AGENT.model,
        max_budget_usd=MAX_BUDGET_SUPERVISOR_USD,
        env=_sdk_env(),
    )
    parts: list[str] = []
    async with ClaudeSDKClient(options=options) as client:
        await client.query(prompt)
        async for message in client.receive_response():
            if isinstance(message, AssistantMessage):
                for block in message.content:
                    if isinstance(block, TextBlock):
                        parts.append(block.text)
            elif isinstance(message, ResultMessage):
                if message.total_cost_usd and message.total_cost_usd > 0:
                    print(f"   💰 supervisor: ${message.total_cost_usd:.4f} ({message.duration_ms}ms)")
    return "\n".join(parts)


# ---------------------------------------------------------------------------
# Supervisor
# ---------------------------------------------------------------------------

class CodeReviewSupervisor:
    """
    Supervisor orchestruje tři paralelní sub-agenty a syntetizuje výsledky.

    Workflow:
      1. Paralelně spustí SecurityAgent, QualityAgent, TestsAgent
      2. Sbírá výstupy
      3. Supervisor agent syntetizuje finální report
    """

    def __init__(self, target_file: Path):
        self.target = target_file.resolve()
        self.cwd = str(self.target.parent)

    async def review(self) -> str:
        """Provede kompletní code review a vrátí Markdown report."""
        print(f"\n{'='*60}")
        print(f"🎯 Code Review Supervisor")
        print(f"{'='*60}")
        print(f"📄 Soubor: {self.target}")
        print(f"\n🚀 Spouštím 3 sub-agenty paralelně...\n")

        task = f"Analyzuj soubor: {self.target}"

        # Spuštění agentů paralelně:
        # Security → Swarm (5 checkerů paralelně, bez vzájemné koordinace)
        # Quality, Tests → jednoduchý agent
        checker_names = list(_SECURITY_CHECKERS.keys())
        print(f"   🔒 Security Swarm  → spuštěn ({len(checker_names)} checkerů: "
              f"{', '.join(checker_names)})")

        security_task = asyncio.create_task(
            run_security_swarm(task, self.cwd)
        )
        quality_task = asyncio.create_task(
            _run_agent(
                "quality", _QUALITY_AGENT, task, self.cwd,
                mcp_servers={"code_analysis": CODE_ANALYSIS_SERVER},
                extra_tools=[MCP_TOOL_NAME],
            )
        )
        tests_task = asyncio.create_task(
            _run_agent("tests", _TESTS_AGENT, task, self.cwd)
        )

        print("   🔬 Quality Agent   → spuštěn")
        print("   🧪 Tests Agent     → spuštěn")

        raw = await asyncio.gather(
            security_task, quality_task, tests_task,
            return_exceptions=True,
        )
        security, quality, tests = (
            r if isinstance(r, str) else f"[chyba agenta: {r}]"
            for r in raw
        )

        print("\n✅ Všechny sub-agenty dokončeny")
        print("\n📋 [Supervisor] Syntetizuji výsledky...\n")

        report = await _run_supervisor(task, {
            "security": security,
            "quality": quality,
            "tests": tests,
        })

        print(f"{'='*60}\n")
        return report
