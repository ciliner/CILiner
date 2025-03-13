import clang.cindex
import os
import sys
import multiprocessing
import psutil

llvm_to_cpp_type_mapping = {
    'i1': 'bool',
    'i8': 'char',
    'i16': 'short',
    'i32': 'int',
    'i64': 'long long',
    'float': 'float',
    'double': 'double',
    'i8*': 'char*',
    'i32*': 'int*',
    'void': 'void',
    'void*': 'void*',
    '[10 x i32]': 'int[10]',
    '{i32, i8}': 'struct { int, char; }',
    '<4 x i32>': '__m128i'
}

cwd = os.getcwd()
libclang_path = cwd + '/libclang.so'
clang.cindex.Config.set_library_file(libclang_path)
index = clang.cindex.Index.create()

def get_c_type(llvm_type):
    return llvm_to_cpp_type_mapping.get(llvm_type, "Unknown Type")

def find_all_header_files(project_path):
    header_files = []
    for root, dirs, files in os.walk(project_path):
        for file in files:
            if file.endswith('.c'):
                header_files.append(os.path.join(root, file))
    # print(header_files)
    return header_files

def find_func_file_mapping_parallel(header_files_chunk):
    func_file_mapping = {}
    index = clang.cindex.Index.create()
    for header_file in header_files_chunk:
        tu = index.parse(header_file, args=['-x', 'c++', '-std=c++14'])  # 确保编译参数与项目设置匹配
        for node in tu.cursor.walk_preorder():
            if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
                if node.is_definition():
                    func_name = node.spelling
                    # 将函数名和文件路径存储在字典中
                    func_file_mapping[func_name] = header_file
    return func_file_mapping

def remove_parent_dir(input_str):
    if "/../" in input_str:
        list = input_str.split('/')
        index = list.index('..')
        list.pop(index)
        list.pop(index-1)
        return '/'.join(list)
    else:
        return input_str
    
def parse_impact_result(func_file_mapping):
    dict = {}
    impact_result_path = os.getcwd() + '/result/impact_result.txt'
    with open(impact_result_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip()
            impact_file = line.split(':')[0]
            impact_file = remove_parent_dir(impact_file)
            impact_func = line.split(':')[1]
            impact_line_no = line.split(':')[2]
            if impact_func in func_file_mapping:
                if impact_file in dict:
                    dict[impact_file].append(int(impact_line_no))
                else:
                    dict[impact_file] = [int(impact_line_no)]
            else:
                print(f"Function {impact_func} not found in mapping, {impact_file}, use the original file path.")
                if impact_file in dict:
                    dict[impact_file].append(int(impact_line_no))
                else:
                    dict[impact_file] = [int(impact_line_no)]
    sorted_dict = {key: sorted(value) for key, value in dict.items()}
    return sorted_dict
                
def extract_source_code(impact_dict, source_file_list: list):
    final_lines = []
    file_dict = {}
    for key in impact_dict.keys():
        for source_file in source_file_list:
            if key in source_file:
                file_dict[key] = source_file
    for key in impact_dict.keys():
        file_path = file_dict[key]
        print("file path: " + file_path)
        print("-----------------------------------")
        impact_dict[key] = list(set(impact_dict[key]))
        lines_check = set(impact_dict[key])
        sorted_lines = sorted(lines_check)
        impacted_line_no = 0
        with open(file_path, 'r') as f:
            lines = f.readlines()
            for line_no in sorted_lines:
                line = "File: " + file_path + ' Line: ' + str(line_no) + ' Code: ' + lines[int(line_no)-1]
                print(line)
                impacted_line_no += 1
                final_lines.append(line)
                if int(line_no) > len(lines):
                    print("Error: Line number is out of range.")
                    break
    print('-----------------------------------')
    print("Total", len(final_lines), "lines of code are impacted.")
    return final_lines

def count_ir_lines():
    impacts_path = os.getcwd() + '/result/tempInfo/impacts.txt'
    lines_set = set()
    with open(impacts_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith('Original Instruction:'):
                continue
            elif line.startswith('Impacted Instructions:'):
                continue
            elif not line.strip():
                continue
            else:
                lines_set.add(line)
    print("Total", len(lines_set), "lines of LLVM IR are impacted.")
    print('-----------------------------------')
    print("Final results has been written to ./result/final_result.txt.")
    return len(lines_set)

def write_to_file(original_line, final_lines, ir_line_no):
    final_result_path = os.getcwd() + '/result/final_result.txt'
    with open(final_result_path, 'w') as f:
        if original_line:
            f.write(original_line + '\n')
            f.write('-----------------------------------\n')
            for line in final_lines:
                f.write(line)
                f.write('\n')
            f.write('-----------------------------------\n')
            f.write("Total " + str(len(final_lines)) + " lines of code are impacted.")
            f.write('\n')
            f.write('-----------------------------------\n')
            f.write("Total " + str(ir_line_no) + " lines of LLVM IR are impacted.")
        else:
            print("Error: Original line is empty.")
            f.write("Error: Original line is empty.")
            f.write('\n')
            f.write('-----------------------------------\n')
            for line in final_lines:
                f.write(line)
                f.write('\n')
            f.write('-----------------------------------\n')
            f.write("Total " + str(len(final_lines)) + " lines of code are impacted.")
            f.write('\n')
            f.write('-----------------------------------\n')
            f.write("Total " + str(ir_line_no) + " lines of LLVM IR are impacted.")
        
def extract_original_line():
    file_name = ''
    line_no = 0
    with open(os.getcwd() + '/result/diff_result.txt', 'r') as f:
        lines = f.readlines()
        for line in lines:
            file_name = line.split(':')[0]
            line_no = line.split(':')[1]
            file_name = file_name.replace('.bc', '.c')
            break
    dataset_path = os.getcwd() + '/dataset/'
    for root, dirs, files in os.walk(dataset_path):
        for file in files:
            if file == file_name:
                file_path = os.path.join(root, file)
                print("Original file path: " + file_path)
                print("Original line number: " + line_no)
                with open(file_path, 'r') as f:
                    lines = f.readlines()
                    original_line = lines[int(line_no)-1]
                    original_line = original_line.strip()
                    result = "Original line: " + original_line + " File: " + file_path + " Line: " + line_no
                    print(result)
                    return result
                
                
def get_available_cpu_count():
    total_cpus = multiprocessing.cpu_count()
    
    load_avg = psutil.getloadavg()[0]
    
    recommended_cpus = max(1, min(total_cpus, int(total_cpus - load_avg)))
    
    return recommended_cpus



def main():
    project_path = sys.argv[1]
    original_line = extract_original_line()
    header_path_list = find_all_header_files(project_path)
    cpu_count = multiprocessing.cpu_count()
    header_chunks = [header_path_list[i::cpu_count] for i in range(cpu_count)]  
    with multiprocessing.Pool(cpu_count) as pool:
        func_file_mappings = pool.map(find_func_file_mapping_parallel, header_chunks)
    func_file_mapping = {}
    for mapping in func_file_mappings:
        func_file_mapping.update(mapping)
    impact_dict = parse_impact_result(func_file_mapping)
    final_lines = extract_source_code(impact_dict, header_path_list)
    ir_line_no = count_ir_lines()
    write_to_file(original_line, final_lines, ir_line_no)
    

if __name__ == '__main__':
    main()
    

