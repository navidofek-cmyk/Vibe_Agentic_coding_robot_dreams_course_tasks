## Shrnutí

Soubor `examples/buggy_app.py` představuje kriticky zranitelný kód, který je **v současném stavu absolutně nevhodný k nasazení**. Obsahuje 7 bezpečnostních zranitelností kategorie KRITICKÁ/VYSOKÁ zahrnujících SQL injection na 4 místech, Remote Code Execution přes `pickle.loads()`, path traversal a hardcoded secrets. Kromě bezpečnostních problémů trpí kód systémickými kvalitativními nedostatky — resource leaky, porušením SRP a absencí type hints — které prohlubují útočnou plochu a znemožňují spolehlivé testování.

---

## Bezpečnostní nálezy

### 🔴 SQL Injection — 4 kritická místa (řádky 20, 50–52, 67–69, 83)

Uživatelský vstup je vkládán přímo do SQL dotazů přes f-stringy bez jakékoliv sanitizace. Jedná se o OWASP Top 1. Payload `' OR '1'='1` obejde autentizaci, `'; DROP TABLE users; --` smaže databázi.

**Dotčené funkce:** `get_user()`, `process_order()`, `UserManager.create_user()`, `UserManager.delete_user()`

```python
# Příklad zranitelného kódu (řádek 20):
cursor.execute(f"SELECT * FROM users WHERE username = '{username}'")

# Oprava — jednotná pro všechna 4 místa:
cursor.execute("SELECT * FROM users WHERE username = ?", (username,))
```

---

### 🔴 Remote Code Execution — `pickle.loads()` (řádek 31)

`load_session()` deserializuje libovolná data bez ověření zdroje nebo podpisu. Útočník dodá speciálně sestavený payload a spustí libovolný OS příkaz na serveru.

```python
# Oprava:
import json
def load_session(data: bytes) -> dict:
    return json.loads(data.decode())
```

---

### 🔴 Hardcoded Secrets (řádky 12–13)

```python
SECRET_KEY = "super-tajny-klic-1234"
DB_PASSWORD = "admin123"
```

Tajné klíče jsou viditelné každému s přístupem k repozitáři včetně git history. Rotace vyžaduje nasazení nové verze kódu.

```python
# Oprava:
SECRET_KEY = os.environ["SECRET_KEY"]
DB_PASSWORD = os.environ["DB_PASSWORD"]
```

---

### 🔴 MD5 hashování hesel bez soli (řádek 26)

MD5 je kryptograficky prolomené a bez soli produkuje determinické hashe — identická hesla mají identický hash, útočník použije rainbow tables. Na moderním GPU se MD5 hash prolomí v sekundách.

```python
# Oprava — použít algoritmus navržený pro hesla:
import bcrypt
return bcrypt.hashpw(password.encode(), bcrypt.gensalt())
```

---

### 🔴 Path Traversal — `read_file()` (řádky 34–37)

`filename` je vložen do cesty bez normalizace. Vstup `../../etc/passwd` přečte systémové soubory.

```python
# Oprava:
UPLOAD_DIR = Path("/var/uploads").resolve()
def read_file(filename: str) -> str:
    safe_path = (UPLOAD_DIR / filename).resolve()
    if not str(safe_path).startswith(str(UPLOAD_DIR) + os.sep):
        raise ValueError(f"Přístup mimo povolený adresář: {filename!r}")
    with open(safe_path) as f:
        return f.read()
```

---

### 🟠 Timing attack při porovnávání hashů (řádek 75)

Přímé `==` porovnání ukončí smyčku po první neshodě — útočník měří dobu odezvy a rekonstruuje hash.

```python
# Oprava:
if user and hmac.compare_digest(user[2], hash_password(password)):
```

---

### 🟠 `delete_user()` bez autorizační kontroly (řádky 79–84)

Kdokoliv s přístupem k instanci `UserManager` může smazat libovolný účet. Chybí ověření role (admin) nebo identity (self-delete).

---

### 🟠 IDOR v `process_order()` (řádky 40–54)

Funkce slepě důvěřuje předanému `user_id` — klasický Insecure Direct Object Reference. Útočník může vytvořit objednávku jménem libovolného uživatele.

---

### 🟠 Data Exposure — `get_all_users()` (řádek 88)

Metoda vrací celý slovník uživatelů včetně hashovaných hesel. Zbytečně rozšiřuje útočnou plochu — volající kód dostane víc, než potřebuje.

```python
# Oprava — vrátit pouze bezpečná pole:
return [{"email": u["email"]} for u in self.users.values()]
```

