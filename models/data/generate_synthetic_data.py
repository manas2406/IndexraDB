import json
import csv
import random
import os
from typing import List, Dict, Tuple

# Vocabularies
TABLES = ["users", "orders", "products", "customers", "logs", "items", "transactions", "invoices", "employees", "departments", "events", "sessions", "analytics", "metrics", "records", "accounts", "billing", "subscriptions"]
FIELDS = ["id", "name", "age", "status", "created_at", "total", "amount", "price", "email", "phone", "address", "role", "description", "category", "type", "quantity", "discount", "tax", "balance", "updated_at"]
OPERATORS = ["above", "below", "greater than", "less than", "equal to", "=", ">", "<", ">=", "<=", "cheaper than", "more expensive than", "not equal to", "!=", "is", "is not", "like", "starting with", "ending with", "containing"]
VALUES = [str(i) for i in range(1, 1000)] + ["active", "inactive", "pending", "completed", "failed", "admin", "user", "guest", "true", "false", "null", "test", "demo", "apple", "banana", "laptop", "phone"]
TIMES = ["last week", "yesterday", "today", "last month", "last year", "this morning", "in the last 24 hours", "over the weekend", "since monday", "before 2023", "after january"]
AGGREGATIONS = ["count", "sum", "average", "avg", "max", "min", "total number of", "mean"]

# Verbs / Synonyms
SELECT_VERBS = ["show me", "find", "get", "fetch", "pull", "display", "give me", "list", "retrieve", "search for", "look up", "can you show", "i need"]
CREATE_VERBS = ["create table", "make a new table called", "build table", "initialize table", "setup table", "add new table"]
INSERT_VERBS = ["insert into", "add record to", "put into", "create new entry in", "add row to", "append to"]
DELETE_VERBS = ["delete from", "remove from", "drop row from", "clear record from", "trash from", "delete user", "remove user"]
UPDATE_VERBS = ["update", "modify", "change", "edit", "set new values for", "alter record in"]
ALTER_VERBS = ["alter table", "add column to", "modify table", "change schema for"]
DROP_VERBS = ["drop table", "remove table", "delete table", "destroy table"]
JOIN_VERBS = ["join", "combine", "merge", "link", "connect"]

# Templates
# Each template is a list of tuples: (literal_string, SLOT_TYPE) or (literal_string, None)
# This helps explicitly track slots without complex regex.
TEMPLATES = {
    "SELECT": [
        # Basic selects
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("verb", None), (" all ", None), ("TABLE", "TABLE")],
        # With conditions
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        # With Time
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" from ", None), ("TIME", "TIME")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" created ", None), ("TIME", "TIME")],
        # With Aggregations
        [("verb", None), (" the ", None), ("AGGREGATION", "AGGREGATION"), (" of ", None), ("FIELD", "FIELD"), (" in ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("AGGREGATION", "AGGREGATION"), (" ", None), ("TABLE", "TABLE")],
        # Multiple Conditions
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE"), (" and ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
    ],
    "CREATE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with fields ", None), ("FIELD", "FIELD"), (" ", None), ("FIELD", "FIELD")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" columns ", None), ("FIELD", "FIELD"), (" and ", None), ("FIELD", "FIELD")],
    ],
    "INSERT": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" values ", None), ("VALUE", "VALUE"), (" ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" with ", None), ("FIELD", "FIELD"), (" ", None), ("VALUE", "VALUE")],
        [("add ", None), ("VALUE", "VALUE"), (" to ", None), ("TABLE", "TABLE")],
    ],
    "DELETE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" ", None), ("OPERATOR", "OPERATOR"), (" ", None), ("VALUE", "VALUE")],
        [("remove ", None), ("TABLE", "TABLE"), (" ", None), ("VALUE", "VALUE")],
    ],
    "UPDATE": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" set ", None), ("FIELD", "FIELD"), (" to ", None), ("VALUE", "VALUE")],
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" where ", None), ("FIELD", "FIELD"), (" is ", None), ("VALUE", "VALUE"), (" set ", None), ("FIELD", "FIELD"), (" ", None), ("VALUE", "VALUE")],
    ],
    "JOIN": [
        [("verb", None), (" ", None), ("TABLE", "TABLE"), (" and ", None), ("TABLE", "TABLE"), (" on ", None), ("FIELD", "FIELD")],
        [("verb", None), (" tables ", None), ("TABLE", "TABLE"), (" and ", None), ("TABLE", "TABLE")],
    ],
    "ALTER": [
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
        [("add column ", None), ("FIELD", "FIELD"), (" to ", None), ("TABLE", "TABLE")],
    ],
    "DROP": [
        [("verb", None), (" ", None), ("TABLE", "TABLE")],
    ]
}

