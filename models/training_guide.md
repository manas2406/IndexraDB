# ML Training Guide for VultureDB

This guide explains how to train the real Machine Learning models (Intent & Slots) using Python and PyTorch, and export them to ONNX for use in the C++ engine.

## Prerequisites
- Python 3.8+
- PyTorch
- Transformers (HuggingFace)
- Scikit-Learn
- Pandas

```bash
pip install torch transformers pandas sklearn
```

## 1. Intent Classification Model

**Goal**: Classify queries into `SELECT`, `CREATE`, `INSERT`, etc.
**Architecture**: DistilBERT (Fine-tuned)

1.  **Edit Data**: Add more examples to `models/data/intent_data.csv`.
2.  **Train**:
    ```bash
    cd models
    python3 train_intent.py
    ```
3.  **Output**: Generates `intent.onnx`.

## 2. Slot Extraction (NER) Model

**Goal**: Extract entities like `TABLE`, `FIELD`, `VALUE`.
**Architecture**: BERT Token Classification

1.  **Edit Data**: Add annotated examples to `models/data/slot_data.json`.
2.  **Train**:
    ```bash
    python3 train_slots.py
    ```
3.  **Output**: Generates `slots.onnx`.

## 3. Deployment

Once you have `intent.onnx` and `slots.onnx`:
1.  Place them in the `models/` directory.
2.  Update `src/nlq_ml/intent.cpp` and `slots.cpp` to use `onnxruntime` C++ API to load these models instead of the current heuristic logic.
