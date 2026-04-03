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
        # --- Regression Test ---
        # y = 2x
        send("1"); send("reg_data"); send("x y .")
        for i in range(1, 4): # (1,2), (2,4), (3,6)
            send("2"); send("reg_data")
            send(f"{i}"); send(f"{i*2}")

        # Predict for x=5 -> Should be 10
        send("12")
        send("reg_data")
        send("y"); send("x"); send("5")

        # --- Clustering Test ---
        send("1"); send("cluster_data"); send("a b .")
        # Group 1: Near (0,0)
        send("2"); send("cluster_data"); send("0"); send("0")
        send("2"); send("cluster_data"); send("1"); send("1")
        # Group 2: Near (10,10)
        send("2"); send("cluster_data"); send("10"); send("10")
        send("2"); send("cluster_data"); send("11"); send("11")

        # Cluster K=2
        send("13")
        send("cluster_data")
        send("2")
        send("a b .")

        send("0") # Exit
        
        out, err = proc.communicate(timeout=3)
        print(out)

        # Check Regression
        # Look for "Predicted y: 10" (floating point tolerance needed?)
        match = re.search(r"Predicted y: ([\d\.]+)", out)
        if match:
            val = float(match.group(1))
            if 9.9 < val < 10.1:
                print("\n[PASS] Regression Prediction Accurate (Expected 10, Got {:.2f})".format(val))
            else:
                print("\n[FAIL] Regression Prediction Inaccurate (Got {:.2f})".format(val))
        else:
             print("\n[FAIL] Regression Output Not Found")

        # Check Clustering
        if "Cluster 0" in out and "Cluster 1" in out:
             print("[PASS] Clustering Output Generated")
        else:
             print("[FAIL] Clustering Output Missing")

    except Exception as e:
        print(f"Error: {e}")
        proc.kill()

if __name__ == "__main__":
    run_test()
