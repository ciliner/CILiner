#ifndef ModuleParse_H
#define ModuleParse_H

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/Signals.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/LegacyPassManager.h"

// pointer analysis
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <map>
#include <filesystem>
#include <regex>
#include <set>
#include <unordered_set>
#include <unordered_map>

using namespace llvm;

void signalHandler(int signum);

class SourceLineInfo
{
public:
    SourceLineInfo(std::string originalInstFile, int originalInstLine)
    {
        this->originalInstFile = originalInstFile;
        this->originalInstLine = originalInstLine;
        this->impactedInsts = std::map<std::string, std::vector<int>>();
    }

    void addImpactedInst(std::string impactedInstFile, int impactedInstLine)
    {
        if (this->impactedInsts.find(impactedInstFile) != this->impactedInsts.end())
        {
            // std::cout << "File: " << impactedInstFile << " already exists\n";
            // std::cout << "Adding line number: " << impactedInstLine << "\n";
            this->impactedInsts[impactedInstFile].push_back(impactedInstLine);
        }
        else
        {
            // std::cout << "File: " << impactedInstFile << " does not exist\n";
            // std::cout << "Creating new file and adding line number: " << impactedInstLine << "\n";
            this->impactedInsts[impactedInstFile] = std::vector<int>{impactedInstLine};
        }
    }
    // getter
    std::string getOriginalInstFile()
    {
        return this->originalInstFile;
    }
    int getOriginalInstLine()
    {
        return this->originalInstLine;
    }
    std::map<std::string, std::vector<int>> getImpactedInsts()
    {
        return this->impactedInsts;
    }

    // print
    void print() const
    {
        errs() << "Original Instruction: " << this->originalInstFile << ":" << this->originalInstLine << "\n";
        errs() << "Impacted Instructions: \n";
        for (auto &impactedInst : this->impactedInsts)
        {
            std::string fileName = impactedInst.first;
            std::vector<int> lineNumbers = impactedInst.second;
            errs() << fileName << ": ";
            for (int i = 0; i < lineNumbers.size(); i++)
            {
                errs() << lineNumbers[i];
                if (i != lineNumbers.size() - 1)
                {
                    errs() << ", ";
                }
            }
        }
    }

    void writeToFile(std::ofstream &file)
    {
        file << "Original Instruction: " << this->originalInstFile << ":" << this->originalInstLine << "\n";
        file << "Impacted Instructions: \n";
        for (auto &impactedInst : this->impactedInsts)
        {
            std::string fileName = impactedInst.first;
            std::vector<int> lineNumbers = impactedInst.second;
            file << fileName << ": ";
            for (int i = 0; i < lineNumbers.size(); i++)
            {
                file << lineNumbers[i];
                if (i != lineNumbers.size() - 1)
                {
                    file << ", ";
                }
            }
            file << "\n";
        }
    }

private:
    std::string originalInstFile;
    int originalInstLine;
    std::map<std::string, std::vector<int>> impactedInsts;
};

class FunctionCaller
{
public:
    FunctionCaller(Function *function)
    {
        this->function = function;
        this->possibleCallers = std::vector<Instruction *>();
    }

    void addPossibleCaller(Instruction *inst)
    {
        this->possibleCallers.push_back(inst);
    }

    Function *getFunction()
    {
        return this->function;
    }
    std::vector<Instruction *> getPossibleCallers()
    {
        return this->possibleCallers;
    }

    void print()
    {
        errs() << "Function: " << this->function->getName().str() << "\n";
        errs() << "Possible Callers: ";
        for (auto *inst : this->possibleCallers)
        {
            errs() << *inst << " ";
        }
        errs() << "\n";
    }

    void writeToFile(llvm::raw_fd_ostream &file)
    {
        file << "Function: " << this->function->getName().str() << "\n";
        file << "Possible Callers: ";
        for (auto *inst : this->possibleCallers)
        {
            file << *inst << " ";
        }
        file << "\n";
    }

private:
    Function *function;
    std::vector<Instruction *> possibleCallers;
};

