import json
import csv
import random
import os
from typing import List, Dict, Tuple

random.seed(42)

# ─── Vocabularies ────────────────────────────────────────────────────────────

TABLES = [
    "users", "orders", "products", "customers", "logs", "items",
    "transactions", "invoices", "employees", "departments", "events",
    "sessions", "analytics", "metrics", "records", "accounts",
    "billing", "subscriptions", "payments", "reviews", "tickets",
    "inventory", "shipments", "categories", "tags", "comments",
    "posts", "messages", "notifications", "settings", "profiles",
    "students", "courses", "grades", "teachers", "classes",
    "projects", "tasks", "bugs", "features", "releases",
    "sales", "expenses", "budgets", "reports", "dashboards",
]

FIELDS = [
    "id", "name", "age", "status", "created_at", "total", "amount",
    "price", "email", "phone", "address", "role", "description",
    "category", "type", "quantity", "discount", "tax", "balance",
    "updated_at", "rating", "score", "level", "priority", "date",
    "title", "salary", "grade", "department", "location", "city",
    "country", "zip", "active", "verified", "count", "size",
    "weight", "height", "duration", "color", "brand", "model",
]

# More natural language operators
OPERATORS_NATURAL = [
    "above", "below", "greater than", "less than", "equal to",
    "more than", "fewer than", "over", "under", "at least",
    "at most", "exceeding", "not exceeding", "cheaper than",
    "more expensive than", "higher than", "lower than",
]

OPERATORS_SYMBOLIC = [">", "<", "=", ">=", "<=", "!="]

VALUES_NUMERIC = [str(i) for i in list(range(1, 100)) + list(range(100, 1001, 50)) + list(range(1000, 10001, 500))]
VALUES_STRING = [
    "active", "inactive", "pending", "completed", "failed", "cancelled",
    "admin", "user", "guest", "manager", "editor", "viewer",
    "true", "false", "null", "test", "demo", "production",
    "apple", "banana", "laptop", "phone", "tablet", "monitor",
    "keyboard", "mouse", "headphones", "cable", "charger",
    "john", "alice", "bob", "charlie", "david", "emma", "sarah",
    "new_york", "london", "tokyo", "paris", "mumbai", "berlin",
    "high", "medium", "low", "critical", "urgent", "normal",
    "premium", "basic", "standard", "enterprise", "free",
]
VALUES = VALUES_NUMERIC + VALUES_STRING

TIMES = [
    "last week", "yesterday", "today", "last month", "last year",
    "this morning", "in the last 24 hours", "over the weekend",
    "since monday", "before 2023", "after january", "this week",
    "last 7 days", "last 30 days", "past hour", "this quarter",
    "last quarter", "this year", "two days ago", "three weeks ago",
]

AGGREGATIONS = [
    "count", "sum", "average", "avg", "max", "min",
    "total number of", "mean", "total", "number of",
]

# ─── Verb Synonyms ───────────────────────────────────────────────────────────

SELECT_VERBS = [
    "show me", "find", "get", "fetch", "pull", "display", "give me",
    "list", "retrieve", "search for", "look up", "can you show",
    "i need", "show", "what are", "tell me about", "i want to see",
    "bring up", "load", "query", "select", "print", "return",
    "which are the", "where are the", "let me see",
]
CREATE_VERBS = [
    "create table", "make a new table called", "build table",
    "initialize table", "setup table", "add new table",
    "create a table named", "make table", "new table",
    "construct table", "define table", "register table",
]
INSERT_VERBS = [
    "insert into", "add record to", "put into",
    "create new entry in", "add row to", "append to",
    "add to", "insert record into", "push to",
    "add data to", "insert data into", "add entry to",
    "put record in", "save to", "store in",
]
DELETE_VERBS = [
    "delete from", "remove from", "drop row from",
    "clear record from", "trash from", "delete record from",
    "remove record from", "erase from", "wipe from",
    "remove entry from", "delete entry from", "purge from",
]
UPDATE_VERBS = [
    "update", "modify", "change", "edit",
    "set new values for", "alter record in",
    "update record in", "modify record in",
    "change value in", "edit record in",
]
JOIN_VERBS = [
    "join", "combine", "merge", "link", "connect",
    "inner join", "cross reference", "match",
]
DROP_VERBS = [
    "drop table", "remove table", "delete table", "destroy table",
    "drop", "remove entire table", "wipe table",
]

# ─── Filler / Noise Words ────────────────────────────────────────────────────

