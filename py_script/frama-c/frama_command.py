# use for extract frama-c impact analysis command
import os
import sys
import argparse


def read_compile_file(bc_dir):
    # read all the file in the dir_path
    file_list = os.listdir(bc_dir)
    print("BC File List: ")
    for file in file_list:
        print(file)
    print("-" * 50) 
    return file_list

def find_source_file_in_project_dir(project_dir, file_list):
    # find the compile file in the project dir
    source_files = []
    for bc_file in file_list:
        # replace the bc file with c file
        c_file = bc_file.replace(".bc", ".c")
        file_list[file_list.index(bc_file)] = c_file
    print("C File List: ")
    for c_file in file_list:
        print(c_file)
    print("-" * 50)    
        
    
    # check if the file is in the project dir
    for c_file in file_list:
        # recursively find the file in the project dir
        for root, dirs, files in os.walk(project_dir):
            if c_file in files:
                # get the absolute path
                source_files.append(os.path.join(root, c_file))
    print("Source Files: ")
    for src in source_files:
        print(src)
    return source_files

def extract_include(source_files):
    # extract the include file in the source file
    include = set()
    for source_file in source_files:
        with open(source_file, "r") as f:
            for line in f:
                if line.startswith("#include"):
                    include.add(line)
    return include

def argument_parser():
    usage_text = (
        "python %(prog)s -p <project_directory> -b <bc_directory>\n\n"
        "Example:\n"
        "  python %(prog)s -p /path/to/project -b /path/to/bc"
    )
    parser = argparse.ArgumentParser(
        description="Extract the include file in the source file",
        usage=usage_text,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("-p", help="The project directory", required=True)
    parser.add_argument("-b", help="The directory of the bc file", required=True)
    args = parser.parse_args()
    return args

def remove_include(include):
    return {line.replace("#include", "").strip() for line in include}

def remove_comment(include):
    new_include = set()
    for inc in include:
        if "//" in inc:
            inc = inc.split("//")[0]
        if "/*" in inc:
            inc = inc.split("/*")[0]
        stripped = inc.strip()
        if stripped != "":
            new_include.add(stripped)
    return new_include

def find_include_dirs(project_dir):
    include_dirs = set()
    for root, dirs, files in os.walk(project_dir):
        for dir in dirs:
            include_dirs.add(os.path.join(root, dir))
    print("Include directories:")
    for d in include_dirs:
        print(d)
    return include_dirs

def find_external_include_dirs(project_dir, include_set):
    remove_items = set()
    include_set = set()
    
    for inc in include_set:
        if (inc.startswith("<") and inc.endswith(">")) or (inc.startswith('"') and inc.endswith('"')):
            header = inc[1:-1]
        else:
            header = inc
        header = os.path.basename(header)

        for root, dirs, files in os.walk(project_dir):
            if header in files:
                remove_items.add(inc)
                break
    filtered_include_set = include_set - remove_items

    print("After Step 1, the following include files are considered external:")
    for inc in filtered_include_set:
        print(inc)

    remove_items_basename = set()
    for inc in filtered_include_set:
        header = os.path.basename(inc)
        for root, dirs, files in os.walk(project_dir):
            if header in files:
                remove_items_basename.add(inc)
                break
    filtered_include_set = filtered_include_set - remove_items_basename

    print("After Step 2, the remaining external include files are:")
    for inc in filtered_include_set:
        print(inc)

    external_include_dirs = set()
    for inc in filtered_include_set:
        for root, dirs, files in os.walk("/"):
            if inc in files:
                external_include_dirs.add(root)
                break
    print("External include file directories:")
    for d in external_include_dirs:
        print(d)
    return external_include_dirs

def construct_frama_c_impact_command(project_dir, include_dirs, external_include_dirs, mode="print", func_list=None):
    source_files = []
    for root, dirs, files in os.walk(project_dir):
        for file in files:
            if file.endswith(".c"):
                source_files.append(os.path.abspath(os.path.join(root, file)))
    
    include_dirs_str = " ".join([f"-I {d}" for d in include_dirs])
    external_include_dirs_str = " ".join([f"-I {d}" for d in external_include_dirs])
    cpp_extra_args = include_dirs_str + " " + external_include_dirs_str
    
    if mode == "print":
        mode_option = "-impact-print"
    elif mode == "pragma":
        if func_list and len(func_list) > 0:
            funcs = ",".join(func_list)
            mode_option = f"-impact-pragma {funcs}"
        else:
            mode_option = "-impact-pragma"
    else:
        raise ValueError("mode error, use 'print' or 'pragma'")
    
    command = f"frama-c {mode_option} -kernel-msg-key pp -cpp-extra-args=\"{cpp_extra_args}\" " + " ".join(source_files)
    
    return command

def main():
    # get the project dir
    args = argument_parser()
    project_dir = args.p
    bc_dir = args.b
    # read all the file in the project dir
    file_list = read_compile_file(bc_dir)
    # find the source file in the project dir
    source_files = find_source_file_in_project_dir(project_dir, file_list)
    # extract the include file in the source file
    include = extract_include(source_files)
    # remove the include in each element
    include = remove_include(include)
    # remove the comment in each element
    include = remove_comment(include)
    # print the include file
    print(include)
    include_dirs = find_include_dirs(project_dir)
    external_include_dirs = find_external_include_dirs(project_dir, include)
    include_dirs = include_dirs.union(external_include_dirs)
    command = construct_frama_c_impact_command(project_dir, include_dirs, external_include_dirs)

    print("Frama-C Impact command:")
    print(command)
if __name__ == "__main__":
    main()