class IndirectCallInfo
{
public:
    Instruction *callInst;
    Type *return_type;
    std::vector<Type *> argTypes;
    mutable std::vector<Function *> possibleCallees;

    IndirectCallInfo() = default;

    IndirectCallInfo(Instruction *callInst, Type *return_type, std::vector<Type *> argTypes)
    {
        this->callInst = callInst;
        this->return_type = return_type;
        this->argTypes = argTypes;
    }

    Instruction *getCallInst() const
    {
        return this->callInst;
    }

    Type *getReturnType() const
    {
        return this->return_type;
    }

    Type *getArgType(int index) const
    {
        return this->argTypes[index];
    }

    const std::vector<Type *> &getArgTypes() const
    {
        return this->argTypes;
    }

    int getNumArgs() const
    {
        return this->argTypes.size();
    }

    const std::vector<Function *> &getPossibleCallees() const
    {
        return possibleCallees;
    }

    void setCallInst(Instruction *inst)
    {
        this->callInst = inst;
    }

    void setReturnType(Type *type)
    {
        this->return_type = type;
    }

    void setArgTypes(std::vector<Type *> types)
    {
        this->argTypes = types;
    }

    void setPossibleCallees(const std::vector<Function *> &callees) const
    {
        possibleCallees.clear();
        possibleCallees.insert(possibleCallees.end(), callees.begin(), callees.end());
    }

    void addPossibleCallee(Function *callee) const
    {
        this->possibleCallees.push_back(callee);
    }

    void print() const
    {
        errs() << "Call Instruction: " << *this->callInst << "\n";
        errs() << "Return Type: ";
        this->return_type->print(errs());
        errs() << "\nArgument Types: ";
        for (auto *argType : this->argTypes)
        {
            argType->print(errs());
            errs() << " ";
        }
        errs() << "\nPossible Callees: ";
        for (auto *callee : this->possibleCallees)
        {
            errs() << callee->getName().str() << " ";
        }
        errs() << "\n";
    }

    void writeToFile(llvm::raw_fd_ostream &file) const
    {
        file << "Call Instruction: " << *this->callInst << "\n";
        file << "Return Type: ";
        this->return_type->print(file);
        file << "\nArgument Types: ";
        for (auto *argType : this->argTypes)
        {
            argType->print(file);
            file << " ";
        }
        file << "\nPossible Callees: ";
        for (auto *callee : this->possibleCallees)
        {
            file << callee->getName().str() << " ";
        }
        file << "\n";
    }
};

class FunctionInformation
{
public:
    Function *function;
    std::vector<Type *> argTypes;
    Type *returnType;

    FunctionInformation() = default;
    FunctionInformation(Function *function, std::vector<Type *> argTypes, Type *returnType)
    {
        this->function = function;
        this->argTypes = argTypes;
        this->returnType = returnType;
    }

    Function *getFunction()
    {
        return this->function;
    }
    Type *getReturnType()
    {
        return this->returnType;
    }
    Type *getArgType(int index)
    {
        return this->argTypes[index];
    }
    std::vector<Type *> getArgTypes()
    {
        return this->argTypes;
    }
    int getNumArgs()
    {
        return this->argTypes.size();
    }

    void print()
    {
        errs() << "Function Name: " << this->function->getName().str() << "\n";
        errs() << "Return Type: ";
        this->returnType->print(errs());
        errs() << "\nArgument Types: ";
        for (auto *argType : this->argTypes)
        {
            argType->print(errs());
            errs() << " ";
        }
        errs() << "\n";
    }

    void writeToFile(llvm::raw_fd_ostream &file)
    {
        file << "Function Name: " << this->function->getName().str() << "\n";
        file << "Return Type: ";
        this->returnType->print(file);
        file << "\nArgument Types: ";
        for (auto *argType : this->argTypes)
        {
            argType->print(file);
            file << " ";
        }
        file << "\n";
    }

    void writeFuncModuleToFile(llvm::raw_fd_ostream &file)
    {
        file << this->function->getName().str() << ":" << this->function->getParent()->getSourceFileName() << "\n";
    }
};

