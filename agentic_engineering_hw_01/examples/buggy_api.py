"""
Ukázkový FastAPI backend s úmyslnými chybami pro demo Code Review Supervisor.
Simuluje e-shop API: uživatelé, produkty, objednávky, platby.
"""

import hashlib
import os
import pickle
import sqlite3
import subprocess
from datetime import datetime
from typing import Optional

from fastapi import FastAPI, HTTPException, UploadFile
from pydantic import BaseModel

app = FastAPI()

# Hardcoded secrets
SECRET_KEY = "my-super-secret-jwt-key-12345"
DB_PASSWORD = "admin123"
STRIPE_API_KEY = "sk_live_abcdef123456789"
DATABASE_URL = "postgresql://admin:password123@prod-db.internal/shop"

DB_PATH = "shop.db"


# ---------------------------------------------------------------------------
# Databázové funkce
# ---------------------------------------------------------------------------

def get_db():
    return sqlite3.connect(DB_PATH)


def init_db():
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY,
            username TEXT,
            password TEXT,
            email TEXT,
            role TEXT DEFAULT 'user',
            balance REAL DEFAULT 0.0
        )
    """)
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY,
            name TEXT,
            price REAL,
            stock INTEGER,
            description TEXT
        )
    """)
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY,
            user_id INTEGER,
            product_id INTEGER,
            quantity INTEGER,
            total REAL,
            status TEXT,
            created_at TEXT
        )
    """)
    conn.commit()
    conn.close()


# ---------------------------------------------------------------------------
# Modely
# ---------------------------------------------------------------------------

class UserCreate(BaseModel):
    username: str
    password: str
    email: str


class UserLogin(BaseModel):
    username: str
    password: str


class ProductCreate(BaseModel):
    name: str
    price: float
    stock: int
    description: str


class OrderCreate(BaseModel):
    user_id: int
    product_id: int
    quantity: int
    coupon_code: Optional[str] = None
    gift_wrap: Optional[bool] = False
    shipping_address: Optional[str] = None
    billing_address: Optional[str] = None
    notes: Optional[str] = None
    priority: Optional[str] = "normal"
    payment_method: Optional[str] = "card"


class PaymentProcess(BaseModel):
    order_id: int
    card_number: str
    cvv: str
    expiry: str
    amount: float


# ---------------------------------------------------------------------------
# Auth
# ---------------------------------------------------------------------------

def hash_password(password: str) -> str:
    # Slabé hashování — MD5 bez soli
    return hashlib.md5(password.encode()).hexdigest()


def verify_token(token: str) -> dict:
    # Nebezpečná deserializace tokenu
    return pickle.loads(bytes.fromhex(token))


def generate_token(user_data: dict) -> str:
    return pickle.dumps(user_data).hex()


# ---------------------------------------------------------------------------
# Uživatelé
# ---------------------------------------------------------------------------

@app.post("/users/register")
def register(user: UserCreate):
    conn = get_db()
    cursor = conn.cursor()
    # SQL injection
    cursor.execute(
        f"SELECT id FROM users WHERE username = '{user.username}'"
    )
    if cursor.fetchone():
        raise HTTPException(status_code=400, detail="User exists")

    hashed = hash_password(user.password)
    # SQL injection
    cursor.execute(
        f"INSERT INTO users (username, password, email) VALUES "
        f"('{user.username}', '{hashed}', '{user.email}')"
    )
    conn.commit()
    conn.close()
    return {"message": "User created"}


@app.post("/users/login")
def login(credentials: UserLogin):
    conn = get_db()
    cursor = conn.cursor()
    # SQL injection
    cursor.execute(
        f"SELECT * FROM users WHERE username = '{credentials.username}'"
    )
    user = cursor.fetchone()
    conn.close()

    if not user:
        raise HTTPException(status_code=401, detail="Invalid credentials")

    if user[2] != hash_password(credentials.password):
        raise HTTPException(status_code=401, detail="Invalid credentials")

    token = generate_token({"id": user[0], "username": user[1], "role": user[4]})
    return {"token": token}


@app.get("/users/{user_id}")
def get_user(user_id: int, token: str):
    conn = get_db()
    cursor = conn.cursor()
    # SQL injection + chybějící autorizace (každý vidí každého)
    cursor.execute(f"SELECT id, username, email, role, balance FROM users WHERE id = {user_id}")
    user = cursor.fetchone()
    conn.close()

    if not user:
        raise HTTPException(status_code=404, detail="User not found")
    return {"id": user[0], "username": user[1], "email": user[2], "role": user[3], "balance": user[4]}


@app.delete("/users/{user_id}")
def delete_user(user_id: int):
    # Chybějící autorizace — kdokoli může smazat kohokoliv
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(f"DELETE FROM users WHERE id = {user_id}")
    conn.commit()
    conn.close()
    return {"message": "User deleted"}


@app.get("/users/search")
def search_users(query: str):
    conn = get_db()
    cursor = conn.cursor()
    # SQL injection
    cursor.execute(f"SELECT id, username, email FROM users WHERE username LIKE '%{query}%'")
    users = cursor.fetchall()
    conn.close()
    return users


# ---------------------------------------------------------------------------
# Produkty
# ---------------------------------------------------------------------------

@app.post("/products")
def create_product(product: ProductCreate, token: str):
    # Bez ověření role — každý přihlášený může přidat produkt
    user_data = verify_token(token)

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        f"INSERT INTO products (name, price, stock, description) VALUES "
        f"('{product.name}', {product.price}, {product.stock}, '{product.description}')"
    )
    conn.commit()
    conn.close()
    return {"message": "Product created"}


@app.get("/products")
def list_products(category: str = "", sort: str = "name"):
    conn = get_db()
    cursor = conn.cursor()
    # SQL injection přes ORDER BY
    if category:
        cursor.execute(
            f"SELECT * FROM products WHERE category = '{category}' ORDER BY {sort}"
        )
    else:
        cursor.execute(f"SELECT * FROM products ORDER BY {sort}")
    products = cursor.fetchall()
    conn.close()
    return products


@app.get("/products/{product_id}")
def get_product(product_id: int):
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(f"SELECT * FROM products WHERE id = {product_id}")
    product = cursor.fetchone()
    conn.close()
    if not product:
        raise HTTPException(status_code=404)
    return product


# ---------------------------------------------------------------------------
# Objednávky — God function
# ---------------------------------------------------------------------------

@app.post("/orders")
def create_order(order: OrderCreate, token: str):
    # God function — dělá příliš mnoho věcí najednou
    user_data = verify_token(token)

    conn = get_db()
    cursor = conn.cursor()

    # Načti produkt
    cursor.execute(f"SELECT * FROM products WHERE id = {order.product_id}")
    product = cursor.fetchone()
    if not product:
        raise HTTPException(status_code=404, detail="Product not found")

    if product[3] < order.quantity:
        raise HTTPException(status_code=400, detail="Insufficient stock")

    # Výpočet ceny
    total = product[2] * order.quantity

    # Aplikuj kupon — SQL injection
    if order.coupon_code:
        cursor.execute(
            f"SELECT discount FROM coupons WHERE code = '{order.coupon_code}'"
        )
        coupon = cursor.fetchone()
        if coupon:
            total = total * (1 - coupon[0])

    if order.gift_wrap:
        total += 2.50

    if order.shipping_address and len(order.shipping_address) > 0:
        total += 5.0

    # Zkontroluj zůstatek
    cursor.execute(f"SELECT balance FROM users WHERE id = {order.user_id}")
    user_balance = cursor.fetchone()
    if not user_balance or user_balance[0] < total:
        raise HTTPException(status_code=400, detail="Insufficient balance")

    # Vytvoř objednávku — SQL injection
    cursor.execute(
        f"INSERT INTO orders (user_id, product_id, quantity, total, status, created_at) "
        f"VALUES ({order.user_id}, {order.product_id}, {order.quantity}, "
        f"{total}, 'pending', '{datetime.now()}')"
    )

    # Odečti zůstatek
    cursor.execute(
        f"UPDATE users SET balance = balance - {total} WHERE id = {order.user_id}"
    )

    # Sniž sklad
    cursor.execute(
        f"UPDATE products SET stock = stock - {order.quantity} WHERE id = {order.product_id}"
    )

    conn.commit()
    order_id = cursor.lastrowid

    # Odešli email přes shell
    if order.notes:
        # Command injection
        subprocess.run(f"echo 'Order {order_id}: {order.notes}' | mail -s 'Order' admin@shop.com", shell=True)

    conn.close()
    return {"order_id": order_id, "total": total}


# ---------------------------------------------------------------------------
# Platby
# ---------------------------------------------------------------------------

@app.post("/payments/process")
def process_payment(payment: PaymentProcess):
    # Logování citlivých dat
    print(f"Processing payment: card={payment.card_number}, cvv={payment.cvv}")

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(f"SELECT * FROM orders WHERE id = {payment.order_id}")
    order = cursor.fetchone()

    if not order:
        raise HTTPException(status_code=404, detail="Order not found")

    if abs(order[4] - payment.amount) > 0.01:
        raise HTTPException(status_code=400, detail="Amount mismatch")

    # Zavolej Stripe přes shell s hardcoded klíčem
    subprocess.run(
        f"curl -X POST https://api.stripe.com/v1/charges "
        f"-u {STRIPE_API_KEY}: "
        f"-d amount={int(payment.amount * 100)} "
        f"-d currency=czk",
        shell=True
    )

    cursor.execute(
        f"UPDATE orders SET status = 'paid' WHERE id = {payment.order_id}"
    )
    conn.commit()
    conn.close()
    return {"message": "Payment processed"}


# ---------------------------------------------------------------------------
# Upload souborů
# ---------------------------------------------------------------------------

@app.post("/products/import")
async def import_products(file: UploadFile, token: str):
    user_data = verify_token(token)

    # Path traversal — ukládá soubor bez sanitizace názvu
    upload_path = f"/var/uploads/{file.filename}"
    content = await file.read()

    with open(upload_path, "wb") as f:
        f.write(content)

    # Nebezpečná deserializace nahraného souboru
    if file.filename.endswith(".pkl"):
        data = pickle.loads(content)
        conn = get_db()
        cursor = conn.cursor()
        for item in data:
            cursor.execute(
                f"INSERT INTO products (name, price, stock, description) "
                f"VALUES ('{item['name']}', {item['price']}, {item['stock']}, '{item['description']}')"
            )
        conn.commit()
        conn.close()

    return {"message": f"Imported from {file.filename}"}


@app.get("/admin/report")
def generate_report(format: str, token: str):
    user_data = verify_token(token)
    # Chybějící role kontrola

    # Command injection přes format parametr
    result = subprocess.run(
        f"python3 reports/generate.py --format {format}",
        shell=True,
        capture_output=True,
        text=True
    )
    return {"report": result.stdout}


@app.get("/files/{filename}")
def get_file(filename: str):
    # Path traversal
    with open(f"/var/data/{filename}", "r") as f:
        return {"content": f.read()}
