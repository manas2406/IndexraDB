from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import json
import os

# Lazy import: chain module requires langchain-groq which may not be installed
try:
    from chain import run_chain
    _chain_available = True
except ImportError:
    _chain_available = False
    print("[WARN] langchain-groq not installed. /nlq endpoint will be unavailable.")

from ml_inference import predict_intent, predict_slots


app = FastAPI()

from typing import Dict, Any

class NLQRequest(BaseModel):
    query: str
    schema: Dict[str, Any]

@app.post("/nlq")
def nlq_handler(body: NLQRequest):
    if not _chain_available:
        raise HTTPException(status_code=503, detail="LLM chain not available. Install langchain-groq.")
    try:
        # Check API Key
        if not os.environ.get("GROQ_API_KEY"):
             pass

        # Convert dict schema back to string for the prompt
        schema_str = json.dumps(body.schema)
        plan_json_str = run_chain(body.query, schema_str)
        
        # Validate JSON
        try:
            plan = json.loads(plan_json_str)
            return plan
        except json.JSONDecodeError:
            return {"error": "Failed to generate valid JSON", "raw": plan_json_str}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


class NLQMLRequest(BaseModel):
    query: str

@app.post("/nlq_ml")
def nlq_ml_handler(body: NLQMLRequest):
    """ONNX ML-based NLQ endpoint (no LLM needed)."""
    try:
        intent = predict_intent(body.query)
        slots = predict_slots(body.query)
        return {
            "intent": intent,
            "slots": slots
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
