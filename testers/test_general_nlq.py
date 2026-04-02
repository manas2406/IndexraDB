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
        # 1. Complex Create
        # "I want a table named gen with fields id price"
        send("14"); send("I want a table named gen with fields id price")

        # 2. Complex Insert
        # "Please put into gen values 1 090"
        send("14"); send("Please put into gen values 1 090")

        # 3. Complex Select
        # "Can you find me gen above 050"
        send("14"); send("Can you find me gen above 050")

        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        if "Table 'gen' created via NLQ" in out:
             print("[PASS] General Create")
        else:
             print("[FAIL] General Create")

        if "Record inserted into 'gen'" in out:
             print("[PASS] General Insert")
        else:
             print("[FAIL] General Insert")

        if "id | price" in out or "090" in out: 
             print("[PASS] General Select")
        else:
             print("[FAIL] General Select")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
