#ifndef DFG_H
#define DFG_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"

#include <vector>
#include <string>
#include <set>
#include <unordered_set>
#include <fstream>

#include "module_parse.h"

using namespace llvm;

class DFGNode
{
public:
    using Ptr = std::shared_ptr<DFGNode>;
    using WeakPtr = std::weak_ptr<DFGNode>;
    using InstructionPtr = llvm::Instruction *;

private:
    InstructionPtr instruction;
    std::vector<Ptr> uses;
    std::vector<WeakPtr> defs;

public:
    DFGNode(InstructionPtr inst) : instruction(inst) {}

    void addUse(Ptr use)
    {
        uses.push_back(std::move(use));
    }

    void addDef(Ptr def)
    {
        defs.push_back(def);
    }

    InstructionPtr getInstruction() const
    {
        return instruction;
    }

    const std::vector<Ptr> &getUses() const
    {
        return uses;
    }

    std::vector<Ptr> getDefs() const
    {
        std::vector<Ptr> result;
        for (const auto &weakDef : defs)
        {
            if (auto def = weakDef.lock())
            {
                result.push_back(def);
            }
        }
        return result;
    }
};

std::vector<std::string> analyzeChangedInstructions(std::vector<llvm::Instruction *> changedInsts);
std::vector<DFGNode::Ptr> analyzeDataFlow(llvm::Instruction *startInst);
void identifyDependents(DFGNode::Ptr dfgNode, std::vector<DFGNode::Ptr> dfgNodes);
std::vector<Instruction *> getGlobalVariableUses(llvm::GlobalVariable *GV);
void findAllUses(IMPACT &impact, DFGNode::Ptr node, std::vector<DFGNode::Ptr> &visitedNodes);
#endif // DFG_H