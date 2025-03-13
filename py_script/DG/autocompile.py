#!/usr/bin/env python3
import os
import sys
import json
import subprocess
import argparse

def modify_command(args_list, out_dir=None):
    """
    Modify the compile command arguments:
      1. If the compiler is '/usr/bin/gcc', replace it with 'clang'.
      2. Insert '-emit-llvm' after the compiler executable if not present.
      3. Change the output file extension from .o to .bc for the '-o' option.
      4. Append '-Wno-unused-command-line-argument' to suppress warnings.
      5. Remove duplicate '-c' flags.
      6. If out_dir is specified, change the output file path to be in out_dir.
    Returns the modified list of arguments.
    """
    # Replace '/usr/bin/gcc' with 'clang' if needed
    if args_list and args_list[0] == "/usr/bin/gcc":
        print("Replacing '/usr/bin/gcc' with 'clang'")
        args_list[0] = "clang"

    # Insert '-emit-llvm' if not present
    if "-emit-llvm" not in args_list:
        # Assume the first argument is the compiler executable
        args_list.insert(1, "-emit-llvm")
    
    # Append flag to suppress unused command-line argument warning
    if "-Wno-unused-command-line-argument" not in args_list:
        args_list.append("-Wno-unused-command-line-argument")
    
    # Remove duplicate '-c' flags (keep the first occurrence)
    new_args_no_dup = []
    found_c = False
    for arg in args_list:
        if arg == "-c":
            if found_c:
                continue
            found_c = True
        new_args_no_dup.append(arg)
    
    new_args = []
    skip_next = False
    for i, arg in enumerate(new_args_no_dup):
        if skip_next:
            skip_next = False
            # Modify the output file name: change .o to .bc and adjust the directory if needed
            if arg.endswith(".o"):
                base = os.path.basename(arg)
                new_output = base[:-2] + ".bc"
                if out_dir:
                    os.makedirs(out_dir, exist_ok=True)
                    new_output = os.path.join(out_dir, new_output)
                arg = new_output
            new_args.append(arg)
            continue
        if arg == "-o":
            new_args.append(arg)
            skip_next = True  # The next argument is the output file name
        else:
            new_args.append(arg)
    return new_args

def main():
    parser = argparse.ArgumentParser(
        description="Build LLVM bitcode (.bc) files from compile_commands.json using clang."
    )
    parser.add_argument("-c", default="compile_commands.json",
                        help="Path to compile_commands.json (default: compile_commands.json)")
    parser.add_argument("-o", default="",
                        help="Directory to output .bc files (default: use original output directory)")
    args = parser.parse_args()

    json_file = args.c
    out_dir = args.o.strip()

    if not os.path.exists(json_file):
        print(f"{json_file} does not exist! Please generate it first.")
        sys.exit(1)

    # Load compile_commands.json
    with open(json_file, "r") as f:
        compile_db = json.load(f)

    # Iterate through each compile command entry
    for entry in compile_db:
        directory = entry.get("directory")
        file_path = entry.get("file")
        arguments = entry.get("arguments")
        
        # Modify the compile command to generate a .bc file with '-emit-llvm'
        new_args = modify_command(arguments.copy(), out_dir if out_dir else None)
        
        print(f"Compiling file {file_path} in directory {directory}")
        print("Executing command:", " ".join(new_args))
        
        # Change to the specified directory and execute the modified command
        try:
            subprocess.run(new_args, cwd=directory, check=True)
        except subprocess.CalledProcessError:
            print(f"Compilation failed for file: {file_path}")
            sys.exit(1)
    
    print("All .bc files have been generated successfully!")

if __name__ == "__main__":
    main()
