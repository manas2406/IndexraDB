import subprocess
import time
import re

def run_test():
    proc = subprocess.Popen(
        ['./db_engine'], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        cwd='/home/vulture/codefiles/vdb/VultureDB'
    )
    
    def send(inp):
        # time.sleep(0.05)
        proc.stdin.write(inp + "\n")
        proc.stdin.flush()

    try:
        # 1. Create Table via NLQ
        # "create table items with fields id name price"
        send("14"); send("create table items with fields id name price")

        # 2. Insert Data via NLQ
        # "add to items 1 apple 100"
        send("14"); send("add to items 1 apple 100")
        
        # "insert into items values 2 pixel 900"
        send("14"); send("insert into items values 2 pixel 900")

        # 3. Select via NLQ
        # "show me items above 500" -> Should find pixel
        send("14"); send("show me items above 500")

        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        if "pixel" in out and "900" in out:
             print("[PASS] NLQ Select found target record")
        else:
             print("[FAIL] NLQ Select failed")

        if "apple" not in out: # Should be filtered out
             print("[PASS] NLQ Filtering worked (excluded cheaper item)")
        else:
             print("[FAIL] NLQ Filtering failed (included cheap item)")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
