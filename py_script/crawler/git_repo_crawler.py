import subprocess
import re
import os

repositories_dir = os.getcwd() + "/result/repos"
results_file = os.getcwd() + "/result/scrap/repo_results.txt"

os.makedirs(repositories_dir, exist_ok=True)

def read_repositories_from_file(file_path):
    urls = []
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            match = re.search(r"URL: (https?://\S+)", line)
            if match:
                url = match.group(1).rstrip(",")
                urls.append(url)
    return urls

def clone_repository(url):
    repo_name = url.split('/')[-1]
    clone_path = os.path.join(repositories_dir, repo_name)
    if os.path.isdir(clone_path):
        print(f"Repository {repo_name} already exists, skipping clone.")
        return clone_path
    try:
        subprocess.run(["git", "clone", url, clone_path], check=True)
        print(f"Successfully cloned repository: {repo_name}")
        return clone_path
    except subprocess.CalledProcessError:
        print(f"Failed to clone repository {repo_name}.")
        return None

def check_build_files(repo_path):
    build_files = {
        "Makefile": False,
        "configure": False,
        "CMakeLists.txt": False
    }
    for root, _, files in os.walk(repo_path):
        if "Makefile" in files:
            build_files["Makefile"] = True
        if "configure" in files:
            build_files["configure"] = True
        if "CMakeLists.txt" in files:
            build_files["CMakeLists.txt"] = True
    return build_files

def analyze_repositories(file_path):
    urls = read_repositories_from_file(file_path)
    with open(results_file, 'w', encoding='utf-8') as output:
        for url in urls:
            repo_path = clone_repository(url)
            if repo_path:
                build_files = check_build_files(repo_path)
                result = f"Repository: {url}, Makefile: {build_files['Makefile']}, configure: {build_files['configure']}, CMakeLists.txt: {build_files['CMakeLists.txt']}\n"
                print(result)
                output.write(result)

if __name__ == "__main__":
    input_file = os.getcwd() + "/result/scrap/result.txt"
    analyze_repositories(input_file)
