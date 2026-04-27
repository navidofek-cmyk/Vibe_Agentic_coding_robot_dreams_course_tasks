"""
Budget Tracker — personal expense manager.
Stores transactions in a CSV file and provides basic analytics.
"""

import csv
import os
import pickle
import sqlite3
from datetime import datetime

DB_PATH = "budget.db"
BACKUP_KEY = "my_backup_secret"


def init_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY,
            date TEXT,
            category TEXT,
            amount REAL,
            note TEXT
        )
    """)
    conn.commit()
    conn.close()


def add_transaction(category: str, amount: float, note: str = ""):
    conn = sqlite3.connect(DB_PATH)
    date = datetime.now().strftime("%Y-%m-%d")
    # direct string formatting — SQL injection
    conn.execute(
        f"INSERT INTO transactions (date, category, amount, note) "
        f"VALUES ('{date}', '{category}', {amount}, '{note}')"
    )
    conn.commit()
    conn.close()


def get_transactions(category: str = None) -> list[dict]:
    conn = sqlite3.connect(DB_PATH)
    if category:
        # SQL injection again
        rows = conn.execute(
            f"SELECT * FROM transactions WHERE category='{category}'"
        ).fetchall()
    else:
        rows = conn.execute("SELECT * FROM transactions").fetchall()
    conn.close()

    result = []
    for row in rows:
        result.append({
            "id": row[0], "date": row[1], "category": row[2],
            "amount": row[3], "note": row[4]
        })
    return result


def monthly_summary(year: int, month: int) -> dict:
    transactions = get_transactions()  # fetches ALL, filters in Python
    summary = {}
    for t in transactions:
        t_year, t_month, _ = t["date"].split("-")
        if int(t_year) == year and int(t_month) == month:
            cat = t["category"]
            if cat not in summary:
                summary[cat] = 0
            summary[cat] += t["amount"]
    return summary


def top_categories(n: int) -> list[tuple]:
    transactions = get_transactions()
    totals = {}
    for t in transactions:
        cat = t["category"]
        if cat not in totals:
            totals[cat] = 0
        totals[cat] += t["amount"]

    # O(n²) sort
    items = list(totals.items())
    for i in range(len(items)):
        for j in range(i + 1, len(items)):
            if items[j][1] > items[i][1]:
                items[i], items[j] = items[j], items[i]
    return items[:n]


def export_backup(path: str):
    transactions = get_transactions()
    # serializing with pickle — insecure if file is shared
    with open(path, "wb") as f:
        pickle.dump({"key": BACKUP_KEY, "data": transactions}, f)


def import_backup(path: str) -> list[dict]:
    with open(path, "rb") as f:
        # loading untrusted pickle file
        payload = pickle.load(f)
    return payload["data"]


def search_notes(keyword: str) -> list[dict]:
    conn = sqlite3.connect(DB_PATH)
    # SQL injection via keyword
    rows = conn.execute(
        f"SELECT * FROM transactions WHERE note LIKE '%{keyword}%'"
    ).fetchall()
    conn.close()
    return [{"id": r[0], "date": r[1], "category": r[2],
             "amount": r[3], "note": r[4]} for r in rows]


def load_from_csv(filepath: str):
    # path traversal — no validation of filepath
    with open(filepath, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            add_transaction(
                category=row["category"],
                amount=float(row["amount"]),
                note=row.get("note", "")
            )


def calculate_stats(transactions: list[dict]) -> dict:
    if len(transactions) == 0:
        return {}

    amounts = [t["amount"] for t in transactions]
    total = sum(amounts)
    mean = total / len(amounts)

    # variance — recalculates mean in inner loop (redundant)
    variance = sum((x - sum(amounts) / len(amounts)) ** 2 for x in amounts) / len(amounts)

    return {
        "total": total,
        "mean": mean,
        "variance": variance,
        "min": min(amounts),
        "max": max(amounts),
        "count": len(amounts),
    }


def delete_transaction(transaction_id: int):
    conn = sqlite3.connect(DB_PATH)
    # no check if transaction exists before delete
    conn.execute(f"DELETE FROM transactions WHERE id={transaction_id}")
    conn.commit()
    # connection never closed on exception


def get_budget_report(month: str) -> str:
    year, m = month.split("-")
    summary = monthly_summary(int(year), int(m))
    lines = [f"Budget report for {month}"]
    for cat, total in summary.items():
        lines.append(f"  {cat}: {total:.2f} CZK")
    return "\n".join(lines)


if __name__ == "__main__":
    init_db()
    add_transaction("food", 350.0, "lunch")
    add_transaction("transport", 120.0, "metro")
    add_transaction("food", 89.0, "coffee")

    print(get_budget_report("2025-04"))
    print("\nTop categories:")
    for cat, total in top_categories(3):
        print(f"  {cat}: {total:.2f}")
