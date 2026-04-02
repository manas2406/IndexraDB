import subprocess
import time

def run_test():
    proc = subprocess.Popen(
        ['./db_engine'], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        cwd='/home/vulture/codefiles/vdb/antigravity-db-bptree'
    )
    
    # Helper to send input
    def send(inp):
        print(f"> {inp}")
        proc.stdin.write(inp + "\n")
        proc.stdin.flush()
        time.sleep(0.1)

    try:
        # 1. Create Table
        send("1") # Option
        send("users") # Name
        send("id name .") # Fields
        
        # 2. Insert
        send("2")
        send("users")
        send("1") # id
        send("Alice") # name
        
        send("2")
        send("users")
        send("2")
        send("Bob")
        
        # 5. View
        send("5")
        send("users")
        
        # 6. Search
        send("6")
        send("users")
        send("name")
        send("Alice")
        
        # 10. Save
        send("10")
        
        # 0. Exit
        send("0")
        
        out, err = proc.communicate(timeout=2)
        print("--- Output ---")
        print(out)
        
        if "Match ID" in out and "Alice" in out:
            print("\n[PASS] Search found Alice")
        else:
            print("\n[FAIL] Search did not find Alice")
            
        if "Database saved" in out:
            print("[PASS] Save confirmed")
        else:
            print("[FAIL] Save not confirmed")
            
    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

    # --- Restart and Load ---
    print("\n--- Testing Persistence ---")
    proc = subprocess.Popen(
        ['./db_engine'], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        cwd='/home/vulture/codefiles/vdb/antigravity-db-bptree'
    )
    
    try:
        # 11. Load
        proc.stdin.write("11\n")
        proc.stdin.flush()
        time.sleep(0.5)
        
        # 5. View
        proc.stdin.write("5\n")
        proc.stdin.write("users\n")
        proc.stdin.flush()
        
        proc.stdin.write("0\n") # Exit
        out, err = proc.communicate(timeout=2)
        
        if "Alice" in out and "Bob" in out:
            print("[PASS] Persistence verified (Data loaded)")
        else:
            print("[FAIL] Persistence failed")
            print(out)

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
