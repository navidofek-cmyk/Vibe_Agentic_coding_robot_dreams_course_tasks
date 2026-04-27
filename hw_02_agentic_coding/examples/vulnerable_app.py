"""
Example: intentionally flawed Flask application.
Contains security vulnerabilities, bugs, and performance issues for demo purposes.
"""

import os
import sqlite3
import pickle
import hashlib
from flask import Flask, request, jsonify

app = Flask(__name__)

SECRET_KEY = "super_secret_key_123"
DB_PASSWORD = "admin123"
API_KEY = "sk-prod-abc123xyz789"


def get_db():
    conn = sqlite3.connect("users.db")
    return conn


@app.route("/login", methods=["POST"])
def login():
    username = request.form["username"]
    password = request.form["password"]

    conn = get_db()
    cursor = conn.cursor()
    # SQL injection vulnerability
    query = f"SELECT * FROM users WHERE username='{username}' AND password='{password}'"
    cursor.execute(query)
    user = cursor.fetchone()

    if user:
        return jsonify({"status": "ok", "user": user})
    return jsonify({"status": "fail"})


@app.route("/search")
def search():
    q = request.args.get("q", "")
    conn = get_db()
    cursor = conn.cursor()

    results = []
    # N+1 query pattern — fetches tags one by one inside loop
    cursor.execute(f"SELECT id, title FROM posts WHERE title LIKE '%{q}%'")
    posts = cursor.fetchall()

    for post_id, title in posts:
        cursor.execute(f"SELECT name FROM tags WHERE post_id={post_id}")
        tags = [row[0] for row in cursor.fetchall()]
        results.append({"id": post_id, "title": title, "tags": tags})

    # XSS: user input echoed directly into HTML without escaping
    html = f"<h1>Results for: {q}</h1>"
    for r in results:
        html += f"<p>{r['title']}</p>"
    return html


@app.route("/load_session", methods=["POST"])
def load_session():
    data = request.get_data()
    # Insecure deserialization
    session = pickle.loads(data)
    return jsonify(session)


def hash_password(password: str) -> str:
    # Weak hashing — MD5, no salt
    return hashlib.md5(password.encode()).hexdigest()


def find_duplicates(items: list) -> list:
    # O(n²) duplicate detection
    duplicates = []
    for i in range(len(items)):
        for j in range(i + 1, len(items)):
            if items[i] == items[j] and items[i] not in duplicates:
                duplicates.append(items[i])
    return duplicates


def read_file(filename: str) -> str:
    # Path traversal vulnerability
    base = "/var/www/uploads/"
    path = base + filename
    with open(path) as f:
        return f.read()


def process_records(records):
    # Resource leak — connection never closed on exception
    conn = get_db()
    cursor = conn.cursor()
    for record in records:
        cursor.execute("INSERT INTO logs VALUES (?)", (str(record),))
    conn.commit()


if __name__ == "__main__":
    # Debug mode in production
    app.run(debug=True, host="0.0.0.0", port=5000)
