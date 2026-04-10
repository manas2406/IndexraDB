FROM python:3.12-slim

# Set working directory to the llm_server directory inside /app
WORKDIR /app/llm_server

# Copy requirements from the llm_server folder
COPY llm_server/requirements.txt .

# Install dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy the entire project to /app to maintain relative paths like ../models/
COPY . /app

# Expose the port
EXPOSE 8000

# Run the FastAPI app
CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000"]
