import pandas as pd
import torch
from sklearn.model_selection import train_test_split
from transformers import DistilBertTokenizer, DistilBertForSequenceClassification, Trainer, TrainingArguments

# 1. Load Data
df = pd.read_csv("data/generated_intent_data.csv")

# Sample a subset so training actually finishes in a reasonable time locally
# Defaulting to 2000. Increase this when running on powerful GPUs overnight.
if len(df) > 2000:
    df = df.sample(n=2000, random_state=42).reset_index(drop=True)

labels = df['label'].unique().tolist()
id2label = {i: l for i, l in enumerate(labels)}
label2id = {l: i for i, l in enumerate(labels)}

df['label_id'] = df['label'].map(label2id)
train_texts, val_texts, train_labels, val_labels = train_test_split(df['text'], df['label_id'], test_size=0.2)

# 2. Tokenize
tokenizer = DistilBertTokenizer.from_pretrained('distilbert-base-uncased')
train_encodings = tokenizer(train_texts.tolist(), truncation=True, padding=True)
val_encodings = tokenizer(val_texts.tolist(), truncation=True, padding=True)

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
model = DistilBertForSequenceClassification.from_pretrained('distilbert-base-uncased', num_labels=len(labels))

training_args = TrainingArguments(
    output_dir='./results',
    num_train_epochs=1,
    per_device_train_batch_size=8,
    full_determinism=True  # Ensure reproducibility
    # logging_dir='./logs',
)

trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=train_dataset,
    eval_dataset=val_dataset
)

print("Starting Training...")
trainer.train()

# 4. Export to ONNX
print("Exporting to ONNX...")
dummy_input = tokenizer("example query", return_tensors="pt")
torch.onnx.export(
    model, 
    (dummy_input["input_ids"], dummy_input["attention_mask"]), 
    "intent.onnx",
    input_names=["input_ids", "attention_mask"],
    output_names=["logits"],
    dynamic_axes={"input_ids": {0: "batch_size", 1: "sequence_length"}, 
                  "attention_mask": {0: "batch_size", 1: "sequence_length"},
                  "logits": {0: "batch_size"}}
)
print("Saved to models/intent.onnx")
