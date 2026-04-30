import json
import torch
import random
import os
from sklearn.model_selection import train_test_split
from transformers import BertTokenizerFast, BertForTokenClassification, Trainer, TrainingArguments

script_dir = os.path.dirname(os.path.abspath(__file__))

print("1. Loading Data...")
with open(os.path.join(script_dir, "data/generated_slot_data.json"), "r", encoding="utf-8") as f:
    data = json.load(f)

# Sample a subset for local training
MAX_SAMPLES = 5000
if len(data) > MAX_SAMPLES:
    random.seed(42)
    data = random.sample(data, MAX_SAMPLES)

print(f"   Using {len(data)} samples.")

tokenizer = BertTokenizerFast.from_pretrained('bert-base-uncased')

# Build tag set from data
unique_tags = set()
for item in data:
    for val, tag in item['slots'].items():
        unique_tags.add("B-" + tag)
        unique_tags.add("I-" + tag)
unique_tags.add("O")
unique_tags = sorted(list(unique_tags))
tag2id = {tag: i for i, tag in enumerate(unique_tags)}
id2tag = {i: tag for tag, i in tag2id.items()}

print(f"   Tags: {unique_tags}")

print("2. Tokenizing and aligning labels...")
texts = []
labels_list = []

for item in data:
    text = item['text']
    slots = item['slots']
    texts.append(text)

    # Character-level label assignment
    char_labels = ["O"] * len(text)

    for val, tag in slots.items():
        # Clean val (remove suffixes like _2 added by generator for dedup)
        clean_val = val.split("_")[0] if val[-1].isdigit() and "_" in val else val
        start_idx = text.lower().find(clean_val.lower())
        if start_idx != -1:
            end_idx = start_idx + len(clean_val)
            for i in range(start_idx, end_idx):
                char_labels[i] = tag

    # Tokenize with offsets
    tokenized = tokenizer(text, return_offsets_mapping=True, truncation=True, padding=False, max_length=64)
    offsets = tokenized["offset_mapping"]

    token_labels = []
    prev_tag = "O"
    for idx, (start, end) in enumerate(offsets):
        if start == end:  # Special tokens [CLS], [SEP]
            token_labels.append(tag2id["O"])
            prev_tag = "O"
            continue

        char_tag = char_labels[start]

        if char_tag == "O":
            token_labels.append(tag2id["O"])
            prev_tag = "O"
        else:
            if prev_tag == char_tag:
                bio_tag = "I-" + char_tag
            else:
                bio_tag = "B-" + char_tag
            if bio_tag in tag2id:
                token_labels.append(tag2id[bio_tag])
            else:
                token_labels.append(tag2id["O"])
            prev_tag = char_tag

    labels_list.append(token_labels)

# Full batch tokenization for padding
encodings = tokenizer(texts, truncation=True, padding=True, max_length=64)

# Pad labels
padded_labels_list = []
for i, token_labels in enumerate(labels_list):
    seq_len = len(encodings["input_ids"][i])
    padded = token_labels[:seq_len]  # truncate if needed
    padded = padded + [-100] * (seq_len - len(padded))  # pad with -100 (ignored by loss)
    padded_labels_list.append(padded)

class SlotDataset(torch.utils.data.Dataset):
    def __init__(self, encodings, labels):
        self.encodings = encodings
        self.labels = labels

    def __getitem__(self, idx):
        item = {key: torch.tensor(val[idx]) for key, val in self.encodings.items()}
        item['labels'] = torch.tensor(self.labels[idx])
        return item

    def __len__(self):
        return len(self.labels)

train_texts, val_texts, train_labels, val_labels = train_test_split(
    texts, padded_labels_list, test_size=0.2, random_state=42
)

# Re-encode splits
train_encodings = tokenizer(train_texts, truncation=True, padding=True, max_length=64)
val_encodings = tokenizer(val_texts, truncation=True, padding=True, max_length=64)

# Re-pad labels to match new encoding lengths
def repad_labels(labels, encodings):
    result = []
    for i, lbl in enumerate(labels):
        seq_len = len(encodings["input_ids"][i])
        padded = lbl[:seq_len]
        padded = padded + [-100] * (seq_len - len(padded))
        result.append(padded)
    return result

train_labels = repad_labels(train_labels, train_encodings)
val_labels = repad_labels(val_labels, val_encodings)

train_dataset = SlotDataset(train_encodings, train_labels)
val_dataset = SlotDataset(val_encodings, val_labels)

print("3. Building Model...")
model = BertForTokenClassification.from_pretrained('bert-base-uncased', num_labels=len(unique_tags))

training_args = TrainingArguments(
    output_dir=os.path.join(script_dir, 'results_slots'),
    num_train_epochs=3,
    per_device_train_batch_size=16,
    per_device_eval_batch_size=32,
    eval_strategy="epoch",
    logging_steps=100,
    save_strategy="no",
    report_to="none",
)

trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=train_dataset,
    eval_dataset=val_dataset
)

print("   Starting Training...")
trainer.train()

# 4. Export tag mapping
tag_map_path = os.path.join(script_dir, "slot_tags.json")
with open(tag_map_path, 'w') as f:
    json.dump({"id2tag": id2tag, "tag2id": tag2id}, f, indent=2)
print(f"\n4. Tag map saved to {tag_map_path}")

# 5. Export to ONNX
print("5. Exporting to ONNX...")
model = model.cpu()
model.eval()
dummy_input = tokenizer("show orders", return_tensors="pt")

onnx_path = os.path.join(script_dir, "slots.onnx")
torch.onnx.export(
    model,
    (dummy_input["input_ids"], dummy_input["attention_mask"]),
    onnx_path,
    input_names=["input_ids", "attention_mask"],
    output_names=["logits"],
    dynamic_axes={
        "input_ids": {0: "batch", 1: "seq"},
        "attention_mask": {0: "batch", 1: "seq"},
    },
    opset_version=14
)
print(f"   Saved to {onnx_path}")
print(f"   Size: {os.path.getsize(onnx_path) / (1024*1024):.2f} MB")
print("\nDone!")
