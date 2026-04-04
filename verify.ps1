$env:GOOGLE_API_KEY="AIzaSyCeUfdPCwKHV7GrS_8tJkivZ9sFpELO-E8"

Write-Host "Starting Python Server..."
$serverProcess = Start-Process -FilePath "$PSScriptRoot\.venv\Scripts\python.exe" -ArgumentList "-m uvicorn app:app --port 8000" -WorkingDirectory "$PSScriptRoot\llm_server" -PassThru -NoNewWindow
# In production, redirect output, but for now we let it run silently.

Start-Sleep -Seconds 5

Write-Host "Running VultureDB..."
$inputData = @"
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
"@

$inputData | .\build\vulturedb.exe

Write-Host "Stopping Python Server..."
Stop-Process -Id $serverProcess.Id -Force
Write-Host "Done."
