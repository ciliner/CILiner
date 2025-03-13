#!/usr/bin/env python3
import os
import sys
import subprocess
import glob
import argparse

def combine_bc_files(bc_dir):
    """
    Link all .bc files in the specified directory using llvm-link.
    The output file 'combined.bc' will be placed in the same directory.
    """
    if not os.path.isdir(bc_dir):
        print(f"Error: Directory '{bc_dir}' does not exist!")
        return False

    # Find all .bc files in the given directory
    bc_files = glob.glob(os.path.join(bc_dir, "*.bc"))
    if not bc_files:
        print(f"Error: No .bc files found in '{bc_dir}'!")
        return False

    print(f"Found {len(bc_files)} .bc files in '{bc_dir}'")

    # Build the llvm-link command with the --internalize option
    output_file = os.path.join(bc_dir, "combined.bc")
    link_cmd = ["llvm-link", "-o", output_file, "--internalize"] + bc_files

    try:
        print("Linking .bc files with llvm-link...")
        subprocess.run(link_cmd, check=True)
        print(f"Success: Combined bitcode file generated at {output_file}")
        return True
    except subprocess.CalledProcessError as e:
        print("Error: llvm-link failed!")
        print("Error message:", e)
        return False

def main():
    parser = argparse.ArgumentParser(
        description="Link all LLVM bitcode (.bc) files in a given directory using llvm-link."
    )
    parser.add_argument("bc_dir", help="Directory containing .bc files to be linked")
    args = parser.parse_args()

    combine_bc_files(args.bc_dir)

if __name__ == "__main__":
    main()
