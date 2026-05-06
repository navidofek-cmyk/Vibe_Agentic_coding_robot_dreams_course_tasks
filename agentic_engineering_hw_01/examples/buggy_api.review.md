## Shrnutí

`buggy_api.py` je FastAPI backend e-shopu s **kritickými bezpečnostními zranitelnostmi**, které v současném stavu zcela vylučují nasazení do produkce. Nejzávažnějšími problémy jsou dvě nezávislé cesty k Remote Code Execution (pickle deserializace) a SQL injection ve všech databázových dotazech. Kód působí dojmem funkčního řešení, ale autentizace je fakticky nefunkční — `verify_token()` vrací libovolná data připravená útočníkem a výsledek kontroly role je na čtyřech místech zcela ignorován.

---

## Bezpečnostní nálezy

### 🔴 [KRITICKÁ] SQL Injection — 13 zranitelných míst v 8 funkcích

**Řádky:** 144, 151–154, 165–167, 211, 228–231, 243–247, 291–293, 397–400

Každý SQL dotaz v souboru konstruuje query pomocí f-stringů s uživatelskými vstupy. Útočník může číst, modifikovat i mazat libovolná data. Zvláště nebezpečný je `ORDER BY {sort}` v `list_products()` (ř. 247) — nelze ošetřit parametrizací, nutný whitelist.

```python
# Zranitelné — ukázka ze tří funkcí
cursor.execute(f"SELECT id FROM users WHERE username = '{user.username}'")
cursor.execute(f"SELECT id, username, email FROM users WHERE username LIKE '%{query}%'")
cursor.execute(f"SELECT * FROM products ORDER BY {sort}")
```

```python
# Oprava
cursor.execute("SELECT id FROM users WHERE username = ?", (user.username,))
cursor.execute("SELECT id, username, email FROM users WHERE username LIKE ?", (f"%{query}%",))

ALLOWED_SORT = {"name", "price", "stock", "id"}
if sort not in ALLOWED_SORT:
    raise HTTPException(status_code=400, detail="Invalid sort column")
cursor.execute(f"SELECT * FROM products ORDER BY {sort}")  # bezpečné po whitelistu
```

---

### 🔴 [KRITICKÁ] Pickle RCE — 2 nezávislé vektory Remote Code Execution

**Řádky:** 127 (`verify_token`), 393 (`import_products`)

`verify_token()` volá `pickle.loads()` přímo na HTTP query parametr `token` — útočník odešle hex-enkódovaný pickle payload, který při deserializaci spustí libovolné OS příkazy. Stejná zranitelnost existuje v `import_products()`, kde se deserializuje obsah nahraného `.pkl` souboru. Tyto dvě zranitelnosti jsou na sobě nezávislé — každá sama o sobě umožňuje plné převzetí serveru.

```python
# Zranitelné
def verify_token(token: str) -> dict:
    return pickle.loads(bytes.fromhex(token))  # RCE

data = pickle.loads(content)  # obsah .pkl od útočníka → RCE
```

```python
# Oprava — tokeny
import jwt
def verify_token(token: str) -> dict:
    try:
        return jwt.decode(token, SECRET_KEY, algorithms=["HS256"])
    except jwt.InvalidTokenError as exc:
        raise HTTPException(status_code=401, detail="Invalid token") from exc

# Oprava — import souborů
if file.filename.endswith(".json"):
    data = json.loads(content)
elif file.filename.endswith(".csv"):
    data = list(csv.DictReader(io.StringIO(content.decode())))
else:
    raise HTTPException(status_code=400, detail="Unsupported format. Use .json or .csv")
```

---

### 🔴 [KRITICKÁ] Hardcoded Secrets — 4 credentials ve zdrojovém kódu

**Řádky:** 20–23

Čtyři produkční tajemství jsou jako stringové literály přímo v kódu. Stripe klíč má prefix `sk_live_` — jde o produkční klíč s okamžitým finančním dopadem při úniku. `DB_PASSWORD` (ř. 21) je navíc dead code — nikde dále se nepoužívá.

```python
SECRET_KEY = "my-super-secret-jwt-key-12345"   # JWT podpis
DB_PASSWORD = "admin123"                         # dead code, ale únik
STRIPE_API_KEY = "sk_live_abcdef123456789"       # PRODUKČNÍ Stripe klíč!
DATABASE_URL = "postgresql://admin:password123@prod-db.internal/shop"
```

```python
# Oprava
SECRET_KEY     = os.environ["SECRET_KEY"]       # bez defaultu — crash > únik
STRIPE_API_KEY = os.environ["STRIPE_API_KEY"]
DATABASE_URL   = os.environ["DATABASE_URL"]
```

---

### 🔴 [KRITICKÁ] MD5 pro hashování hesel — bez soli

**Řádek:** 122

MD5 je message digest, ne password hashing funkce. GPU crackery zvládají miliardy MD5 hashů za sekundu. Chybí sůl — stejné heslo u dvou uživatelů generuje identický hash (rainbow tables). Celá tabulka hesel je trivially crackable.

