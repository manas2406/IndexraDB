import pandas as pd
import torch
import json
import os
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
from transformers import DistilBertTokenizer, DistilBertForSequenceClassification, Trainer, TrainingArguments

script_dir = os.path.dirname(os.path.abspath(__file__))

# 1. Load Data
print("1. Loading Data...")
df = pd.read_csv(os.path.join(script_dir, "data/generated_intent_data.csv"))

# Sample a subset so training finishes in reasonable time locally
MAX_SAMPLES = 5000
if len(df) > MAX_SAMPLES:
    df = df.sample(n=MAX_SAMPLES, random_state=42).reset_index(drop=True)

print(f"   Using {len(df)} samples.")

labels = sorted(df['label'].unique().tolist())
id2label = {i: l for i, l in enumerate(labels)}
label2id = {l: i for i, l in enumerate(labels)}

df['label_id'] = df['label'].map(label2id)
train_texts, val_texts, train_labels, val_labels = train_test_split(
    df['text'], df['label_id'], test_size=0.2, random_state=42
)

# 2. Tokenize
print("2. Tokenizing...")
tokenizer = DistilBertTokenizer.from_pretrained('distilbert-base-uncased')
train_encodings = tokenizer(train_texts.tolist(), truncation=True, padding=True, max_length=64)
val_encodings = tokenizer(val_texts.tolist(), truncation=True, padding=True, max_length=64)

class IntentDataset(torch.utils.data.Dataset):
    def __init__(self, encodings, labels):
        self.encodings = encodings
        self.labels = labels.tolist()

    def __getitem__(self, idx):
        item = {key: torch.tensor(val[idx]) for key, val in self.encodings.items()}
        item['labels'] = torch.tensor(self.labels[idx])
        return item

    def __len__(self):
        return len(self.labels)

train_dataset = IntentDataset(train_encodings, train_labels)
val_dataset = IntentDataset(val_encodings, val_labels)

# 3. Train
print("3. Training...")
model = DistilBertForSequenceClassification.from_pretrained(
    'distilbert-base-uncased', num_labels=len(labels)
)

training_args = TrainingArguments(
    output_dir=os.path.join(script_dir, 'results_intent'),
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

trainer.train()

# 4. Evaluate
print("\n4. Evaluating...")
predictions = trainer.predict(val_dataset)
preds = predictions.predictions.argmax(-1)
true_labels = val_labels.tolist()
target_names = [id2label[i] for i in range(len(labels))]
print(classification_report(true_labels, preds, target_names=target_names))

# 5. Export label mapping
label_map_path = os.path.join(script_dir, "intent_labels.json")
with open(label_map_path, 'w') as f:
    json.dump({"id2label": id2label, "label2id": label2id}, f, indent=2)
print(f"   Label map saved to {label_map_path}")

# 6. Export to ONNX
print("5. Exporting to ONNX...")
model = model.cpu()
model.eval()
dummy_input = tokenizer("example query", return_tensors="pt")
onnx_path = os.path.join(script_dir, "intent.onnx")

torch.onnx.export(
    model,
    (dummy_input["input_ids"], dummy_input["attention_mask"]),
    onnx_path,
    input_names=["input_ids", "attention_mask"],
    output_names=["logits"],
    dynamic_axes={
        "input_ids": {0: "batch_size", 1: "sequence_length"},
        "attention_mask": {0: "batch_size", 1: "sequence_length"},
        "logits": {0: "batch_size"}
    },
    opset_version=14
)
print(f"   Saved to {onnx_path}")
print(f"   Size: {os.path.getsize(onnx_path) / (1024*1024):.2f} MB")
print("\nDone!")
