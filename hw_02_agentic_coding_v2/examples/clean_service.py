"""
Example: well-written async user service.
Demonstrates secure practices, proper error handling, and efficient patterns.
"""

from __future__ import annotations

import hashlib
import hmac
import os
import secrets
from contextlib import asynccontextmanager
from dataclasses import dataclass
from typing import Optional

import asyncpg


@dataclass(frozen=True)
class User:
    id: int
    username: str
    email: str


class UserNotFoundError(Exception):
    pass


class UserService:
    def __init__(self, pool: asyncpg.Pool) -> None:
        self._pool = pool

    async def get_by_id(self, user_id: int) -> User:
        async with self._pool.acquire() as conn:
            row = await conn.fetchrow(
                "SELECT id, username, email FROM users WHERE id = $1",
                user_id,
            )
        if row is None:
            raise UserNotFoundError(f"User {user_id} not found")
        return User(**row)

    async def authenticate(self, username: str, password: str) -> Optional[User]:
        async with self._pool.acquire() as conn:
            row = await conn.fetchrow(
                "SELECT id, username, email, password_hash, salt FROM users "
                "WHERE username = $1",
                username,
            )
        if row is None:
            # Constant-time comparison even on missing user to prevent timing attacks
            _dummy_compare(password)
            return None

        expected = _hash_password(password, row["salt"])
        if not hmac.compare_digest(expected, row["password_hash"]):
            return None

        return User(id=row["id"], username=row["username"], email=row["email"])

    async def list_with_tags(self, user_ids: list[int]) -> list[dict]:
        if not user_ids:
            return []
        async with self._pool.acquire() as conn:
            rows = await conn.fetch(
                """
                SELECT u.id, u.username, array_agg(t.name) AS tags
                FROM users u
                LEFT JOIN user_tags t ON t.user_id = u.id
                WHERE u.id = ANY($1::int[])
                GROUP BY u.id, u.username
                """,
                user_ids,
            )
        return [dict(r) for r in rows]


# ── Helpers ─────────────────────────────────────────────────────────────────

def _hash_password(password: str, salt: str) -> str:
    dk = hashlib.pbkdf2_hmac("sha256", password.encode(), salt.encode(), 260_000)
    return dk.hex()


def _dummy_compare(password: str) -> None:
    fake_salt = "x" * 32
    _hash_password(password, fake_salt)


def generate_salt() -> str:
    return secrets.token_hex(32)


def find_duplicates(items: list) -> list:
    seen: set = set()
    duplicates: list = []
    for item in items:
        if item in seen:
            duplicates.append(item)
        else:
            seen.add(item)
    return duplicates


# ── Database setup ───────────────────────────────────────────────────────────

@asynccontextmanager
async def create_pool(dsn: str):
    pool = await asyncpg.create_pool(dsn, min_size=2, max_size=10)
    try:
        yield pool
    finally:
        await pool.close()
