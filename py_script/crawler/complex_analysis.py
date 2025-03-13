import os
from collections import Counter

# Set the base path to check
# Please change this to the that contains the directories you want to analyze


# List of build configuration files to check for
build_files = [
    "Makefile",
    "CMakeLists.txt",
    "configure",
    "meson.build",
    "SConstruct",
    "BUILD",
    "WORKSPACE",
    "wscript",
    "build.xml",
    "pom.xml",
    "build.gradle",
    "settings.gradle"
]

def count_c_file_lines(directory):
    """Count the total number of .c files and their total lines in a directory."""
    total_lines = 0
    c_file_count = 0
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.c'):
                file_path = os.path.join(root, file)
                # Check if the file exists
                if os.path.exists(file_path):
                    # Check if it is a symbolic link
                    if os.path.islink(file_path):
                        # Resolve the symbolic link
                        real_path = os.path.realpath(file_path)
                        # Check if the target of the symbolic link exists and is a file
                        if os.path.exists(real_path) and os.path.isfile(real_path):
                            try:
                                with open(real_path, 'r', encoding='utf-8', errors='ignore') as f:
                                    lines = f.readlines()
                                    total_lines += len(lines)
                                    c_file_count += 1
                            except Exception as e:
                                print(f"Unable to read file {real_path}: {e}")
                        else:
                            print(f"Symbolic link {file_path} points to a non-existent file, skipping.")
                    elif os.path.isfile(file_path):
                        # For regular files, read directly
                        try:
                            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                                lines = f.readlines()
                                total_lines += len(lines)
                                c_file_count += 1
                        except Exception as e:
                            print(f"Unable to read file {file_path}: {e}")
                    else:
                        print(f"File {file_path} is not a regular file, skipping.")
                else:
                    print(f"File {file_path} does not exist, skipping.")
    return c_file_count, total_lines

def find_build_files(directory):
    """Find the build configuration files present in the directory and return a set of them."""
    found_build_files = set()
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file in build_files:
                found_build_files.add(file)
    return found_build_files

def main():
    total_folders = 0
    folders_with_c_files = 0
    folders_meeting_all_conditions = 0
    build_file_counter = Counter()

    # Iterate through each direct subfolder under base_path
    for folder in os.listdir(base_path):
        folder_path = os.path.join(base_path, folder)

        # Check if the current path is a directory
        if os.path.isdir(folder_path):
            total_folders += 1  # Total number of directories

            # Count the number of .c files and their total lines
            c_file_count, total_lines = count_c_file_lines(folder_path)

            # Check if there are at least two .c files and the total lines exceed 20,000
            if c_file_count >= 2 and total_lines > 20000:
                folders_with_c_files += 1  # Directories meeting the .c file conditions

                # Find build configuration files in the directory
                found_build_files = find_build_files(folder_path)
                if found_build_files:
                    folders_meeting_all_conditions += 1  # Directories meeting all conditions
                    build_file_counter.update(found_build_files)

    # Output the statistics
    print(f"Total number of folders: {total_folders}")
    print(f"Number of directories with at least two .c files and more than 20,000 lines: {folders_with_c_files}")
    print(f"Number of directories meeting all conditions (at least two .c files, more than 20,000 lines, and containing build configuration files): {folders_meeting_all_conditions}\n")

    if folders_with_c_files > 0:
        # Calculate the percentage of directories with at least one build configuration file
        at_least_one_build_percentage = (folders_meeting_all_conditions / folders_with_c_files) * 100
        print(f"Percentage of directories with at least one build configuration file (based on directories meeting the .c file criteria): {at_least_one_build_percentage:.2f}%")

        # Detailed percentages for each build configuration file
        print("\nBuild configuration file percentages (among directories meeting the .c file criteria):")
        for build_file, count in build_file_counter.items():
            percentage = (count / folders_with_c_files) * 100
            print(f"{build_file}: {percentage:.2f}%")
    else:
        print("No directories with at least two .c files and more than 20,000 lines were found.")

if __name__ == '__main__':
    main()
