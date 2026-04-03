import subprocess
import time

def run_test():
    proc = subprocess.Popen(
        ['./vulturedb'], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        cwd='/home/vulture/codefiles/vdb/VultureDB/build'
    )
    
    def send(inp):
        proc.stdin.write(inp + "\n")
        proc.stdin.flush()

    try:
        # Create table for testing
        # "create table orders fields id price date"
        send("1"); send("orders"); send("id price date .")
        
        # Insert Data
        # 1. id:1, price:5000, date:2025-10-10
        send("2"); send("orders"); send("1"); send("5000"); send("2025-10-10")
        # 2. id:2, price:1000, date:2025-10-01
        send("2"); send("orders"); send("2"); send("1000"); send("2025-10-01")

        # Test 1: Simple Select
        # "show me orders above 2000" (normalize -> select orders > 2000)
        # Should return ID 1
        send("14"); send("show me orders above 2000")

        # Test 2: Complex Select (Time + Currency normalization)
        # "get orders over 2k from last week" (normalize -> select orders > 2000 last_week)
        # Should return ID 1 (if date matches? wait, date is hardcoded above. 
        # "last week" from system time 2025-12-30 is > 2025-12-23.
        # My inserted dates are 2025-10. So last week will return nothing.
        # But parsing should work and print logic.
        send("14"); send("get orders over 2k from last week")

        # Test 3: Create via NLQ
        # "create a new table items with fields name price"
        send("14"); send("create a new table items with fields name price")

        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        if "1 | 5000" in out:
             print("[PASS] ML Select Simple")
        else:
             print("[FAIL] ML Select Simple")

        if "Executing SELECT on orders where price > 2000" in out:
             print("[PASS] ML Pipeline Parsing")
        else:
             print("[FAIL] ML Pipeline Parsing")

        if "Table 'items' created" in out:
             print("[PASS] ML Create")
        else:
             print("[FAIL] ML Create")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
