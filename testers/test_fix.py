import subprocess
import time

def run_test():
    # Use the newly built binary if accessible, but test script uses ./db_engine
    # I'll assumet the user wants me to update ./db_engine with the new build
    # For this test, let's target the binary we expect to use. 
    # If cmake builds 'vulturedb', I should probably point to that or copy it.
    
    proc = subprocess.Popen(
        ['./build/vulturedb'], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        cwd='/home/vulture/codefiles/vdb/VultureDB'
    )
    
    def send(inp):
        proc.stdin.write(inp + "\n")
        proc.stdin.flush()
        time.sleep(0.1)

    try:
        # Create table
        send("14"); send("create table student with fields id name")
        time.sleep(0.5)
        
        # Test the specific failure case
        send("14"); send("show table with name student")
        
        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        
        if "Table 'student'" in out: 
             # Logic: if it prints table info, it found it. 
             # If it failed, it might say "Table 'with' not found" or similar.
             pass 

        print(out)
        
        if "NLQ Error: Table 'with' not found" in out:
            print("[FAIL] Parsing still failed, found table 'with'")
        elif "Fields: id, name" in out:
             print("[PASS] Successfully found table 'student'")
        else:
             print("[INCONCLUSIVE] output: ", out)

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
