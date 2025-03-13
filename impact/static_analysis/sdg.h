#ifndef SDG_H
#define SDG_H

#include "module_parse.h"
#include "dfg.h"

bool checkGlobalVariableChanges(Instruction *inst);
void checkChanges(IA &ia);
void impactAnalyzeGlobal(IMPACT &impact, IA &ia);
void constructDFG(Instruction *inst, IMPACT &impact);
void analyzeGlobalInst(Instruction *inst, IA &ia, IMPACT &impact);
void analyzeReturnInst(Instruction *inst, IA &ia, IMPACT &impact);
void analyzeBrInst(Instruction *inst, IA &ia, IMPACT &impact);
void analyzeSwitchInst(Instruction *inst, IA &ia, IMPACT &impact, std::vector<BasicBlock *> &visitedBlocks, std::vector<Instruction *> &visitedSwitchInsts);

#endif
