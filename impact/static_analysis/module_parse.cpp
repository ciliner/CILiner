#include "module_parse.h"

// 信号处理 - 用于 debug
void signalHandler(int signum)
{
    void *array[10];
    size_t size;

    // 获取 void*'s 的数组，每个都指向一个堆栈帧
    size = backtrace(array, 20);

    // 打印出所有堆栈帧的地址
    errs() << "Error: signal " << signum << ":\n";
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

bool IA::argsHandle(int argc, char **argv)
{
    bool flag = true;
    if (argc != 3 && argc != 4)
    {
        errs() << "Usage: <bcoutput dir> <file:inst_no> [-n]\n";
        exit(1);
    }
    commitBCDir = argv[1];

    std::filesystem::path commitBCPath(commitBCDir);
    if (!std::filesystem::exists(commitBCPath))
    {
        errs() << "Error: bcoutput dir not found\n";
        errs() << commitBCPath << "\n";
        exit(1);
    }

    std::string fileInstNo = argv[2];
    parseFileInstNo(fileInstNo);

    if (argc == 4)
    {
        if (strcmp(argv[3], "-n") != 0)
        {
            errs() << "Usage: <bcoutput dir> <file:inst_no> [-n]\n";
            exit(1);
        }
        flag = false;
    }
    return flag;
}

void IA::parseFileInstNo(std::string fileInstNo)
{
    std::string fileName;
    int lineNumber;
    std::string::size_type pos = fileInstNo.find(':');
    if (pos != std::string::npos)
    {
        fileName = fileInstNo.substr(0, pos);
        fileName.replace(fileName.find(".c"), 2, ".bc");
        std::string lineNumberStr = fileInstNo.substr(pos + 1);
        lineNumber = std::stoi(lineNumberStr);
    }
    else
    {
        errs() << "Error: file:inst_no format error\n";
        exit(1);
    }
    diffResult[fileName] = lineNumber;
    std::string workingDir = std::filesystem::current_path();
    std::string filePath = workingDir + "/result/diff_result.txt";
    std::ofstream file(filePath);
    if (!file)
    {
        file.open(filePath);
    }
    file << fileName << ":" << lineNumber << "\n";
}

void IA::parseFiles(LLVMContext &context)
{
    std::vector<std::string> commitBCPaths;
    for (const auto &entry : std::filesystem::directory_iterator(commitBCDir))
    {
        std::string filePath = entry.path().string();
        if (filePath.find(".bc") != std::string::npos)
        {
            commitBCPaths.push_back(filePath);
        }
    }
    for (const auto &filePath : commitBCPaths)
    {
        SMDiagnostic error;
        auto module = parseIRFile(filePath, error, context);
        if (!module)
        {
            errs() << "Error: failed to load " << filePath << "\n";
            exit(1);
        }
        else
        {
            // errs() << "Successfully loaded " << filePath << "\n";
        }
        commitBCModules.push_back(std::move(module));
    }

    errs() << "commitBCModules: " << commitBCModules.size() << "\n";

    for (const auto &module : commitBCModules)
    {
        for (auto &function : *module)
        {
            if (function.getName().str().find("llvm.dbg.declare") != std::string::npos)
            {
                continue;
            }
            commitBCFunctions.push_back(&function);
        }
    }
    // errs() << "commitOneFunctions: " << commitOneFunctions.size() << "\n";
    // errs() << "commitTwoFunctions: " << commitTwoFunctions.size() << "\n";
    errs() << "commitBCFunctions: " << commitBCFunctions.size() << "\n";
}

void IA::compareChanges()
{
    for (auto &i : diffResult)
    {
        std::string fileName = i.first;
        int lineNumbers = i.second;
        setOriginalFile(fileName);
        setOriginalLine(lineNumbers);
        for (auto &module : commitBCModules)
        {
            std::string moduleName = module->getName().str();
            std::string moduleFileName = moduleName.substr(moduleName.find_last_of('/') + 1);
            if (moduleFileName == fileName)
            {
                for (auto &function : *module)
                {
                    std::string functionName = function.getName().str();
                    for (auto &bb : function)
                    {
                        for (auto &inst : bb)
                        {
                            DILocation *loc = inst.getDebugLoc();
                            if (!loc)
                            {
                                continue;
                            }
                            unsigned line = loc->getLine();
                            if (lineNumbers == line)
                            {
                                errs() << "Changed Instruction: " << inst << "\n";
                                changedInstructions.push_back(&inst);
                            }
                        }
                    }
                }
            }
        }
    }
}

llvm::Function *IA::getChangedFunction(llvm::Module &module, const std::string fileName, std::string funcName)
{
    std::string moduleName = module.getName().str();
    std::string moduleFileName = moduleName.substr(moduleName.find_last_of('/') + 1);
    if (moduleFileName == fileName)
    {
        for (auto &f : module)
        {
            if (f.getName().str() == funcName)
            {
                // llvm::errs() << "Found function: " << f.getName() << "\n";
                return &f;
            }
        }
    }
    return nullptr;
}

void IA::getAllGVs()
{
    std::cout << "Get All Global Variables" << std::endl;
    for (auto &module : commitBCModules)
    {
        // errs() << "Module: " << module->getSourceFileName() << "\n";
        for (auto &gv : module->globals())
        {
            // errs() << "Global Variable: " << gv.getName() << "\n";
            // errs() << "In Module: " << gv.getParent()->getName() << "\n";
            // errs() << "Uses of this gv:" << gv.getNumUses() << "\n";
            // for (auto &use : gv.uses())
            // {
            //     errs() << "Use: " << *use.getUser() << "\n";
            // }
            // errs() << "--------------------------------------" << "\n";
            globalVariables.push_back(&gv);
        }
    }
    for (auto &gv : globalVariables)
    {
        std::vector<Instruction *> uses;
        for (auto &use : gv->uses())
        {
            // errs() << "Use: " << *use.getUser() << "\n";
            if (auto *inst = dyn_cast<Instruction>(use.getUser()))
            {
                uses.push_back(inst);
            }
            else
            {
                // errs() << "Error: use " << *use << " is not an instruction\n ";
            }
        }
        GlobalVariableInfo gvInfo = GlobalVariableInfo(gv, uses);
        globalVariableInfos.push_back(gvInfo);
    }
    std::cout << "Global Variables: " << globalVariables.size() << std::endl;
}

std::string IA::removeStructVersionNumber(const std::string &inst)
{
    std::string result;
    size_t lastPos = 0;
    size_t structPos = inst.find("%struct.", lastPos);

    while (structPos != std::string::npos)
    {
        result += inst.substr(lastPos, structPos - lastPos);
        size_t start = structPos + 8;
        size_t end = start;
        while (end < inst.length() && (isalnum(inst[end]) || inst[end] == '_'))
        {
            end++;
        }

        size_t versionStart = inst.find('.', end);
        if (versionStart != std::string::npos)
        {
            size_t versionEnd = versionStart + 1;
            while (versionEnd < inst.length() && isdigit(inst[versionEnd]))
            {
                versionEnd++;
            }
            if (versionEnd != versionStart + 1)
            {
                result += inst.substr(structPos, versionStart - structPos);
                lastPos = versionEnd;
            }
            else
            {
                result += inst.substr(structPos, end - structPos);
                lastPos = end;
            }
        }
        else
        {
            result += inst.substr(structPos, end - structPos);
            lastPos = end;
        }

        structPos = inst.find("%struct.", lastPos);
    }

    result += inst.substr(lastPos);

    size_t debugPos = result.find(", !dbg !");
    if (debugPos != std::string::npos)
    {
        result = result.substr(0, debugPos);
    }

    return result;
}

void IA::callAnalyze(CallInst *callStatement)
{
    if (auto *CI = dyn_cast<CallInst>(callStatement))
    {
        if (CI->isInlineAsm())
        {
            // asmCalls.push_back(CI);
            // errs() << "Asm call: " << *CI << "\n";
            // errs() << "Asm call Function name: " << CI->getParent()->getName() << "\n";
        }
        else if (CI->isIndirectCall() == true)
        {
            indirectCalls.push_back(CI);
            // errs() << "Indirect call: " << *CI << "\n";
            // errs() << "Indirect call Function name: " << CI->getParent()->getName() << "\n";
        }
        else if (CI->getCalledFunction() == nullptr)
        {
            indirectCalls.push_back(CI);
            // errs() << "Indirect call: " << *CI << "\n";
            // errs() << "Indirect call Function name: " << CI->getParent()->getName() << "\n";
        }
        else
        {
            directCalls.push_back(CI);
            // errs() << "Direct call: " << *CI << "\n";
            // errs() << "Direct call Function name: " << CI->getParent()->getName() << "\n";
            if (CI->getCalledFunction())
            {
                // errs() << "Direct call target: " << CI->getCalledFunction()->getName() << "\n";
            }
            else
            {
                // errs() << "Direct call target: Indirect call or target not available\n";
            }
        }
    }
}

void IA::analyzeAllCallInsts()
{
    std::cout << "Analyze All Call Insts" << std::endl;
    std::vector<Instruction *> indirectCalls;
    for (auto &module : commitBCModules)
    {
        for (auto &function : *module)
        {
            for (auto &bb : function)
            {
                for (auto &inst : bb)
                {
                    if (auto callInst = dyn_cast<CallInst>(&inst))
                    {
                        // errs() << "CallInst: " << *callInst << "\n";

                        std::string instStr;
                        llvm::raw_string_ostream rso(instStr);
                        callInst->print(rso);
                        rso.flush();
                        if (instStr.find("call void @llvm.dbg.declare(") != std::string::npos)
                        {
                            continue;
                        }
                        callAnalyze(callInst);
                    }
                }
            }
        }
    }
}

// 传入一个 llvm 函数，返回该函数的所有参数类型
std::vector<Type *> getFuncParameterTypes(Function *function)
{
    std::vector<Type *> parameterTypes;
    if (!function)
    {
        return parameterTypes; // 如果传入的函数为空，则返回空列表
    }

    for (auto &arg : function->args())
    {
        parameterTypes.push_back(arg.getType());
    }

    return parameterTypes;
}

// 将 IA 类中的所有 indirectCallInfos 转换为 functionCaller
void IA::parseICallInfos()
{
    // 首先遍历所有的 iCallInfos
    for (auto &info : iCallInfos)
    {
        // 获取其 callStatement
        Instruction *inst = info.getCallInst();
        // 获取其可能的 possibleCallees
        std::vector<Function *> possibleCallees = info.getPossibleCallees();

        // 遍历所有的 possibleCallees
        for (auto &callee : possibleCallees)
        {
            // 首先检查 callee 是否为空
            if (!callee)
            {
                continue;
            }
            // 然后检查在 functionCallers 中是否已经存在该 callee
            bool found = false;
            for (auto &caller : functionCallers)
            {
                if (caller.getFunction() == callee)
                {
                    // 如果找到，则将 inst 加入到该 caller 的 callStatements 中
                    caller.addPossibleCaller(inst);
                    found = true;
                    break;
                }
            }
            // 如果没有找到，则创建一个新的 functionCaller
            if (!found)
            {
                FunctionCaller caller(callee);
                caller.addPossibleCaller(inst);
                functionCallers.push_back(caller);
            }
        }
    }
    // 打印 functionCallers
    // for (auto &caller : functionCallers)
    // {
    //     errs() << "Function: " << caller.getFunction()->getName() << "\n";
    //     errs() << "Callers: \n";
    //     for (auto &inst : caller.getPossibleCallers())
    //     {
    //         errs() << *inst << "\n";
    //     }
    //     errs() << "--------------------------------------" << "\n";
    // }
}

// 然后处理所有的 直接调用，将其转换为 functionCaller
void IA::parseDirectCalls()
{
    for (auto &inst : directCalls)
    {
        // 将 callInst 转换为 CallInst
        CallInst *callInst = dyn_cast<CallInst>(inst);
        Function *callee = callInst->getCalledFunction();
        if (!callInst)
        {
            continue;
        }
        bool found = false;
        for (auto &caller : functionCallers)
        {
            if (caller.getFunction() == callee)
            {
                caller.addPossibleCaller(callInst);
                found = true;
                break;
            }
        }
        if (!found)
        {
            FunctionCaller caller(callee);
            caller.addPossibleCaller(callInst);
            functionCallers.push_back(caller);
        }
    }
    // 打印 functionCallers
    // for (auto &caller : functionCallers)
    // {
    //     errs() << "Function: " << caller.getFunction()->getName() << "\n";
    //     errs() << "Callers: \n";
    //     for (auto &inst : caller.getPossibleCallers())
    //     {
    //         errs() << *inst << "\n";
    //     }
    //     errs() << "--------------------------------------" << "\n";
    // }
}

// 当检查完所有的 impact 之后，我们需要对所有的 IA 类中的 IMPACT 对象进行分析，获取其原代码行号
void IA::parseSourceLine()
{
    int noLocInsts = 0;
    std::cout << "Parse Source Line informations" << std::endl;
    // 创建一个 SourceLineInfo 对象
    SourceLineInfo sli(originalFile, originalLine);

    // 遍历所有的 impact 对象
    std::cout << "Impacts size: " << impacts.size() << std::endl;
    for (auto &impact : impacts)
    {
        // 将每个 impact 的受影响的指令添加到 SourceLineInfo 对象中
        for (auto &inst : impact.getImpactedInsts())
        {
            // errs() << "Impacted Instruction: " << *inst << "\n";
            Instruction *impactedInst = dyn_cast<Instruction>(inst);
            // 确保 impactedInst 不为空
            if (!impactedInst)
            {
                // errs() << "Error: impactedInst is null\n";
                continue;
            }
            // 获取 impactedInst 的 debug location
            DILocation *loc = impactedInst->getDebugLoc();
            // 确保 loc 不为空
            if (!loc)
            {
                noLocInsts++;
                // errs() << "Error: loc is null\n";
                continue;
            }
            unsigned line = loc->getLine();
            // errs() << "Impacted Instruction Line: " << line << "\n";
            std::string fileName = loc->getFilename().str();
            // errs() << "Impacted Instruction File: " << fileName << "\n";
            sli.addImpactedInst(fileName, line);
        }
    }
    // 将新创建的 SourceLineInfo 对象添加到 IA 的 sourceLineInfos 中
    std::cout << "No Loc Insts numbers: " << noLocInsts << std::endl;
    addSourceLineInfo(sli);
}

// void IA::parseSourceLine()
// {
//     std::cout << "Parse Source Line informations" << std::endl;
//     // 遍历所有的 impact 对象
//     for (auto &impact : impacts)
//     {
//         Instruction *originalInst = impact.getOriginalInst();
//         if (!originalInst)
//             continue; // 确保 originalInst 不为空

//         Function *function = originalInst->getFunction();
//         Module *module = function->getParent();
//         DILocation *loc = originalInst->getDebugLoc();

//         if (!loc)
//             continue; // 确保 loc 不为空

//         unsigned line = loc->getLine();
//         std::string fileName = loc->getFilename().str();

//         bool found = false;
//         for (auto &sli : sourceLineInfos)
//         {
//             if (sli.getOriginalInstFile() == fileName && sli.getOriginalInstLine() == line)
//             {
//                 found = true;
//                 // 为已存在的 SourceLineInfo 添加 impact 的受影响的指令
//                 for (auto &use : impact.getImpactedInsts())
//                 {
//                     Instruction *useInst = dyn_cast<Instruction>(use);
//                     if (!useInst)
//                         continue; // 确保 useInst 不为空

//                     DILocation *useLoc = useInst->getDebugLoc();
//                     if (!useLoc)
//                         continue; // 确保 useLoc 不为空

//                     unsigned useLine = useLoc->getLine();
//                     std::string useFileName = useLoc->getFilename().str();

//                     sli.addImpactedInst(useFileName, useLine);
//                 }
//                 break; // 找到匹配后不需要继续检查其他 SourceLineInfo 对象
//             }
//         }

//         if (!found)
//         {
//             // 创建一个新的 SourceLineInfo 对象并添加当前 impact 的所有受影响的指令
//             SourceLineInfo sli(fileName, line);
//             for (auto &use : impact.getImpactedInsts())
//             {
//                 Instruction *useInst = dyn_cast<Instruction>(use);
//                 if (!useInst)
//                     continue; // 确保 useInst 不为空

//                 DILocation *useLoc = useInst->getDebugLoc();
//                 if (!useLoc)
//                     continue; // 确保 useLoc 不为空

//                 unsigned useLine = useLoc->getLine();
//                 std::string useFileName = useLoc->getFilename().str();

//                 sli.addImpactedInst(useFileName, useLine);
//             }
//             // 将新创建的 SourceLineInfo 对象添加到 IA 的 sourceLineInfos 中
//             addSourceLineInfo(sli);
//         }
//     }
// }

// 将所有的 sourceLineInfo 写入到文件中
void IA::writeSourceLineInfo()
{
    // 首先获取当前工作目录
    std::string currentDir = std::filesystem::current_path();
    // 在当前工作目录下的 result 文件夹下创建一个 sourceLineInfo.txt 文件
    // 首先检查 result 文件夹是否存在
    std::string resultDir = currentDir + "/result";
    if (!std::filesystem::exists(resultDir))
    {
        std::filesystem::create_directory(resultDir);
    }
    // 检查并创建 sourceLineInfo.txt 文件
    if (!std::filesystem::exists(resultDir + "/sourceLineInfo.txt"))
    {
        std::ofstream sourceLineInfoFile(resultDir + "/sourceLineInfo.txt");
    }
    // 如果当前 sourceLineInfo.txt 文件存在，则清空
    std::ofstream sourceLineInfoFile(resultDir + "/sourceLineInfo.txt");
    std::cout << "Write Source Line Info into: " << resultDir + "/sourceLineInfo.txt" << std::endl;
    // 将所有的 sourceLineInfo 写入到文件中
    std::cout << "Source Line Infos: " << sourceLineInfos.size() << std::endl;
    for (auto &sli : sourceLineInfos)
    {
        sli.writeToFile(sourceLineInfoFile);
    }
}

// 将 indirectCalls, directCalls, globalVariables， GlobalVariableInfo, FunctionInfos,functionCallers，ICallInfos, Impacts 写入到文件中
void IA::writeAllInfoToFile()
{
    // 写入目录为 result/tempInfo 文件夹，每个文件名为对应的名字
    // 首先获取当前工作目录
    std::string currentDir = std::filesystem::current_path();
    // 在当前工作目录下的 result/tempInfo 文件夹下创建一个 indirectCalls.txt 文件
    // 首先检查 result 文件夹是否存在
    std::string resultDir = currentDir + "/result";
    if (!std::filesystem::exists(resultDir))
    {
        std::filesystem::create_directory(resultDir);
    }
    // 检查并创建 tempInfo 文件夹
    std::string tempInfoDir = resultDir + "/tempInfo";
    if (!std::filesystem::exists(tempInfoDir))
    {
        std::filesystem::create_directory(tempInfoDir);
    }

    std::error_code EC;
    llvm::raw_fd_ostream indirectCallsFile(tempInfoDir + "/indirectCalls.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &inst : indirectCalls)
    {
        if (inst)
        {
            inst->print(indirectCallsFile);
            indirectCallsFile << "\n"; // For better readability
        }
    }
    llvm::raw_fd_ostream directCallsFile(tempInfoDir + "/directCalls.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &inst : directCalls)
    {
        if (inst)
        {
            inst->print(directCallsFile);
            directCallsFile << "\n"; // For better readability
        }
    }
    llvm::raw_fd_ostream globalVariablesFile(tempInfoDir + "/globalVariables.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &gv : globalVariables)
    {
        if (gv)
        {
            gv->print(globalVariablesFile);
            globalVariablesFile << "\n"; // For better readability
        }
    }
    llvm::raw_fd_ostream globalVariableInfosFile(tempInfoDir + "/globalVariableInfos.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &gvi : globalVariableInfos)
    {
        gvi.writeToFile(globalVariableInfosFile);
    }
    llvm::raw_fd_ostream functionInfosFile(tempInfoDir + "/functionInfos.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &fi : functionInfos)
    {
        fi.writeToFile(functionInfosFile);
    }
    llvm::raw_fd_ostream functionCallersFile(tempInfoDir + "/functionCallers.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &fc : functionCallers)
    {
        fc.writeToFile(functionCallersFile);
    }
    llvm::raw_fd_ostream iCallInfosFile(tempInfoDir + "/iCallInfos.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &ici : iCallInfos)
    {
        ici.writeToFile(iCallInfosFile);
    }
    llvm::raw_fd_ostream impactsFile(tempInfoDir + "/impacts.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &impact : impacts)
    {
        impact.writeToFile(impactsFile);
    }
    // 将 funcmodule 对应关系写入到文件中
    llvm::raw_fd_ostream funcModuleFile(tempInfoDir + "/funcModule.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &fm : functionInfos)
    {
        fm.writeFuncModuleToFile(funcModuleFile);
    }
}

void IA::write_impacts()
{
    // 写入目录为 result/tempInfo 文件夹，每个文件名为对应的名字
    // 首先获取当前工作目录
    std::string currentDir = std::filesystem::current_path();
    // 在当前工作目录下的 result/tempInfo 文件夹下创建一个 indirectCalls.txt 文件
    // 首先检查 result 文件夹是否存在
    std::string resultDir = currentDir + "/result";
    if (!std::filesystem::exists(resultDir))
    {
        std::filesystem::create_directory(resultDir);
    }
    // 检查并创建 tempInfo 文件夹
    std::string tempInfoDir = resultDir + "/tempInfo";
    if (!std::filesystem::exists(tempInfoDir))
    {
        std::filesystem::create_directory(tempInfoDir);
    }

    std::error_code EC;
    llvm::raw_fd_ostream impactsFile(tempInfoDir + "/impacts.txt", EC);
    if (EC)
    {
        llvm::errs() << "Error opening file for writing: " << EC.message() << "\n";
        return;
    }
    for (auto &impact : impacts)
    {
        impact.writeToFile(impactsFile);
    }
}

llvm::AAResults &IA::getAliasAnalysis()
{
    if (!aliasAnalysis)
    {
        // 获取模块的 DataLayout
        const llvm::DataLayout &DL = module->getDataLayout();

        // 创建 TargetLibraryInfo
        llvm::Triple ModuleTriple(module->getTargetTriple());
        auto TLII = std::make_unique<llvm::TargetLibraryInfoImpl>(ModuleTriple);
        auto TLI = std::make_unique<llvm::TargetLibraryInfo>(*TLII);

        // 初始化 AAResults
        aliasAnalysis = std::make_unique<llvm::AAResults>(*TLI);

        // 遍历模块中的每个函数
        for (auto &F : *module)
        {
            // 创建辅助分析对象
            auto AC = std::make_unique<llvm::AssumptionCache>(F);
            auto DT = std::make_unique<llvm::DominatorTree>(F);
            auto LI = std::make_unique<llvm::LoopInfo>(*DT);

            // 使用指针传递 DominatorTree 和 LoopInfo
            llvm::BasicAAResult BAA(DL, F, *TLI, *AC, DT.get(), LI.get());

            // 将分析结果添加到 AAResults
            aliasAnalysis->addAAResult(BAA);
        }
    }

    return *aliasAnalysis;
}