def pick_verb(intent):
    if intent == "SELECT": return random.choice(SELECT_VERBS)
    if intent == "CREATE": return random.choice(CREATE_VERBS)
    if intent == "INSERT": return random.choice(INSERT_VERBS)
    if intent == "DELETE": return random.choice(DELETE_VERBS)
    if intent == "UPDATE": return random.choice(UPDATE_VERBS)
    if intent == "JOIN": return random.choice(JOIN_VERBS)
    if intent == "ALTER": return random.choice(ALTER_VERBS)
    if intent == "DROP": return random.choice(DROP_VERBS)
    return ""

def generate_sample(intent: str) -> Tuple[str, Dict[str, str]]:
    template = random.choice(TEMPLATES[intent])
    text_parts = []
    slots = {}
    
    for token, slot_type in template:
        if token == "verb":
            text_parts.append(pick_verb(intent))
        elif slot_type == "TABLE":
            val = random.choice(TABLES)
            text_parts.append(val)
            slots[val] = "TABLE"
        elif slot_type == "FIELD":
            val = random.choice(FIELDS)
            text_parts.append(val)
            slots[val] = "FIELD"
        elif slot_type == "VALUE":
            val = random.choice(VALUES)
            text_parts.append(val)
            slots[val] = "VALUE"
        elif slot_type == "OPERATOR":
            val = random.choice(OPERATORS)
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
            
    final_text = "".join(text_parts).strip().replace("  ", " ")
    return final_text, slots

def generate_datasets(num_intent_samples: int = 500000, num_slot_samples: int = 200000):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    intent_csv_path = os.path.join(script_dir, "generated_intent_data.csv")
    slot_json_path = os.path.join(script_dir, "generated_slot_data.json")
    
    intent_labels = list(TEMPLATES.keys())
    
    # 1. Generate Intent Data (CSV)
    print(f"Generating {num_intent_samples} intent samples -> {intent_csv_path}")
    with open(intent_csv_path, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["text", "label"])
        
        # Keep track of generated to reduce exact duplicates somewhat
        seen = set()
        count = 0
        while count < num_intent_samples:
            label = random.choice(intent_labels)
            text, _ = generate_sample(label)
            if text not in seen:
                seen.add(text)
                writer.writerow([text, label])
                count += 1
                if count % 100000 == 0:
                    print(f"  ... {count} done")

    # 2. Generate Slot Data (JSON)
    print(f"Generating {num_slot_samples} slot samples -> {slot_json_path}")
    slot_data = []
    seen_slots = set()
    while len(slot_data) < num_slot_samples:
        label = random.choice(intent_labels)
        text, slots = generate_sample(label)
        
        # Only keep if it has slots and is relatively unique
        if len(slots) > 0 and text not in seen_slots:
            seen_slots.add(text)
            slot_data.append({
                "text": text,
                "slots": slots
            })
            if len(slot_data) % 50000 == 0:
                print(f"  ... {len(slot_data)} done")
                
    with open(slot_json_path, 'w', encoding='utf-8') as jsonfile:
        json.dump(slot_data, jsonfile, indent=2)
        
    print("Generation complete!")
    print(f"CSV Size: {os.path.getsize(intent_csv_path) / (1024*1024):.2f} MB")
    print(f"JSON Size: {os.path.getsize(slot_json_path) / (1024*1024):.2f} MB")

if __name__ == "__main__":
    # We aim for ~50MB total. JSON is very verbose, so 300,000 JSON rows ≈ 35MB. 
    # CSV is dense, 500,000 rows ≈ 15-20MB. Total should be > 50MB.
    generate_datasets(num_intent_samples=500000, num_slot_samples=300000)
