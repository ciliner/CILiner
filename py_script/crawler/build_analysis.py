import os
import subprocess

# Define the build configuration files to check
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

# Define the repository directory and initialize counters
repos_dir = os.path.join(os.getcwd(), "result/repos")
total_repos = 0
build_file_counts = {file: 0 for file in build_files}  # Initialize count for each file type to 0
repos_with_build_system = 0  # Counter for repositories that contain at least one build file
build_failures = {}  # Dictionary to record build failure reasons

# Iterate through all repository folders
for repo_name in os.listdir(repos_dir):
    repo_path = os.path.join(repos_dir, repo_name)
    if not os.path.isdir(repo_path):
        continue  # Skip non-directory items
    
    total_repos += 1  # Count total repositories
    repo_files = set()
    
    # Traverse all files in the repository to check for build configuration files
    for root, _, files in os.walk(repo_path):
        repo_files.update(files)
    
    # Flag to indicate whether the repository contains any build file
    has_build_system = False
    for build_file in build_files:
        if build_file in repo_files:
            build_file_counts[build_file] += 1
            has_build_system = True  # Mark as True if any build file is found
    
    if has_build_system:
        repos_with_build_system += 1  # Count repositories that contain at least one build file

        # Attempt to build the project and capture errors
        try:
            # Assume the project is built using "make" or "cmake"
            if "Makefile" in repo_files:
                result = subprocess.run(["make", "-C", repo_path], capture_output=True, text=True, check=True)
            elif "CMakeLists.txt" in repo_files:
                build_dir = os.path.join(repo_path, "build")
                os.makedirs(build_dir, exist_ok=True)
                subprocess.run(["cmake", ".."], cwd=build_dir, capture_output=True, text=True, check=True)
                result = subprocess.run(["make"], cwd=build_dir, capture_output=True, text=True, check=True)
            else:
                continue  # Skip if no buildable file is detected
            
            # Print success message if the build is successful
            print(f"Repository '{repo_name}' built successfully.")
        
        except subprocess.CalledProcessError as e:
            # If the build fails, analyze the error reason
            error_message = e.stderr.strip().split('\n')[-1]  # Capture the last line of the error message
            build_failures[error_message] = build_failures.get(error_message, 0) + 1
            print(f"Repository '{repo_name}' build failed with error: {error_message}")

# Output build file statistics
print(f"Total number of repositories: {total_repos}")
for build_file, count in build_file_counts.items():
    percentage = (count / total_repos * 100) if total_repos > 0 else 0
    print(f"Number of repositories containing {build_file}: {count} ({percentage:.2f}%)")

# Output the percentage of repositories containing at least one build system file
build_system_percentage = (repos_with_build_system / total_repos * 100) if total_repos > 0 else 0
print(f"Percentage of repositories containing at least one build system: {build_system_percentage:.2f}%")

# Output build failure reasons
print("\nBuild failure reasons:")
for error, count in build_failures.items():
    print(f"{error}: {count} occurrences")
