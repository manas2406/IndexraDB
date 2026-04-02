import subprocess
import time

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
        proc.stdin.write(inp + "\n")
        proc.stdin.flush()

    try:
        # 1. Fuzzy Create
        # "crate tabl fuzz with filds price name"
        send("14"); send("crate tabl fuzz with filds price name")

        # 2. Fuzzy Insert
        # "add to fuzz 100 test" -> "add" is short, exact match needed? No, dist<=0 for len<=3.
        # "inser into fuzz valus 100 test"
        send("14"); send("inser into fuzz valus 90 test")

        # 3. Fuzzy Select
        # "shw me fuzz abov 50" (show, above)
        send("14"); send("shw me fuzz abov 50")

        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        if "Table 'fuzz' created via NLQ" in out:
             print("[PASS] Fuzzy Create")
        else:
             print("[FAIL] Fuzzy Create")

        if "Record inserted into 'fuzz'" in out:
             print("[PASS] Fuzzy Insert")
        else:
             print("[FAIL] Fuzzy Insert")

        if "Cond: price > 50" in out or "90" in out:
             print("[PASS] Fuzzy Select")
        else:
             print("[FAIL] Fuzzy Select")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