---

## Kvalita kódu

> **Propojení s bezpečností:** Níže popsané kvalitativní problémy přímo zvyšují riziko bezpečnostních chyb — chybějící validace vstupu usnadňuje injection, tight coupling znemožňuje bezpečné unit testy, resource leaky mohou vést k DoS.

### Resource leaky — neuzavřená DB spojení (řádky 18–21, 47–54)

`get_user()` a `process_order()` otevírají nové DB spojení, které při výjimce (ale i za normálního běhu) nikdy nezavřou. Konekce se hromadí, dokud OS nezačne odmítat nová připojení.

```python
# Oprava — context manager nebo try/finally:
def get_user(username: str) -> tuple | None:
    conn = sqlite3.connect("users.db")
    try:
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM users WHERE username = ?", (username,))
        return cursor.fetchone()
    finally:
        conn.close()
```

---

### `UserManager` porušuje Single Responsibility Principle (řádky 57–88)

Třída kombinuje 4 zodpovědnosti: in-memory cache, DB persistence, autentizaci a hashování. Každá zodpovědnost ztěžuje testování ostatních a každá změna (jiné hashování, Redis cache) zasáhne do jedné velké třídy.

**Dopad na bezpečnost:** SRP porušení znemožňuje izolovat autentizační logiku a testovat ji bez skutečné DB, čímž bezpečnostní regrese snáze uniknou.

**Návrh rozkladu:**
```python
class UserRepository:     # DB persistence
class AuthService:        # autentizace a hashování
```

---

### Hardcoded DB závislost — tight coupling (řádek 60)

`sqlite3.connect("users.db")` přímo v `__init__` znemožňuje unit testy bez skutečné DB a porušuje Dependency Inversion.

```python
# Oprava — Dependency Injection:
class UserManager:
    def __init__(self, db: sqlite3.Connection) -> None:
        self.db = db
```

---

### Funkce s 10 parametry — `process_order()` (řádek 40)

Pozicinální parametry jsou záměna náchylné a API je nečitelné. Funkce míchá výpočet ceny a DB persistence (dvě zodpovědnosti).

```python
# Oprava — dataclass + oddělení zodpovědností:
@dataclass
class OrderRequest:
    user_id: str; product_id: str; quantity: int; price: float
    discount: float = 0.0; tax: float = 0.0; shipping: float = 0.0
    # ...

def calculate_total(order: OrderRequest) -> float: ...
def save_order(order: OrderRequest, db: sqlite3.Connection) -> float: ...
```

---

### `delete_user()` — nekonzistentní chování při chybě (řádek 81)

`del self.users[username]` vyhodí `KeyError` před DB operací — stav paměti a DB se mohou rozejít. Typ výjimky není dokumentovaný součástí API.

---

### Chybějící type hints — 9 veřejných funkcí

Žádná funkce nemá anotace. Mypy ani IDE nemohou odhalit chyby, refaktoring je nebezpečný.

```python
# Příklady správných anotací:
def get_user(username: str) -> tuple | None: ...
def load_session(data: bytes) -> object: ...
def hash_password(password: str) -> str: ...
```

---

### DRY porušení — duplikované DB připojení (řádky 18, 47, 60)

`sqlite3.connect("users.db")` a `sqlite3.connect("orders.db")` se opakují. Při přechodu na connection pool nebo jiné DB je nutná změna na více místech.

---

### Chybějící docstringy — všechny veřejné funkce

`help()`, Sphinx a IDE tooltips vrátí prázdné výsledky. Záměr funkcí, parametry a výjimky nejsou zdokumentovány.

---

## Doporučení pro testy

Tests Agent navrhl **67 testů** rozdělených do 11 testovacích tříd s **19 regresními testy** explicitně dokumentujícími existující bugy.

### Souhrnná tabulka testů

| Funkce / Třída | Počet testů | Regresní (BUG) | Priorita |
|---|---|---|---|
| `load_session` | 7 | 1 (RCE) | 🔴 Kritická |
| `read_file` | 5 | 2 (path traversal) | 🔴 Kritická |
| `get_user` | 7 | 2 (SQL injection) | 🔴 Kritická |
| `process_order` | 10 | 2 (SQL injection) | 🔴 Kritická |
| `UserManager.create_user` | 8 | 2 (SQL injection, prázdný username) | 🔴 Kritická |
| `UserManager.authenticate` | 6 | 2 (DB error leak, SQL injection) | 🔴 Kritická |
| `UserManager.delete_user` | 5 | 3 (KeyError, SQL injection, chybějící autorizace) | 🔴 Kritická |
| `UserManager.get_all_users` | 5 | 1 (data exposure) | 🔴 Kritická |
| Hardcoded secrets | 2 | 2 | 🔴 Kritická |
| `hash_password` | 9 | 2 (MD5, bez soli) | 🟠 Vysoká |
| `UserManager.__init__` | 3 | 0 | 🟡 Střední |
| **CELKEM** | **67** | **19** | — |

