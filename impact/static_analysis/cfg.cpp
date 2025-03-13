#include "cfg.h"

std::unordered_map<CallInst *, std::vector<Function *>> getAllPossibleCallTargets(IA &ia)
{
    std::vector<Instruction *> insts = ia.getIndirectCalls();
    std::vector<CallInst *> indirectCallInsts;
    for (auto inst : insts)
    {
        CallInst *callInst = dyn_cast<CallInst>(inst);
        if (callInst)
        {
            indirectCallInsts.push_back(callInst);
        }
    }
    std::unordered_map<CallInst *, std::vector<Function *>> possibleCallTargets;
    for (auto indirectCall : indirectCallInsts)
    {
        errs() << "indirectCall: " << *indirectCall << "\n";
        std::vector<Function *> possibleTargets;
        possibleTargets = indirectCallAnalyze(indirectCall);
        possibleCallTargets[indirectCall] = possibleTargets;
    }
    return possibleCallTargets;
}

std::vector<Function *> indirectCallAnalyze(CallInst *callStatement)
{
    std::vector<Function *> possibleTargets;

    errs() << "indirectCallAnalyze for " << *callStatement << "\n";
    std::string return_type = "";
    std::string variables = "";
    std::string signature = "";

    Value *calledOperand = callStatement->getCalledOperand();
    SmallPtrSet<Value *, 8> Visited;

    while (calledOperand && !isa<Function>(calledOperand) && Visited.insert(calledOperand).second)
    {
        errs() << "Called operand: " << *calledOperand << "\n";
    }
    return possibleTargets;
}

void getAllFuncInfo(IA &ia)
{
    std::cout << "Getting all function information\n";
    std::vector<Function *> functions = ia.getCommitBCFunctions();
    for (auto func : functions)
    {
        std::string name = func->getName().str();
        Type *funcReturnType = func->getReturnType();
        int numParams = func->arg_size();
        std::vector<Type *> paramTypes = getFuncParameterTypes(func);
        FunctionInformation funcInfo(func, paramTypes, funcReturnType);
        ia.addFunctionInfo(funcInfo);
    }
}

IndirectCallInfo getCallStatementInfo(llvm::CallInst *callStatement, IA &ia)
{
    if (!callStatement)
    {
        errs() << "Call statement is null\n";
        return IndirectCallInfo();
    }

    Type *return_type = callStatement->getType();
    int num_args = callStatement->arg_size();
    std::vector<Type *> arg_types;
    for (int i = 0; i < num_args; ++i)
    {
        arg_types.push_back(callStatement->getArgOperand(i)->getType());
    }

    // errs() << "Call statement: " << *callStatement << "\n";
    // errs() << "Return type: " << *return_type << "\n";
    // errs() << "Number of arguments: " << num_args << "\n";
    // for (Type *arg_type : arg_types)
    // {
    //     errs() << "Argument type: " << *arg_type << "\n";
    // }

    return IndirectCallInfo(callStatement, return_type, arg_types);
}

void analyzeICall(IndirectCallInfo &iCallInfo, IA &ia)
{
    std::vector<FunctionInformation> functionInfos = ia.getFunctionInfos();
    for (FunctionInformation funcInfo : functionInfos)
    {

        if (funcInfo.getReturnType() == iCallInfo.getReturnType() && funcInfo.getNumArgs() == iCallInfo.getNumArgs() && funcInfo.getArgTypes() == iCallInfo.getArgTypes())
        {
            // errs() << "Found matching function: " << funcInfo.getFunction()->getName().str() << " in file " << funcInfo.getFunction()->getParent()->getSourceFileName() << "\n";
            iCallInfo.addPossibleCallee(funcInfo.getFunction());
        }
    }
}

void analyzeAllICalls(IA &ia)
{
    std::cout << "Analyzing all indirect calls possible callees\n";
    for (auto icall : ia.getIndirectCalls())
    {
        CallInst *callInst = dyn_cast<CallInst>(icall);
        IndirectCallInfo ici = getCallStatementInfo(callInst, ia);
        analyzeICall(ici, ia);
        // ici.print();
        ia.addICallInfo(ici);
    }
}

std::vector<Instruction *> getFunctionCallSites(Function *func, IA &ia)
{
    // std::cout << "Getting all call sites for function: " << func->getName().str() << "\n";
    const std::vector<FunctionCaller> &functionCallers = ia.getFunctionCallers();
    // for (FunctionCaller fc : functionCallers)
    // {
    //     fc.print();
    // }

    for (FunctionCaller fc : functionCallers)
    {
        if (fc.getFunction()->getName() == func->getName())
        {
            // errs() << "Function fc: " << fc.getFunction()->getName() << "\n";
            // errs() << "Function func : " << func->getName() << "\n";
            std::vector<Instruction *> callSites = fc.getPossibleCallers();
            // for (Instruction *inst : callSites)
            // {
            //     errs() << "Call site: " << *inst << "\n";
            // }
            return callSites;
        }
    }
    // std::cout << "No call sites found for function: " << func->getName().str() << "\n";
    return std::vector<Instruction *>();
}

void pointerAnalysis(IA &ia)
{
    llvm::AAResults &aliasAnalysis = ia.getAliasAnalysis();

    for (auto &ici : ia.getICallInfos())
    {
        CallInst *callInst = dyn_cast<CallInst>(ici.getCallInst());
        if (!callInst)
        {
            errs() << "Skipping null CallInst in pointerAnalysis\n";
            continue;
        }

        const std::vector<Function *> &typeAnalysisCallees = ici.getPossibleCallees();
        std::vector<Function *> filteredCallees;

        Value *calledOperand = callInst->getCalledOperand();
        if (!calledOperand)
        {
            errs() << "Skipping null CalledOperand for CallInst: " << *callInst << "\n";
            continue;
        }

        for (Function *callee : typeAnalysisCallees)
        {
            if (aliasAnalysis.alias(calledOperand, callee) != llvm::NoAlias)
            {
                filteredCallees.push_back(callee); // 仅保留可能的目标
            }
        }

        if (filteredCallees.empty())
        {
            filteredCallees = typeAnalysisCallees;
        }

        ici.setPossibleCallees(filteredCallees); // 使用 setter 方法更新

        // errs() << "Call Instruction: " << *callInst << "\n";
        // errs() << "Filtered Possible Callees:\n";
        for (Function *func : filteredCallees)
        {
            errs() << "  " << func->getName() << "\n";
        }
    }
}