```python
def hash_password(password: str) -> str:
    return hashlib.md5(password.encode()).hexdigest()  # zranitelné
```

```python
import bcrypt
def hash_password(password: str) -> str:
    return bcrypt.hashpw(password.encode(), bcrypt.gensalt()).decode()
```

---

### 🟠 [VYSOKÁ] Command Injection — `shell=True` s uživatelskými daty

**Řádky:** 333, 360–365, 413–418

Tři volání `subprocess.run()` s `shell=True` vkládají uživatelský vstup přímo do shell stringu. V `generate_report()` parametr `format` z URL; útočník zadá `format='; rm -rf /; echo '` a smaže filesystem.

```python
subprocess.run(f"echo 'Order {order_id}: {order.notes}' | mail ...", shell=True)
subprocess.run(f"python3 reports/generate.py --format {format}", shell=True)
```

```python
ALLOWED_FORMATS = {"pdf", "csv", "json"}
if format not in ALLOWED_FORMATS:
    raise HTTPException(status_code=400, detail="Invalid format")
subprocess.run(["python3", "reports/generate.py", "--format", format], check=True)
```

---

### 🟠 [VYSOKÁ] Path Traversal — zápis i čtení mimo povolený adresář

**Řádky:** 385–388 (`import_products`), 423–426 (`get_file`)

`file.filename` z multipart uploadu bez sanitizace — útočník pojmenuje soubor `../../etc/cron.d/backdoor`. `get_file()` otevírá `/var/data/{filename}` přímo z URL; dotaz `/files/../../../../etc/shadow` vrátí systémové soubory.

```python
upload_path = f"/var/uploads/{file.filename}"   # traversal
open(f"/var/data/{filename}", "r")              # traversal
```

```python
UPLOAD_DIR = Path("/var/uploads").resolve()
def _safe_path(base: Path, name: str) -> Path:
    target = (base / name).resolve()
    if not str(target).startswith(str(base)):
        raise HTTPException(status_code=400, detail="Invalid filename")
    return target
```

---

### 🟠 [VYSOKÁ] Chybějící autorizace — mazání uživatelů, admin report

**Řádky:** 196–203, 207–214, 408–419

`delete_user()` nemá žádný autentizační parametr — kdokoli odešle DELETE request a smaže libovolného uživatele. `search_users()` nevyžaduje token. `generate_report()` token přijme a ověří, ale výslednou roli nikdy nekontroluje.

```python
@app.delete("/users/{user_id}")
def delete_user(user_id: int):  # žádný token!
    cursor.execute(f"DELETE FROM users WHERE id = {user_id}")
```

---

### 🔴 [KRITICKÁ] PCI DSS — číslo karty a CVV logováno do stdout

**Řádek:** 346

Číslo platební karty a CVV jsou tisknuty přes `print()`. V produkci jde výstup do log aggregátorů a SIEM systémů — přímé porušení PCI DSS s potenciálními smluvními a regulatorními sankcemi.

```python
print(f"Processing payment: card={payment.card_number}, cvv={payment.cvv}")
```

```python
logger.info("Processing payment for order_id=%s, amount=%.2f", payment.order_id, payment.amount)
```

---

## Kvalita kódu

### Architektura — God Function `create_order()`

**Řádky:** 270–336 (66 řádků, 7+ odpovědností)

Funkce současně: autentizuje uživatele, načítá produkt, validuje sklad, počítá cenu, aplikuje kupon, kontroluje zůstatek, persistuje objednávku, odesílá email. Každá změna obchodní logiky riskuje regrese v nesouvisejících částech. **Přímá vazba na bezpečnost:** netestovatelná funkce = regresní testy na SQL injection a autorizaci jsou prakticky nepsatelné bez mockování celé DB.

**Oprava:** rozložit na `_calculate_order_total()`, `_validate_stock()`, `_apply_coupon()`, `_check_user_balance()`, `_persist_order()`, `_notify_order()`.

---

### Falešná autorizace — `verify_token()` výsledek ignorován

**Řádky:** 224, 272, 382, 409

Na čtyřech místech je výsledek `verify_token()` přiřazen do `user_data`, ale proměnná se dále nepoužívá. Kód vypadá, jako by kontroloval roli — ale nekontroluje. **Přímá vazba na bezpečnost:** tato chyba kvality kódu je příčinou missing authorization nalezené security analýzou.

```python
user_data = verify_token(token)   # uloženo, ale nikdy nepoužito!
conn = get_db()
# ... žádná kontrola user_data["role"]
```

---

### Dead Code a duplicitní odpovědnosti

- `DB_PASSWORD` (ř. 21) definována, ale nikde nepoužita
- Autentizace implementována ručně místo FastAPI `Depends()` — kopírovaný kód ve všech endpointech
- Chybějící type hints u většiny funkcí (narušuje statickou analýzu, která by odhalila část chyb)

---

## Doporučení pro testy