FILLERS_PRE = ["please", "can you", "could you", "kindly", "hey", "yo", ""]
FILLERS_MID = ["all", "the", "all the", "every", "each", "any", "some", ""]
NOISE_TYPOS = {
    "show": ["shwo", "shw", "show"],
    "find": ["fnd", "fidn", "find"],
    "create": ["crate", "craete", "create"],
    "table": ["tabel", "tabl", "table"],
    "where": ["wher", "whre", "where"],
    "insert": ["insrt", "inser", "insert"],
    "delete": ["delte", "delet", "delete"],
    "update": ["updae", "updte", "update"],
}

# ─── Template Definitions ────────────────────────────────────────────────────
# Each template token is (placeholder_type, slot_label_or_None)
# placeholder_type: "verb", "TABLE", "FIELD", "VALUE", "OPERATOR", "TIME",
#                    "AGGREGATION", or a literal string like " where ", " from ", etc.

TEMPLATES = {
    "SELECT": [
        # Basic: "show me users"
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("FILLER_MID", None), (" ", None), ("TABLE", "TABLE")],
        # Conditional: "find orders above 2000"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # Field conditional: "show users where age > 25"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # With field specified: "find products with price above 100"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # Time-based: "show me orders from last week"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" from ", None), ("TIME", "TIME")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" created ", None), ("TIME", "TIME")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" since ", None), ("TIME", "TIME")],
        # Aggregation: "get the count of users"
        [("verb", None), (" the ", None), ("AGGREGATION", "AGGREGATION"), (" of ", None), ("FIELD", "FIELD"), (" in ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("AGGREGATION", "AGGREGATION"), (" ", None), ("TABLE", "TABLE")],
        # Multiple conditions: "show orders where status is active and amount > 500"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE"), (" and ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # Question forms
        [("how many ", None), ("TABLE", "TABLE"), (" have ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        [("what are the ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # Equality: "find users where name is john"
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" = ", None), ("VALUE", "VALUE")],
        # Bare field: "get name from users"
        [("verb", None), (" ", None), ("FIELD", "FIELD"), (" from ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("FIELD", "FIELD"), (" of ", None), ("TABLE", "TABLE")],
    ],
    "CREATE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with fields ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with fields ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" columns ", None), ("FIELD", "FIELD"), (" and ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with columns ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" having fields ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" and ", None), ("FIELD", "FIELD"), (" and ", None), ("FIELD", "FIELD")],
    ],
    "INSERT": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" values ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" values ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("VALUE", "VALUE")],
        [("add ", None), ("VALUE", "VALUE"), (" to ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" record ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE")],
    ],
    "DELETE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" = ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        [("remove ", None), ("TABLE", "TABLE"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("VALUE", "VALUE")],
        [("delete ", None), ("VALUE", "VALUE"), (" from ", None), ("TABLE", "TABLE")],
    ],
    "UPDATE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" set ", None), ("FIELD", "FIELD"), (" to ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" set ", None), ("FIELD", "FIELD"), (" ", None), ("VALUE", "VALUE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" change ", None), ("FIELD", "FIELD"), (" to ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("FIELD", "FIELD"), (" in ", None), ("TABLE", "TABLE"), (" to ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE"), (" set ", None), ("FIELD", "FIELD"), (" to ", None), ("VALUE", "VALUE")],
        [("set ", None), ("FIELD", "FIELD"), (" of ", None), ("TABLE", "TABLE"), (" to ", None), ("VALUE", "VALUE")],
    ],
    "JOIN": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" and ", None), ("TABLE", "TABLE"), (" on ", None), ("FIELD", "FIELD")],
        [("verb", None), (" tables ", None), ("TABLE", "TABLE"), (" and ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("TABLE", "TABLE"), (" on ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" and ", None), ("TABLE", "TABLE"), (" using ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("TABLE", "TABLE"), (" on ", None), ("FIELD", "FIELD")],
    ],
    "DROP": [
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("verb", None), (" named ", None), ("TABLE", "TABLE")],
        [("verb", None), (" called ", None), ("TABLE", "TABLE")],
    ],
}


def pick_verb(intent):
    verbs = {
        "SELECT": SELECT_VERBS, "CREATE": CREATE_VERBS, "INSERT": INSERT_VERBS,
        "DELETE": DELETE_VERBS, "UPDATE": UPDATE_VERBS, "JOIN": JOIN_VERBS,
        "DROP": DROP_VERBS,
    }
    return random.choice(verbs.get(intent, [""]))


def add_noise(text: str) -> str:
    """Randomly inject noise into text for robustness."""
    # 15% chance: add filler prefix
    if random.random() < 0.15:
        text = random.choice(FILLERS_PRE) + " " + text

    # 5% chance: introduce a typo in one word
    if random.random() < 0.05:
        words = text.split()
        for i, w in enumerate(words):
            if w.lower() in NOISE_TYPOS:
                words[i] = random.choice(NOISE_TYPOS[w.lower()])
                break
        text = " ".join(words)

    # 10% chance: random case variation
    if random.random() < 0.10:
        choice = random.random()
        if choice < 0.5:
            text = text.upper()
        else:
            text = text.title()

    return text


