from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import json
import os
from chain import run_chain

app = FastAPI()

from typing import Dict, Any

class NLQRequest(BaseModel):
    query: str
    schema: Dict[str, Any]

@app.post("/nlq")
def nlq_handler(body: NLQRequest):
    try:
        # Check API Key
        if not os.environ.get("GOOGLE_API_KEY"):
             pass

        # Convert dict schema back to string for the prompt
        schema_str = json.dumps(body.schema)
        plan_json_str = run_chain(body.query, schema_str)
        
        # Validate JSON
        try:
            plan = json.loads(plan_json_str)
            return plan
        except json.JSONDecodeError:
            # Fallback or retry logic could go here, for now return error
            return {"error": "Failed to generate valid JSON", "raw": plan_json_str}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