class GlobalVariableInfo
{
public:
    GlobalVariable *globalVariable;
    std::vector<Instruction *> useInstructions;
    GlobalVariableInfo() = default;

    GlobalVariableInfo(GlobalVariable *globalVariable, std::vector<Instruction *> instructions)
    {
        this->globalVariable = globalVariable;
        this->useInstructions = instructions;
    }

    GlobalVariable *getGlobalVariable()
    {
        return this->globalVariable;
    }
    std::vector<Instruction *> getUseInstructions()
    {
        return this->useInstructions;
    }

    void setGlobalVariable(GlobalVariable *globalVariable)
    {
        this->globalVariable = globalVariable;
    }
    void setUseInstructions(std::vector<Instruction *> instructions)
    {
        this->useInstructions = instructions;
    }

    void addUseInstruction(Instruction *inst)
    {
        this->useInstructions.push_back(inst);
    }

    void writeToFile(llvm::raw_fd_ostream &file)
    {
        file << "Global Variable: " << this->globalVariable->getName().str() << "\n";
        file << "Use Instructions: ";
        for (auto *inst : this->useInstructions)
        {
            file << *inst << " ";
        }
        file << "\n";
    }
};

class IMPACT
{
public:
    IMPACT(Instruction *inst)
    {
        this->originalInst = inst;
        this->impactedInsts = std::set<Instruction *>();
    }
    void setOriginalInst(Instruction *inst)
    {
        this->originalInst = inst;
    }
    void addImpactedInstsWithVector(std::vector<Instruction *> impactedInsts)
    {
        for (auto *inst : impactedInsts)
        {
            for (auto *impactedInst : this->impactedInsts)
            {
                if (impactedInst == inst)
                {
                    return;
                }
            }
            this->impactedInsts.insert(inst);
        }
    }
    void addImpactedInst(Instruction *inst)
    {
        for (auto *impactedInst : this->impactedInsts)
        {
            if (impactedInst == inst)
            {
                return;
            }
        }
        this->impactedInsts.insert(inst);
    }

    Instruction *getOriginalInst()
    {
        return this->originalInst;
    }
    std::set<Instruction *> getImpactedInsts()
    {
        return this->impactedInsts;
    }

    void print()
    {
        Module *module = this->originalInst->getModule();
        errs() << "Original Instruction: " << *this->originalInst << " in File " << module->getSourceFileName() << " in Func:" << this->originalInst->getFunction()->getName() << "\n";
        errs() << "Impacted Instructions: \n";
        for (auto *inst : this->impactedInsts)
        {
            Module *impactedModule = inst->getModule();
            errs() << *inst << " : ";
            errs() << "In File " << impactedModule->getSourceFileName() << " in Func:" << inst->getFunction()->getName() << "\n";
        }
    }

    void writeToFile(llvm::raw_fd_ostream &file)
    {
        Module *module = this->originalInst->getModule();
        file << "Original Instruction: " << *this->originalInst << " in File " << module->getSourceFileName() << " in Func:" << this->originalInst->getFunction()->getName() << "\n";
        file << "Impacted Instructions: \n";
        for (auto *inst : this->impactedInsts)
        {
            Module *impactedModule = inst->getModule();
            file << *inst << " : ";
            file << "In File " << impactedModule->getSourceFileName() << " in Func:" << inst->getFunction()->getName() << "\n";
        }
    }

    void writeFinalResult()
    {
        int noLocCount = 0;
        std::string workingDir = std::filesystem::current_path();
        std::string filePath = workingDir + "/result/impact_result.txt";
        std::remove(filePath.c_str());
        std::error_code EC;
        llvm::raw_fd_ostream file(filePath, EC);
        if (EC)
        {
            llvm::errs() << "Error opening file: " << EC.message() << "\n";
            return;
        }
        for (auto *inst : this->impactedInsts)
        {
            DILocation *loc = inst->getDebugLoc();
            if (!loc)
            {
                // errs() << "Error: loc is null\n";
                noLocCount++;
                continue;
            }
            std::string funcName = inst->getFunction()->getName().str();
            unsigned line = loc->getLine();
            std::string fileName = loc->getFilename().str();
            file << fileName << ":" << funcName << ":" << line << "\n";
        }
        std::cout << "No location count: " << noLocCount << "\n";
    }

private:
    Instruction *originalInst;
    std::set<Instruction *> impactedInsts;
};

