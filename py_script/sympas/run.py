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
            result = subprocess.Popen(
                ["ps", "aux"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            stdout, _ = result.communicate()
            for line in stdout.splitlines():
                if target_keyword in line.decode("utf-8"):
                    # Split columns and filter empty entries
                    columns = [col for col in line.decode("utf-8").split() if col]
                    if len(columns) > 6:  # Ensure column 6 (RSS) exists
                        memory_kb = columns[5]  # RSS in KB
                        memory_kb = "".join([c for c in memory_kb if c.isdigit()])  # Remove non-digit chars
                        if memory_kb.isdigit():
                            memory_mb = float(memory_kb) / 1024  # Convert to MB
                            peak_memory = max(peak_memory, memory_mb)
                            memory_info["current_memory"] = memory_mb
            time.sleep(0.1)  # Monitor every 100ms
        except Exception as e:
            print("Memory monitoring error: {}".format(e))
    
    memory_info["peak_memory"] = peak_memory  # Record final peak memory

def run_command(command, memory_info):
    """
    Run command and start memory monitoring thread.
    """
    # Initialize memory monitoring state
    memory_info["running"] = True
    memory_info["current_memory"] = 0
    memory_info["peak_memory"] = 0

    # Start memory monitoring thread
    monitor_thread = threading.Thread(
        target=monitor_memory,
        args=("llvm", memory_info)
    )
    monitor_thread.start()

    # Execute command
    start_time = time.time()
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    end_time = time.time()

    # Stop memory monitoring
    memory_info["running"] = False
    monitor_thread.join()

    execution_time = end_time - start_time  # Calculate execution time
    return stdout.decode("utf-8"), stderr.decode("utf-8"), execution_time, process.returncode

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 run.py <command>")
        sys.exit(1)

    command = " ".join(sys.argv[1:])  # Get user input command
    memory_info = {}  # Shared memory monitoring data

    # Execute command and monitor
    stdout, stderr, execution_time, return_code = run_command(command, memory_info)

    # Print results
    print("\n--- Execution Results ---")
    print("Command: {}".format(command))
    print("Command Output:\n{}".format(stdout))
    print("Command Error (if any):\n{}".format(stderr))
    print("Return Code: {}".format(return_code))
    print("Peak Memory Usage: {} MB".format(memory_info['peak_memory']))
    print("Execution Time: {:.6f} s".format(execution_time))
