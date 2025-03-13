import os
import argparse

def count_code_lines(directory):
    c_cpp_files = 0
    total_lines = 0
    extensions = ['.c', '.cpp', '.h', '.hpp', 'cc']  # C/C++ file extensions

    for root, _, files in os.walk(directory):
        for file in files:
            if any(file.endswith(ext) for ext in extensions):
                c_cpp_files += 1
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        lines = f.readlines()
                        total_lines += len(lines)
                except (UnicodeDecodeError, FileNotFoundError):
                    print(f"Unable to read file: {file_path}")

    return c_cpp_files, total_lines

def main():
    parser = argparse.ArgumentParser(description="Count the number of C/C++ files and lines of code in a directory.")
    parser.add_argument("directory", type=str, help="Path to the directory to analyze")
    args = parser.parse_args()

    if os.path.exists(args.directory):
        file_count, line_count = count_code_lines(args.directory)
        print(f"The directory contains {file_count} C/C++ files with a total of {line_count} lines of code.")
    else:
        print("The directory does not exist. Please check the path!")

if __name__ == "__main__":
    main()