Tests Agent navrhl **kompletní pytest test suite** s přibližně **60+ testy** pokrývajícími všechny kritické funkce. Regresní testy jsou označeny `# REGRESNÍ TEST — BUG` — dokumentují existující chyby a zabrání jejich tichému opravení bez povšimnutí.

### Prioritizovaná tabulka testů

| Komponenta | Počet testů | Priorita | Klíčové regresní testy |
|---|---|---|---|
| `hash_password` | 7 | 🔴 Okamžitě | MD5 bez soli, rainbow table |
| `generate_token` / `verify_token` | 6 | 🔴 Okamžitě | pickle RCE demonstrace |
| `register` endpoint | 5 | 🔴 Okamžitě | SQL injection v username/email |
| `login` endpoint | 5 | 🔴 Okamžitě | SQL injection auth bypass |
| `create_order` | 8 | 🔴 Okamžitě | SQL injection kupon, command injection v notes |
| `import_products` | 6 | 🔴 Okamžitě | pickle RCE v .pkl souboru, path traversal |
| `get_file` | 4 | 🟠 Sprint | path traversal `../` sekvence |
| `delete_user` | 3 | 🟠 Sprint | missing auth — unauthenticated delete |
| `list_products` | 5 | 🟠 Sprint | ORDER BY injection |
| `process_payment` | 4 | 🟠 Sprint | command injection, PCI DSS log |
| `get_db` / `init_db` | 4 | 🟡 Backlog | idempotence, connection path |
| `search_users` | 4 | 🟡 Backlog | LIKE injection, missing auth |

**Celkem:** ~61 testů | Sdílené fixtures: `tmp_db`, `seeded_db`, `client`, `seeded_client`, `alice_token`

**Klíčová poznámka k testovatelnosti:** God function `create_order()` vyžaduje mockování celé DB i subprocess — rozložení na menší funkce (viz Kvalita) je prerekvizitou pro smysluplné unit testy.

---

## Prioritizovaný akční plán

- 🔴 **Okamžitě** (blokuje nasazení): Nahradit `pickle.loads()` v `verify_token()` za `PyJWT` — eliminuje RCE přes každý autentizovaný endpoint
- 🔴 **Okamžitě** (blokuje nasazení): Zakázat import `.pkl` souborů v `import_products()` — nahradit JSON/CSV
- 🔴 **Okamžitě** (blokuje nasazení): Přesunout všechny 4 hardcoded secrets do env proměnných — zejména `sk_live_` Stripe klíč
- 🔴 **Okamžitě** (blokuje nasazení): Parametrizovat všechny SQL dotazy; `ORDER BY` ošetřit whitelistem
- 🔴 **Okamžitě** (blokuje nasazení): Odebrat `print()` s číslem karty a CVV (PCI DSS)
- 🟠 **Tento sprint** (důležité): Nahradit `hashlib.md5` za `bcrypt` nebo `argon2-cffi`
- 🟠 **Tento sprint** (důležité): Přidat chybějící autorizaci do `delete_user()`, `search_users()` a ověření role do `generate_report()`, `create_product()`
- 🟠 **Tento sprint** (důležité): Opravit path traversal v `import_products()` a `get_file()` pomocí `Path.resolve()` + prefix check
- 🟠 **Tento sprint** (důležité): Nahradit `subprocess.run(..., shell=True)` za list argumentů s whitelistem
- 🟠 **Tento sprint** (důležité): Reálně využít `user_data` z `verify_token()` — zkontrolovat roli na všech 4 místech
- 🟡 **Backlog** (zlepšení kvality): Rozložit `create_order()` na 6 menších funkcí
- 🟡 **Backlog** (zlepšení kvality): Přidat FastAPI `Depends()` pro centralizovanou autentizaci
- 🟡 **Backlog** (zlepšení kvality): Doplnit type hints a docstrings
- 🟡 **Backlog** (zlepšení kvality): Odstranit dead code `DB_PASSWORD`
- 🟡 **Backlog** (zlepšení kvality): Implementovat navržené pytest testy (61 testů, začít regresními)

---

## Celkové hodnocení

| Dimenze | Skóre | Zdůvodnění |
|---|---|---|
| **Bezpečnost** | 1 / 10 | RCE, SQL injection všude, hardcoded live Stripe klíč, MD5, missing auth |
| **Kvalita** | 3 / 10 | God function, dead code, PCI log, `shell=True`, falešná autorizace |
| **Testovatelnost** | 3 / 10 | Žádné existující testy; god function a globální DB ztěžují unit testování |
| **Čitelnost** | 5 / 10 | Struktura FastAPI endpointů je přehledná, ale délka funkcí a chybějící type hints snižují skóre |
| **Celkové skóre** | **3 / 10** | Průměr čtyř dimenzí |

**Verdikt: 🔴 Neschválit** — kód obsahuje nejméně dvě nezávislé cesty k Remote Code Execution a SQL injection ve všech databázových operacích; před jakýmkoli nasazením jsou nutné blokující opravy označené „Okamžitě".