class IA
{
public:
    IA() = default;

    bool argsHandle(int argc, char **argv);
    void parseFileInstNo(std::string fileInstNo);
    void parseFiles(llvm::LLVMContext &context);
    void compareChanges();
    llvm::Function *getChangedFunction(llvm::Module &module, std::string fileName, std::string funcName);
    std::string removeStructVersionNumber(const std::string &str);
    void analyzeAllCallInsts();
    void callAnalyze(CallInst *callStatement);
    void getAllGVs();
    void parseICallInfos();
    void parseDirectCalls();
    void parseSourceLine();
    void writeSourceLineInfo();
    void writeAllInfoToFile();
    void write_impacts();
    llvm::AAResults &getAliasAnalysis();

    std::vector<Function *> getCommitBCFunctions() { return commitBCFunctions; }
    std::vector<Instruction *> getChangedInstructions() { return changedInstructions; }
    std::vector<Instruction *> getDirectCalls() { return directCalls; }
    std::vector<Instruction *> getIndirectCalls() { return indirectCalls; }
    std::vector<GlobalVariable *> getGlobalVariables() { return globalVariables; }

    const std::vector<IndirectCallInfo> &getICallInfos() const
    {
        return iCallInfos;
    }
    const std::vector<FunctionInformation> &getFunctionInfos() const
    {
        return functionInfos;
    }
    const std::vector<GlobalVariableInfo> &getGlobalVariableInfos() const
    {
        return globalVariableInfos;
    }
    const std::vector<IMPACT> &getImpacts() const
    {
        return impacts;
    }
    const std::vector<FunctionCaller> &getFunctionCallers() const
    {
        return functionCallers;
    }
    const std::vector<SourceLineInfo> &getSourceLineInfos() const
    {
        return sourceLineInfos;
    }

    void setOriginalFile(std::string originalFile)
    {
        this->originalFile = originalFile;
    }
    void setOriginalLine(int originalLine)
    {
        this->originalLine = originalLine;
    }

    void
    addFunctionInfo(FunctionInformation functionInfo)
    {
        functionInfos.push_back(functionInfo);
    }

    void addICallInfo(IndirectCallInfo iCallInfo)
    {
        iCallInfos.push_back(iCallInfo);
    }
    void addFunctionCaller(FunctionCaller functionCaller)
    {
        functionCallers.push_back(functionCaller);
    }

    void addGlobalVariableInfo(GlobalVariableInfo globalVariableInfo)
    {
        globalVariableInfos.push_back(globalVariableInfo);
    }
    void addImpact(IMPACT impact)
    {
        impacts.push_back(impact);
    }
    void addSourceLineInfo(SourceLineInfo sourceLineInfo)
    {
        sourceLineInfos.push_back(sourceLineInfo);
    }

private:
    std::map<std::string, int> diffResult;
    std::string workingDir = std::filesystem::current_path();
    std::string commitBCDir = workingDir + "/test/bc";
    std::vector<std::unique_ptr<llvm::Module>> commitBCModules;
    std::vector<Function *> commitBCFunctions;
    std::string originalFile;
    int originalLine;
    std::vector<Instruction *> changedInstructions;
    std::vector<Instruction *> directCalls;
    std::vector<Instruction *> indirectCalls;
    std::vector<GlobalVariable *> globalVariables;
    std::vector<IndirectCallInfo> iCallInfos;
    std::vector<FunctionInformation> functionInfos;
    std::vector<GlobalVariableInfo> globalVariableInfos;
    std::vector<IMPACT> impacts;
    std::vector<FunctionCaller> functionCallers;
    std::vector<SourceLineInfo> sourceLineInfos;
    std::unique_ptr<llvm::AAResults> aliasAnalysis;
    std::unique_ptr<llvm::Module> module;
};

std::vector<Type *> getFuncParameterTypes(Function *function);

#endif