import json
import torch
import random
from sklearn.model_selection import train_test_split
from transformers import BertTokenizerFast, BertForTokenClassification, Trainer, TrainingArguments

print("1. Loading Data...")
with open("data/generated_slot_data.json", "r", encoding="utf-8") as f:
    data = json.load(f)

# Sample a subset for local testing to avoid massive local hardware lockup
if len(data) > 2000:
    random.seed(42)
    data = random.sample(data, 2000)

print(f"Loaded {len(data)} samples.")

tokenizer = BertTokenizerFast.from_pretrained('bert-base-uncased')

# Mapping tags
unique_tags = set()
for item in data:
    for val, tag in item['slots'].items():
        unique_tags.add("B-" + tag)
        unique_tags.add("I-" + tag)
unique_tags.add("O")
unique_tags = sorted(list(unique_tags))
tag2id = {tag: i for i, tag in enumerate(unique_tags)}
id2tag = {i: tag for tag, i in tag2id.items()}

print("2. Tokenizing and Aligning labels...")
# Pre-process into NER format
texts = []
labels_list = []

for item in data:
    text = item['text']
    slots = item['slots']
    texts.append(text)
    
    # Simple character level masking
    char_labels = ["O"] * len(text)
    
    for val, tag in slots.items():
        start_idx = text.find(val)
        if start_idx != -1:
            end_idx = start_idx + len(val)
            for i in range(start_idx, end_idx):
                char_labels[i] = tag
                
    # Tokenize with offsets
    tokenized = tokenizer(text, return_offsets_mapping=True, truncation=True, padding=False)
    offsets = tokenized["offset_mapping"]
    
    token_labels = []
    prev_tag = "O"
    for idx, (start, end) in enumerate(offsets):
        if start == end: # Special tokens [CLS], [SEP]
            token_labels.append(tag2id["O"])
            continue
            
        char_tag = char_labels[start] # Take the tag of the first character of the token
        
        if char_tag == "O":
            token_labels.append(tag2id["O"])
            prev_tag = "O"
        else:
            # Determine if B- or I-
            if prev_tag == char_tag:
                token_labels.append(tag2id["I-" + char_tag])
            else:
                token_labels.append(tag2id["B-" + char_tag])
            prev_tag = char_tag
            
    labels_list.append(token_labels)

# Do full batch tokenization for padding
encodings = tokenizer(texts, truncation=True, padding=True)

# Pad labels to match attention masking
padded_labels_list = []
for i, token_labels in enumerate(labels_list):
    padded_labels = token_labels + [tag2id["O"]] * (len(encodings["input_ids"][i]) - len(token_labels))
    # PyTorch CrossEntropyLoss ignores index -100
    padded_labels = [label if label != tag2id["O"] else -100 for label in padded_labels]
    padded_labels_list.append(padded_labels)

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

train_texts, val_texts, train_labels, val_labels = train_test_split(texts, padded_labels_list, test_size=0.2, random_state=42)

# Re-encode splits
train_encodings = tokenizer(train_texts, truncation=True, padding=True)
val_encodings = tokenizer(val_texts, truncation=True, padding=True)

train_dataset = SlotDataset(train_encodings, train_labels)
val_dataset = SlotDataset(val_encodings, val_labels)

print("3. Building Model...")
model = BertForTokenClassification.from_pretrained('bert-base-uncased', num_labels=len(unique_tags))

training_args = TrainingArguments(
    output_dir='./results_slots',
    num_train_epochs=1, # 1 epoch for local subset test
    per_device_train_batch_size=8,
    logging_steps=50,
)

trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=train_dataset,
    eval_dataset=val_dataset
)

print("Starting Training...")
trainer.train()

print("4. Exporting to ONNX...")
dummy_input = tokenizer("show orders", return_tensors="pt")
# Need to make sure export device matches model device
model = model.cpu()
dummy_input = {k: v.cpu() for k, v in dummy_input.items()}

torch.onnx.export(
    model,
    (dummy_input["input_ids"], dummy_input["attention_mask"]),
    "slots.onnx",
    input_names=["input_ids", "attention_mask"],
    output_names=["logits"],
    dynamic_axes={"input_ids": {0: "batch", 1: "seq"}, "attention_mask": {0: "batch", 1: "seq"}}
)
print("Saved to models/slots.onnx")
