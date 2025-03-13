import subprocess
import threading
import time
import os
import sys

def monitor_memory(target_keyword, memory_info):
    """
    Monitor memory usage of processes containing a specific keyword.
    """
    peak_memory = 0  # Track peak memory usage
    while memory_info["running"]:
        try:
            result = subprocess.run(
                ["ps", "aux"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            for line in result.stdout.splitlines():
                if target_keyword in line:
                    # Split columns and filter empty entries
                    columns = [col for col in line.split() if col]
                    if len(columns) > 6:  # Ensure column 6 (RSS) exists
                        memory_kb = columns[5]  # RSS in KB
                        memory_kb = "".join([c for c in memory_kb if c.isdigit()])  # Remove non-digit chars
                        if memory_kb.isdigit():
                            memory_mb = float(memory_kb) / 1024  # Convert to MB
                            peak_memory = max(peak_memory, memory_mb)
                            memory_info["current_memory"] = memory_mb
            time.sleep(0.1)  # Monitor every 100ms
        except Exception as e:
            print(f"Memory monitoring error: {e}")
    
    memory_info["peak_memory"] = peak_memory  # Record final peak memory

def run_command(command, memory_info):
    memory_info["running"] = True
    memory_info["current_memory"] = 0
    memory_info["peak_memory"] = 0

    monitor_thread = threading.Thread(
        target=monitor_memory,
        args=("impact_analysis", memory_info)
    )
    monitor_thread.start()

    start_time = time.time()
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    end_time = time.time()

    memory_info["running"] = False
    monitor_thread.join()

    execution_time = end_time - start_time
    return stdout.decode(), stderr.decode(), execution_time, process.returncode

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 run.py <command>")
        sys.exit(1)

    command = " ".join(sys.argv[1:])
    memory_info = {}

    stdout, stderr, execution_time, return_code = run_command(command, memory_info)

    print(f"\n--- Result ---")
    print(f"Command: {command}")
    print(f"Command Output:\n{stdout}")
    print(f"Command Error (if any):\n{stderr}")
    print(f"Return Code: {return_code}")
    print(f"Peak Memory Usage: {memory_info['peak_memory']} MB")
    print(f"Execution Time: {execution_time:.6f} s")
