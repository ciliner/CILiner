import requests
import time
import random
import os
from datetime import datetime, timedelta

# GitHub token for authentication
# Please provide your own token here
GITHUB_TOKEN = "YOUR_GITHUB"

GITHUB_API_URL = "https://api.github.com"

# Pass authentication information to GitHub API
headers = {
    "Authorization": f"token {GITHUB_TOKEN}"
}

def random_delay():
    delay = random.uniform(1, 4)
    print(f"Waiting {delay:.2f} seconds...")
    time.sleep(delay)

def check_rate_limit():
    rate_limit_url = f"{GITHUB_API_URL}/rate_limit"
    try:
        response = requests.get(rate_limit_url, headers=headers)
        if response.status_code == 200:
            rate_limit_data = response.json()
            remaining = rate_limit_data['rate']['remaining']
            limit = rate_limit_data['rate']['limit']
            reset_time = rate_limit_data['rate']['reset']
            current_time = time.time()

            reset_time_formatted = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(reset_time))
            print(f"Current API rate limit: {limit}, remaining requests: {remaining}, reset time: {reset_time_formatted}")

            # If remaining requests are close to 0, wait until reset time
            if remaining <= 10:
                wait_time = reset_time - current_time
                if wait_time > 0:
                    print(f"API rate limit is about to be reached, waiting {int(wait_time)} seconds until reset.")
                    time.sleep(wait_time + 1)
        else:
            print(f"Failed to retrieve rate limit information, status code: {response.status_code}, reason: {response.text}")
    except Exception as e:
        print(f"Failed to retrieve rate limit information, exception: {str(e)}")

def get_repo_language_stats(full_name):
    """Get the code line counts for each language in the repository, especially for C language"""
    url = f"{GITHUB_API_URL}/repos/{full_name}/languages"

    try:
        random_delay()  # Add a random delay
        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            languages = response.json()
            print(f"Repository {full_name} language statistics: {languages}")  # Print language statistics
            c_code_lines = languages.get("C", 0)  # Get the number of C code lines, default to 0
            return c_code_lines
        else:
            print(f"Failed to retrieve repository language data: {full_name}, status code: {response.status_code}, reason: {response.text}")
            return 0
    except Exception as e:
        print(f"Failed to retrieve repository language data: {full_name}, exception: {str(e)}")
        return 0

def get_c_file_count(full_name):
    """Get the count of .c files in the repository"""
    url = f"{GITHUB_API_URL}/search/code?q=extension:c+repo:{full_name}"

    try:
        random_delay()  # Add a random delay
        response = requests.get(url, headers=headers)
        if response.status_code == 200:
            total_count = response.json().get("total_count", 0)  # Get the count of .c files
            print(f"Repository {full_name} .c file count: {total_count}")
            return total_count
        else:
            print(f"Failed to get .c file count for repository {full_name}, status code: {response.status_code}, reason: {response.text}")
            return 0
    except Exception as e:
        print(f"Failed to get .c file count for repository {full_name}, exception: {str(e)}")
        return 0

def search_c_language_repos(query, page=1, per_page=30):
    """Search for C language repositories"""
    params = {
        "q": query,
        "per_page": per_page,  # Number of repositories returned per page
        "page": page           # Current page number for the request
    }

    # Check rate limit and wait until reset if near the limit
    check_rate_limit()

    # Add a random delay before each request
    random_delay()

    response = requests.get(f"{GITHUB_API_URL}/search/repositories", headers=headers, params=params)
    
    print(f"Requesting page: {page}, status code: {response.status_code}")
    if response.status_code == 200:
        return response.json()
    else:
        print(f"Request failed, status code: {response.status_code}, response: {response.text}")
        return None

def write_result_to_file(result, file_path):
    """Write result to file"""
    with open(file_path, 'a', encoding='utf-8') as file:
        file.write(result + "\n")

def ensure_directory_and_file_exists(file_path):
    """Ensure that the target file and its parent directory exist"""
    dir_name = os.path.dirname(file_path)
    
    # If the directory doesn't exist, create it
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)
    
    # If the file doesn't exist, create an empty file
    if not os.path.exists(file_path):
        open(file_path, 'w', encoding='utf-8').close()

