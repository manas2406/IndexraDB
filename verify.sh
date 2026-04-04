#!/bin/bash
export GOOGLE_API_KEY="AIzaSyCeUfdPCwKHV7GrS_8tJkivZ9sFpELO-E8"

# Start Python Server (Verified Venv)
echo "Starting Python Server..."
cd llm_server
venv/bin/uvicorn app:app --port 8000 > ../server.log 2>&1 &
SERVER_PID=$!
cd ..

# Wait for server
sleep 5

# Run VultureDB with input
echo "Running VultureDB..."
./build/vulturedb <<EOF
1
employees
name salary .
2
employees
Alice
60000
2
employees
Bob
40000
15
Show me employees with salary above 50000
0
EOF

# Kill Server
kill $SERVER_PID
echo "Done."
