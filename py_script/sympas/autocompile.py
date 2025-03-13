#!/usr/bin/env python3
import os
import sys
import json
import subprocess
import argparse
import shlex

def modify_command(args_list, out_dir=None):
    """
    Modify the compile command arguments:
      1. If the compiler ends with 'gcc', replace it with 'clang'.
         Similarly, if it ends with 'g++', replace with 'clang++';
         if it ends with 'cc', replace with 'clang';
         if it ends with 'c++', replace with 'clang++'.
      2. Insert '-emit-llvm' after the compiler executable if not present.
      3. Append '-Wno-unused-command-line-argument' to suppress warnings.
      4. Remove duplicate '-c' flags.
      5. Change the output file name so that its extension is forced to '.bc'.
      6. If out_dir is specified, adjust the output file path to be in out_dir.
    Returns the modified list of arguments.
    """
    # Replace compilers with clang/clang++ as needed, using endswith to cover non-absolute names.
    if args_list:
        compiler = args_list[0]
        if compiler.endswith("gcc"):
            print("Replacing '{}' with 'clang'".format(compiler))
            args_list[0] = "clang"
        elif compiler.endswith("g++"):
            print("Replacing '{}' with 'clang++'".format(compiler))
            args_list[0] = "clang++"
        elif compiler.endswith("cc"):
            print("Replacing '{}' with 'clang'".format(compiler))
            args_list[0] = "clang"
        elif compiler.endswith("c++"):
            print("Replacing '{}' with 'clang++'".format(compiler))
            args_list[0] = "clang++"

    # Insert '-emit-llvm' if not present
    if "-emit-llvm" not in args_list:
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
    
    # Modify the '-o' option: force output file name to have a .bc extension
    new_args = []
    skip_next = False
    for arg in new_args_no_dup:
        if skip_next:
            skip_next = False
            # Force output file name to have .bc extension regardless of original extension
            base = os.path.basename(arg)
            new_output = os.path.splitext(base)[0] + ".bc"
            if out_dir:
                os.makedirs(out_dir, exist_ok=True)
                new_output = os.path.join(out_dir, new_output)
            new_args.append(new_output)
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
    parser.add_argument("-o", "-b", "--out-dir", default="",
                        dest="o",
                        help="Directory to output .bc files (default: use original output directory)")
    args = parser.parse_args()

    json_file = args.c
    out_dir = args.o.strip()

    if not os.path.exists(json_file):
        print("{} does not exist! Please generate it first.".format(json_file))
        sys.exit(1)

    # Load compile_commands.json
    with open(json_file, "r") as f:
        compile_db = json.load(f)

    # Iterate through each compile command entry
    for entry in compile_db:
        directory = entry.get("directory")
        file_path = entry.get("file")
        # Use the 'command' field if available; otherwise try 'arguments'
        command_str = entry.get("command")
        if command_str is None:
            arguments = entry.get("arguments")
            if arguments is None:
                print("Warning: No command or arguments found for file {}. Skipping...".format(file_path))
                continue
        else:
            # Split the command string into a list of arguments
            arguments = shlex.split(command_str)
        
        # Modify the compile command to generate a .bc file with '-emit-llvm'
        new_args = modify_command(arguments.copy(), out_dir if out_dir else None)
        
        print("Compiling file {} in directory {}".format(file_path, directory))
        print("Executing command: {}".format(" ".join(new_args)))
        
        # Change to the specified directory and execute the modified command using check_call
        try:
            subprocess.check_call(new_args, cwd=directory)
            print("Success: {} compiled successfully.\n".format(file_path))
        except subprocess.CalledProcessError:
            print("Compilation failed for file: {}".format(file_path))
            sys.exit(1)
    
    print("All .bc files have been generated successfully!")

if __name__ == "__main__":
    main()
