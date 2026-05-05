"""
Ukázkový soubor s úmyslnými chybami pro demo Code Review Supervisor.
Obsahuje bezpečnostní zranitelnosti, problémy s kvalitou i chybějící testy.
"""

import hashlib
import os
import pickle
import sqlite3

# Hardcoded secret — bezpečnostní zranitelnost
SECRET_KEY = "super-tajny-klic-1234"
DB_PASSWORD = "admin123"


def get_user(username):
    # SQL injection — bezpečnostní zranitelnost
    conn = sqlite3.connect("users.db")
    cursor = conn.cursor()
    cursor.execute(f"SELECT * FROM users WHERE username = '{username}'")
    return cursor.fetchone()


def hash_password(password):
    # Slabé hashování — MD5 pro hesla
    return hashlib.md5(password.encode()).hexdigest()


def load_session(data):
    # Nebezpečná deserializace
    return pickle.loads(data)


def read_file(filename):
    # Path traversal zranitelnost
    with open(f"/var/uploads/{filename}", "r") as f:
        return f.read()


def process_order(user_id, product_id, quantity, price, discount, tax, shipping, notes, priority, metadata):
    # Příliš mnoho parametrů — porušení principů clean code
    # Bez type hints, bez docstringu
    total = price * quantity
    total = total * (1 - discount)
    total = total + tax
    total = total + shipping
    conn = sqlite3.connect("orders.db")
    cursor = conn.cursor()
    # Další SQL injection
    cursor.execute(
        f"INSERT INTO orders VALUES ('{user_id}', '{product_id}', {quantity}, {total})"
    )
    conn.commit()
    return total


class UserManager:
    def __init__(self):
        self.users = {}
        self.db = sqlite3.connect("users.db")

    def create_user(self, username, password, email):
        # Bez validace vstupu
        h = hash_password(password)
        self.users[username] = {"password": h, "email": email}
        cursor = self.db.cursor()
        cursor.execute(
            f"INSERT INTO users VALUES ('{username}', '{h}', '{email}')"
        )
        self.db.commit()

    def authenticate(self, username, password):
        # Bez ošetření výjimek, leak informací
        user = get_user(username)
        if user and user[2] == hash_password(password):
            return True
        return False

    def delete_user(self, username):
        # Bez autorizační kontroly, SQL injection
        del self.users[username]
        cursor = self.db.cursor()
        cursor.execute(f"DELETE FROM users WHERE username = '{username}'")
        self.db.commit()

    def get_all_users(self):
        # Vrací hesla — data exposure
        return list(self.users.values())
