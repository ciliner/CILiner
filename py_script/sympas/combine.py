#!/usr/bin/env python3
import os
import sys
import subprocess
import glob
import argparse

def internalize_bc_file(bc_file, out_dir):
    """
    Internalize a bitcode file using opt with the internalize and globaldce passes.
    The output file will have a _int.bc suffix.
    Returns the path to the internalized file.
    """
    base = os.path.basename(bc_file)
    name = os.path.splitext(base)[0]
    output_file = os.path.join(out_dir, name + "_int.bc")
    cmd = ["opt", "-internalize", "-globaldce", bc_file, "-o", output_file]
    print("Internalizing {} -> {}".format(bc_file, output_file))
    try:
        subprocess.check_call(cmd)
        return output_file
    except subprocess.CalledProcessError as e:
        print("Error: internalization failed for file {}. Message: {}".format(bc_file, e))
        sys.exit(1)

def combine_bc_files(bc_dir):
    """
    Internalize all .bc files in the specified directory using opt, then link
    the internalized bitcode files using llvm-link. The final output file
    'combined.bc' will be placed in the same directory.
    """
    if not os.path.isdir(bc_dir):
        print("Error: Directory '{}' does not exist!".format(bc_dir))
        return False

    # Find all .bc files in the given directory
    bc_files = glob.glob(os.path.join(bc_dir, "*.bc"))
    if not bc_files:
        print("Error: No .bc files found in '{}'!".format(bc_dir))
        return False

    print("Found {} .bc files in '{}'".format(len(bc_files), bc_dir))

    # Use the same directory for output internalized files
    out_dir = bc_dir

    # Internalize each .bc file
    internalized_files = []
    for bc_file in bc_files:
        int_file = internalize_bc_file(bc_file, out_dir)
        internalized_files.append(int_file)

    # Build the llvm-link command to combine the internalized bitcode files
    output_file = os.path.join(bc_dir, "combined.bc")
    link_cmd = ["llvm-link", "-o", output_file] + internalized_files

    try:
        print("Linking internalized .bc files with llvm-link...")
        subprocess.check_call(link_cmd)
        print("Success: Combined bitcode file generated at {}".format(output_file))
        return True
    except subprocess.CalledProcessError as e:
        print("Error: llvm-link failed!")
        print("Error message:", e)
        return False

def main():
    parser = argparse.ArgumentParser(
        description="Internalize and link all LLVM bitcode (.bc) files in a given directory using opt and llvm-link."
    )
    parser.add_argument("bc_dir", help="Directory containing .bc files to be processed and linked")
    args = parser.parse_args()

    combine_bc_files(args.bc_dir)

if __name__ == "__main__":
    main()
