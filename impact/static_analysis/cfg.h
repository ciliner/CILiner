#ifndef CFG_H
#define CFG_H

#include "module_parse.h"
#include <unordered_map>

using namespace llvm;

std::unordered_map<CallInst *, std::vector<Function *>> getAllPossibleCallTargets(IA &ia);
std::vector<Function *> indirectCallAnalyze(CallInst *callStatement);
void getAllFuncInfo(IA &ia);
IndirectCallInfo getCallStatementInfo(llvm::CallInst *callStatement, IA &ia);
void analyzeICall(IndirectCallInfo &iCallInfo, IA &ia);
void analyzeAllICalls(IA &ia);
std::vector<Instruction *> getFunctionCallSites(Function *func, IA &ia);

#endif