### Klíčové poznámky k testovací strategii

- **Regresní testy jsou záměrně "červené"** — dokumentují existující bugy a po opravě by měly začít **selhávat** (testy ověřují bugové chování). Po opravě budu nutné je přepsat na ověření správného chování.
- **Fixtures** (`mock_db_conn`, `user_manager`) jsou navrženy pro izolaci od skutečné SQLite DB — unit testy nebudou vyžadovat soubor `users.db`.
- **RCE test** v `TestLoadSession` je funkční proof-of-concept — spustí se na serveru stejně jako útočníkův payload.
- Celkové pokrytí navrženými testy: ~95 % řádků souboru `buggy_app.py`.

---

## Prioritizovaný akční plán

- 🔴 **Okamžitě** (blokuje nasazení):
  - Nahradit všechna 4 SQL f-string sestavení parametrizovanými dotazy (`?` placeholder)
  - Přesunout `SECRET_KEY` a `DB_PASSWORD` do proměnných prostředí (`.env` + python-dotenv, přidat do `.gitignore`)
  - Nahradit `pickle.loads()` za `json.loads()` v `load_session()`
  - Opravit path traversal v `read_file()` — přidat `Path.resolve()` + kontrolu prefixu
  - Nahradit MD5 za `bcrypt` nebo `hashlib.scrypt` v `hash_password()`
  - Přidat `hmac.compare_digest()` pro porovnávání hashů v `authenticate()`
  - Přidat autorizační kontrolu do `delete_user()` (admin role nebo self-delete)
  - Odebrat `password` z výstupu `get_all_users()`
  - Uzavřít DB spojení v `get_user()` a `process_order()` pomocí `try/finally`

- 🟠 **Tento sprint** (důležité):
  - Implementovat Dependency Injection pro `UserManager` — předávat `db: sqlite3.Connection` v konstruktoru
  - Zavést IDOR ochranu v `process_order()` — ověřit identitu volajícího vůči `user_id`
  - Rozložit `UserManager` na `UserRepository` + `AuthService` (SRP)
  - Refaktorovat `process_order()` — zavést `OrderRequest` dataclass, oddělit `calculate_total()` od `save_order()`
  - Opravit `delete_user()` — ošetřit `KeyError`, atomizovat operaci (nejdřív DB commit, pak paměť)
  - Doplnit type hints ke všem 9 veřejným funkcím a metodám
  - Napsat a spustit navrhovaných 67 testů (se zaměřením na 🔴 kritické třídy)

- 🟡 **Backlog** (zlepšení kvality):
  - Centralizovat DB konfiguraci — konstanty `DB_USERS`, `DB_ORDERS` nebo factory funkce
  - Doplnit docstringy ke všem veřejným funkcím a třídě `UserManager`
  - Přidat `mypy` do CI pipeline s `strict` konfigurací

---

## Celkové hodnocení

| Dimenze | Skóre | Odůvodnění |
|---|---|---|
| **Bezpečnost** | 1 / 10 | 3× KRITICKÁ zranitelnost (RCE, SQL injection ×4, path traversal), hardcoded secrets, MD5 bez soli, IDOR, chybějící autorizace |
| **Kvalita** | 3 / 10 | Resource leaky, SRP porušení, tight coupling, 10-param funkce, nekonzistentní error handling |
| **Testovatelnost** | 2 / 10 | Hardcoded DB dependency znemožňuje unit testy bez monkey-patchingu, žádná DI, žádné existující testy |
| **Čitelnost** | 3 / 10 | Žádné type hints, žádné docstringy, komentáře popisují bugy místo záměru, magická čísla v SQL |
| **Celkové skóre** | **2,25 / 10** | Průměr dimenzí |

> **🚫 Neschválit.** Kód obsahuje kritické bezpečnostní zranitelnosti (RCE, SQL injection, path traversal), které jej diskvalifikují z nasazení v jakémkoliv prostředí — opravit minimálně všechny body z kategorie 🔴 Okamžitě před dalším code review.