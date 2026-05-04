"""
ONNX ML Inference Module for VultureDB NLQ Pipeline.
Loads intent.onnx and slots.onnx and runs inference using ONNX Runtime.
"""
import os
import json
import numpy as np
import onnxruntime as ort
from transformers import DistilBertTokenizer, BertTokenizerFast

# Resolve paths relative to this file
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "..", "models")

# ─── Load Models & Mappings on Module Import ─────────────────────────────────

# Intent Model (DistilBERT)
_intent_tokenizer = DistilBertTokenizer.from_pretrained("distilbert-base-uncased")
_intent_session = ort.InferenceSession(os.path.join(MODELS_DIR, "intent.onnx"))

with open(os.path.join(MODELS_DIR, "intent_labels.json"), "r") as f:
    _intent_maps = json.load(f)
_id2label = {int(k): v for k, v in _intent_maps["id2label"].items()}

# Slots Model (BERT Token Classification)
_slots_tokenizer = BertTokenizerFast.from_pretrained("bert-base-uncased")
_slots_session = ort.InferenceSession(os.path.join(MODELS_DIR, "slots.onnx"))

with open(os.path.join(MODELS_DIR, "slot_tags.json"), "r") as f:
    _slot_maps = json.load(f)
_id2tag = {int(k): v for k, v in _slot_maps["id2tag"].items()}

print(f"[ML] Loaded intent.onnx ({len(_id2label)} classes)")
print(f"[ML] Loaded slots.onnx ({len(_id2tag)} tags)")


def predict_intent(text: str) -> str:
    """Predict the intent (SELECT, CREATE, INSERT, etc.) for a query."""
    encoded = _intent_tokenizer(
        text, return_tensors="np", truncation=True, padding=True, max_length=64
    )
    
    outputs = _intent_session.run(
        ["logits"],
        {
            "input_ids": encoded["input_ids"].astype(np.int64),
            "attention_mask": encoded["attention_mask"].astype(np.int64),
        },
    )
    
    logits = outputs[0][0]  # shape: (num_classes,)
    predicted_id = int(np.argmax(logits))
    return _id2label.get(predicted_id, "SELECT")


def predict_slots(text: str) -> list:
    """
    Extract slot entities from the query text.
    Returns list of dicts: [{"text": "orders", "label": "TABLE"}, ...]
    """
    encoded = _slots_tokenizer(
        text,
        return_tensors="np",
        truncation=True,
        padding=True,
        max_length=64,
        return_offsets_mapping=True,
    )
    
    offsets = encoded.pop("offset_mapping")[0]  # shape: (seq_len, 2)
    
    outputs = _slots_session.run(
        ["logits"],
        {
            "input_ids": encoded["input_ids"].astype(np.int64),
            "attention_mask": encoded["attention_mask"].astype(np.int64),
        },
    )
    
    logits = outputs[0][0]  # shape: (seq_len, num_tags)
    predicted_ids = np.argmax(logits, axis=-1)
    
    # Reconstruct entities from BIO tags
    entities = []
    current_entity = None
    current_label = None
    
    for idx, (tag_id, (start, end)) in enumerate(zip(predicted_ids, offsets)):
        if start == 0 and end == 0:
            # Special token [CLS] or [SEP]
            if current_entity is not None:
                entities.append({"text": current_entity.strip(), "label": current_label})
                current_entity = None
                current_label = None
            continue
        
        tag = _id2tag.get(int(tag_id), "O")
        
        if tag.startswith("B-"):
            # Start of a new entity
            if current_entity is not None:
                entities.append({"text": current_entity.strip(), "label": current_label})
            current_label = tag[2:]  # Remove "B-" prefix
            current_entity = text[start:end]
        elif tag.startswith("I-") and current_entity is not None:
            # Continuation of current entity
            current_entity += text[start:end] if text[start - 1:start] == "" else " " + text[start:end]
        else:
            # "O" tag — end any current entity
            if current_entity is not None:
                entities.append({"text": current_entity.strip(), "label": current_label})
                current_entity = None
                current_label = None
    
    # Don't forget the last entity
    if current_entity is not None:
        entities.append({"text": current_entity.strip(), "label": current_label})
    
    return entities


# Quick test
if __name__ == "__main__":
    test_queries = [
        "show me all users",
        "find orders above 2000",
        "create table students with fields name age grade",
        "insert into products values 1 laptop 900",
        "update products set price 60 where name mouse",
        "join users and orders on id",
        "delete from orders where status is cancelled",
    ]
    
    for q in test_queries:
        intent = predict_intent(q)
        slots = predict_slots(q)
        print(f"\nQuery: '{q}'")
        print(f"  Intent: {intent}")
        print(f"  Slots:  {slots}")
