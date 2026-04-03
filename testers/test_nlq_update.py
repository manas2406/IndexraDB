import subprocess

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
        # Create table items
        send("14"); send("create table items with fields id name price")
        
        # Insert Data
        send("14"); send("insert into items values 1 mouse 50")
        
        # Update Data (NLQ)
        # "update items set price 100 where name mouse"
        send("14"); send("update items set price 100 where name mouse")
        
        # Verify
        send("14"); send("show me items where name mouse")
        
        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        if "100" in out and "mouse" in out:
             print("[PASS] NLQ Update")
        else:
             print("[FAIL] NLQ Update")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
