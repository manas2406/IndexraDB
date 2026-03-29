import os
from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_core.prompts import PromptTemplate
from langchain_core.output_parsers import StrOutputParser
from dotenv import load_dotenv

load_dotenv()

# Load prompt
with open("prompts/planner_prompt.txt", "r") as f:
    template = f.read()

prompt = PromptTemplate(
    template=template,
    input_variables=["schema", "query"]
)

# Initialize LLM
llm = ChatGoogleGenerativeAI(
    model="gemini-2.5-flash",
    temperature=0.1
)

# Use LCEL
nlq_chain = prompt | llm | StrOutputParser()

def run_chain(user_query: str, schema_json: str):
    # invoke returns string directly with StrOutputParser
    response = nlq_chain.invoke({
        "query": user_query, 
        "schema": schema_json
    })
    
    # Clean possible markdown code blocks
    content = response
    if "```json" in content:
        content = content.replace("```json", "").replace("```", "")
    elif "```" in content:
        content = content.replace("```", "")
        
    return content.strip()