def save_progress(processed_pages, progress_file):
    """Save the list of processed page numbers"""
    with open(progress_file, 'w', encoding='utf-8') as file:
        file.write(','.join(map(str, processed_pages)))

def load_progress(progress_file):
    """Load the list of processed page numbers"""
    if os.path.exists(progress_file):
        with open(progress_file, 'r', encoding='utf-8') as file:
            content = file.read().strip()
            if content:
                try:
                    return list(map(int, content.split(',')))
                except ValueError:
                    print(f"Warning: The contents of file {progress_file} are not a valid integer list, reset to empty.")
                    return []
            else:
                print(f"Warning: The file {progress_file} is empty, reset to empty.")
                return []
    return []  # If the file doesn't exist, return an empty list

def get_total_count(query):
    """Get the total number of search results (not exceeding 1000)"""
    params = {
        "q": query,
        "per_page": 1  # Only fetch one record to get total_count
    }
    response = requests.get(f"{GITHUB_API_URL}/search/repositories", headers=headers, params=params)
    if response.status_code == 200:
        total_count = response.json().get("total_count", 0)
        return min(total_count, 1000)  # Maximum not exceeding 1000
    else:
        print(f"Unable to retrieve total count, status code: {response.status_code}, reason: {response.text}")
        return 0

def generate_date_ranges(start_date, end_date, delta_days):
    """Generate date ranges in reverse order, moving backwards in time."""
    ranges = []
    current_start = start_date
    while current_start > end_date:
        current_end = current_start - timedelta(days=delta_days)
        if current_end < end_date:
            current_end = end_date
        ranges.append((current_end, current_start))
        current_start = current_end - timedelta(days=1)
    return ranges

def main_crawler(query, start_date, end_date, delta_days, per_page, result_file_path, progress_file):
    date_ranges = generate_date_ranges(start_date, end_date, delta_days)
    processed_ranges = load_progress(progress_file)
    for date_range in date_ranges:
        if date_range in processed_ranges:
            print(f"Date range already processed: {date_range}")
            continue
        
        sub_query = f"{query} created:{date_range[0].strftime('%Y-%m-%d')}..{date_range[1].strftime('%Y-%m-%d')}"
        print(f"Processing date range: {date_range[1].strftime('%Y-%m-%d')} to {date_range[0].strftime('%Y-%m-%d')}")
        
        total_count = get_total_count(sub_query)
        if total_count == 0:
            print(f"No results for date range {date_range}.")
            processed_ranges.append(date_range)
            save_progress(processed_ranges, progress_file)
            continue

        max_pages = (total_count + per_page - 1) // per_page
        print(f"Total repos: {total_count}, Max pages: {max_pages}")
        
        for page in range(1, max_pages + 1):
            repos_response = search_c_language_repos(sub_query, page=page, per_page=per_page)
            if repos_response is None:
                print("Failed to retrieve repos, skipping date range.")
                break
            
            repos = repos_response.get("items", [])
            if not repos:
                break
            
            for repo in repos:
                full_name = repo["full_name"]
                repo_url = repo["html_url"]
                c_code_lines = get_repo_language_stats(full_name)
                c_file_count = get_c_file_count(full_name)
                    
                result = f"Repo: {full_name}, URL: {repo_url}, C Files: {c_file_count}, C Code Lines: {c_code_lines}"
                print(result)
                write_result_to_file(result, result_file_path)
        
        processed_ranges.append(date_range)
        save_progress(processed_ranges, progress_file)
        print(f"Processed date range: {date_range[1].strftime('%Y-%m-%d')} to {date_range[0].strftime('%Y-%m-%d')}")
        print("-" * 40)
        
def main():
    query = "language:C"
    per_page = 30
    result_file_path = "result/scrap/result.txt"
    progress_file = "result/scrap/progress.txt"
    
    start_date = datetime.today()
    end_date = datetime.strptime("2008-01-01", "%Y-%m-%d")
    delta_days = 30
    
    ensure_directory_and_file_exists(result_file_path)
    ensure_directory_and_file_exists(progress_file)
    
    main_crawler(query, start_date, end_date, delta_days, per_page, result_file_path, progress_file)
    print("Crawling completed.")

if __name__ == "__main__":
    main()