def generate_sample(intent: str) -> Tuple[str, Dict[str, str]]:
    template = random.choice(TEMPLATES[intent])
    text_parts = []
    slots = {}

    # Track used values to avoid duplicate slot keys
    used_tables = set()
    used_fields = set()

    for token, slot_type in template:
        if token == "verb":
            text_parts.append(pick_verb(intent))
        elif token == "FILLER_MID":
            text_parts.append(random.choice(FILLERS_MID))
        elif slot_type == "TABLE":
            val = random.choice([t for t in TABLES if t not in used_tables])
            used_tables.add(val)
            text_parts.append(val)
            # For slot data, if we already have this table, add suffix
            key = val
            suffix = 2
            while key in slots:
                key = f"{val}_{suffix}"
                suffix += 1
            slots[key] = "TABLE"
        elif slot_type == "FIELD":
            available = [f for f in FIELDS if f not in used_fields]
            if not available:
                available = FIELDS
            val = random.choice(available)
            used_fields.add(val)
            text_parts.append(val)
            key = val
            suffix = 2
            while key in slots:
                key = f"{val}_{suffix}"
                suffix += 1
            slots[key] = "FIELD"
        elif slot_type == "VALUE":
            val = random.choice(VALUES)
            text_parts.append(val)
            key = val
            suffix = 2
            while key in slots:
                key = f"{val}_{suffix}"
                suffix += 1
            slots[key] = "VALUE"
        elif slot_type == "OPERATOR":
            # Mix natural and symbolic operators
            if random.random() < 0.6:
                val = random.choice(OPERATORS_NATURAL)
            else:
                val = random.choice(OPERATORS_SYMBOLIC)
            text_parts.append(val)
            slots[val] = "OPERATOR"
        elif slot_type == "TIME":
            val = random.choice(TIMES)
            text_parts.append(val)
            slots[val] = "TIME"
        elif slot_type == "AGGREGATION":
            val = random.choice(AGGREGATIONS)
            text_parts.append(val)
            slots[val] = "AGGREGATION"
        else:
            text_parts.append(token)

    final_text = "".join(text_parts).strip()
    # Collapse multiple spaces
    while "  " in final_text:
        final_text = final_text.replace("  ", " ")

    return final_text, slots


def generate_datasets(num_intent_samples: int = 15000, num_slot_samples: int = 10000):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    intent_csv_path = os.path.join(script_dir, "generated_intent_data.csv")
    slot_json_path = os.path.join(script_dir, "generated_slot_data.json")

    intent_labels = list(TEMPLATES.keys())

    # ── 1. Intent Data (CSV) ──
    print(f"Generating {num_intent_samples} intent samples -> {intent_csv_path}")
    with open(intent_csv_path, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["text", "label"])

        seen = set()
        count = 0
        attempts = 0
        max_attempts = num_intent_samples * 10  # safety valve

        while count < num_intent_samples and attempts < max_attempts:
            attempts += 1
            label = random.choice(intent_labels)
            text, _ = generate_sample(label)
            text = add_noise(text)

            if text not in seen:
                seen.add(text)
                writer.writerow([text, label])
                count += 1
                if count % 5000 == 0:
                    print(f"  Intent: {count}/{num_intent_samples}")

    print(f"  Intent: {count} samples generated.")

    # ── 2. Slot Data (JSON) ──
    print(f"Generating {num_slot_samples} slot samples -> {slot_json_path}")
    slot_data = []
    seen_slots = set()
    attempts = 0
    max_attempts = num_slot_samples * 10

    while len(slot_data) < num_slot_samples and attempts < max_attempts:
        attempts += 1
        label = random.choice(intent_labels)
        text, slots = generate_sample(label)

        if len(slots) > 0 and text not in seen_slots:
            seen_slots.add(text)
            slot_data.append({
                "text": text,
                "intent": label,  # Also store intent for context
                "slots": slots
            })
            if len(slot_data) % 5000 == 0:
                print(f"  Slots: {len(slot_data)}/{num_slot_samples}")

    with open(slot_json_path, 'w', encoding='utf-8') as jsonfile:
        json.dump(slot_data, jsonfile, indent=2)

    print(f"  Slots: {len(slot_data)} samples generated.")
    print(f"\nGeneration complete!")
    print(f"  Intent CSV: {os.path.getsize(intent_csv_path) / 1024:.1f} KB")
    print(f"  Slot JSON:  {os.path.getsize(slot_json_path) / 1024:.1f} KB")


if __name__ == "__main__":
    generate_datasets(num_intent_samples=15000, num_slot_samples=10000)
