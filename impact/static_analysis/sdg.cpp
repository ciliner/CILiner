#include "sdg.h"
#include "cfg.h"

bool checkGlobalVariableChanges(Instruction *inst)
{

    if (auto storeInst = dyn_cast<StoreInst>(inst))
    {
        Value *storeOperand = storeInst->getValueOperand();
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(storeOperand))
        {
            //     errs() << "Store instruction: " << *storeInst << "\n";
            //     errs() << "In function: " << storeInst->getFunction()->getName() << "\n";
            //     errs() << "Changes global variable: " << GV->getName() << "\n\n";
            return true;
        }
    }
    return false;
}

void checkChanges(IA &ia)
{
    std::cout << "Checking changes\n";
    std::vector<Instruction *> changedInstructions = ia.getChangedInstructions();
    for (Instruction *inst : changedInstructions)
    {
        IMPACT impact = IMPACT(inst);
        // errs() << "Analyzing impact for inst: " << *inst << "\n";
        constructDFG(inst, impact);
        impactAnalyzeGlobal(impact, ia);
        ia.addImpact(impact);
        // impact.print();
    }
}

void impactAnalyzeGlobal(IMPACT &impact, IA &ia)
{
    std::set<Instruction *> impactedInstructions = impact.getImpactedInsts();
    std::vector<BasicBlock *> visitedBlocks;
    std::vector<Instruction *> visitedSwitchInsts;
    std::vector<Instruction *> visitedInstructions; // 用于记录已经访问过的指令
    std::vector<Function *> visitedFunctions;

    int i = 0;
    while (i < impactedInstructions.size())
    {
        Instruction *inst = *std::next(impactedInstructions.begin(), i);

        if (std::find(visitedInstructions.begin(), visitedInstructions.end(), inst) != visitedInstructions.end())
        {
            // errs() << "Instruction already visited\n";
            i++;
            continue;
        }

        Function *func = inst->getFunction();

        std::vector<Instruction *> callTargets = getFunctionCallSites(func, ia);
        for (auto callTarget : callTargets)
        {
            impact.addImpactedInst(callTarget);
        }

        visitedFunctions.push_back(func);

        constructDFG(inst, impact);

        // if (auto returnInst = dyn_cast<ReturnInst>(inst))
        // {
        //     analyzeReturnInst(inst, ia, impact);
        // }
        if (checkGlobalVariableChanges(inst))
        {
            analyzeGlobalInst(inst, ia, impact);
        }
        else if (auto brInst = dyn_cast<BranchInst>(inst))
        {
            analyzeBrInst(inst, ia, impact);
        }
        else if (auto switchInst = dyn_cast<SwitchInst>(inst))
        {
            BasicBlock *parentBlock = inst->getParent();
            visitedBlocks.push_back(parentBlock);
            analyzeSwitchInst(inst, ia, impact, visitedBlocks, visitedSwitchInsts);
        }

        visitedInstructions.push_back(inst);

        impactedInstructions = impact.getImpactedInsts();
        i++;
    }
}

void constructDFG(Instruction *inst, IMPACT &impact)
{
    std::vector<DFGNode::Ptr> dfgNodes = analyzeDataFlow(inst);
    // for (DFGNode::Ptr dfgNode : dfgNodes)
    // {
    //     errs() << "DFGNode: " << *dfgNode->getInstruction() << "\n";
    //     errs() << "In function: " << dfgNode->getInstruction()->getFunction()->getName() << "\n";
    //     for (DFGNode::Ptr use : dfgNode->getUses())
    //     {
    //         errs() << "Uses: " << *use->getInstruction() << "\n";
    //     }
    // }
    std::vector<DFGNode::Ptr> visited;
    for (DFGNode::Ptr dfgNode : dfgNodes)
    {
        findAllUses(impact, dfgNode, visited);
    }
}

void analyzeGlobalInst(Instruction *inst, IA &ia, IMPACT &impact)
{
    if (auto storeInst = dyn_cast<StoreInst>(inst))
    {
        Value *storeOperand = storeInst->getValueOperand();
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(storeOperand))
        {
            // errs() << "Store instruction: " << *storeInst << "\n";
            // errs() << "In function: " << storeInst->getFunction()->getName() << "\n";
            // errs() << "Changes global variable: " << GV->getName() << "\n\n";
            // std::vector<Instruction *> uses = findAllUses(GV);
            // impact.addImpactedInstsWithVector(uses);
            for (User *U : GV->users())
            {
                if (Instruction *use = dyn_cast<Instruction>(U))
                {
                    impact.addImpactedInst(use);
                }
            }
        }
    }
}

void analyzeBrInst(Instruction *inst, IA &ia, IMPACT &impact)
{
    // errs() << "Analyzing br instruction: " << *inst << "\n";
    // 如果该指令是 br 指令
    if (auto brInst = dyn_cast<BranchInst>(inst))
    {
        for (unsigned i = 0; i < brInst->getNumSuccessors(); i++)
        {
            // errs() << "Successor: " << *brInst->getSuccessor(i) << "\n";
            BasicBlock *successor = brInst->getSuccessor(i);
            for (Instruction &inst : *successor)
            {
                // errs() << "Successor instruction: " << inst << "\n";
                // 将该基本快的所有指令添加到 impact 的 uses 中
                impact.addImpactedInst(&inst);
            }
        }
    }
}

void analyzeSwitchInst(Instruction *inst, IA &ia, IMPACT &impact, std::vector<BasicBlock *> &visitedBlocks, std::vector<Instruction *> &visitedSwitchInsts)
{
    if (auto switchInst = dyn_cast<SwitchInst>(inst))
    {
        visitedSwitchInsts.push_back(inst);
        for (unsigned i = 0; i < switchInst->getNumSuccessors(); i++)
        {
            BasicBlock *successor = switchInst->getSuccessor(i);
            if (std::find(visitedBlocks.begin(), visitedBlocks.end(), successor) != visitedBlocks.end())
            {
                continue;
            }
            for (Instruction &inst : *successor)
            {
                impact.addImpactedInst(&inst);
            }
        }
    }
}