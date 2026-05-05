---
name: python-backend
description: Python backend best practices — použij při psaní FastAPI endpointů, SQLAlchemy modelů, Pydantic schémat nebo async kódu.
---

# Python Backend Best Practices

## FastAPI endpointy

```python
# Správně — response_model, status_code, async
@router.post("/users", response_model=UserResponse, status_code=201)
async def create_user(data: UserCreate, db: AsyncSession = Depends(get_db)) -> UserResponse:
    ...

# Špatně — žádný response_model, sync
@app.post("/users")
def create_user(data: dict):
    ...
```

## SQLAlchemy 2.0 async

```python
# Správně — async with, select()
async def get_user(user_id: int, db: AsyncSession) -> User | None:
    result = await db.execute(select(User).where(User.id == user_id))
    return result.scalar_one_or_none()

# Špatně — sync query API
def get_user(user_id, db):
    return db.query(User).filter(User.id == user_id).first()
```

## Pydantic v2

```python
class UserCreate(BaseModel):
    model_config = ConfigDict(str_strip_whitespace=True)
    email: EmailStr
    password: str = Field(min_length=8)
```

## Dependency Injection

```python
# DB session jako závislost — ne globální proměnná
async def get_db() -> AsyncGenerator[AsyncSession, None]:
    async with async_session() as session:
        yield session
```

## Error handling

```python
# HTTPException s jasnou zprávou, nikdy neodhaluj interní chyby
raise HTTPException(status_code=404, detail="User not found")

# Nikdy
raise Exception(str(db_error))  # stack trace v odpovědi = security issue
```

## Env a konfigurace

```python
# Settings přes pydantic-settings
class Settings(BaseSettings):
    database_url: str
    secret_key: str
    model_config = SettingsConfigDict(env_file=".env")
```
