#include "dfg.h"

std::vector<DFGNode::Ptr> analyzeDataFlow(llvm::Instruction *startInst)
{
    std::vector<DFGNode::Ptr> dfgNodes;
    if (!startInst || !startInst->getFunction())
    {
        return dfgNodes;
    }

    llvm::Function *func = startInst->getFunction();
    std::unordered_map<llvm::Instruction *, DFGNode::Ptr> instToNode;

    for (auto &BB : *func)
    {
        for (auto &I : BB)
        {
            auto node = std::make_shared<DFGNode>(&I);
            dfgNodes.push_back(node);
            instToNode[&I] = node;
        }
    }

    for (auto &BB : *func)
    {
        for (auto &I : BB)
        {
            auto currentNode = instToNode[&I];

            for (auto &use : I.uses())
            {
                if (auto *userInst = dyn_cast<Instruction>(use.getUser()))
                {
                    if (userInst->getFunction() == func)
                    {
                        if (auto useNode = instToNode[userInst])
                        {
                            currentNode->addUse(useNode);
                        }
                    }
                }
            }
        }
    }

    // for (auto &node : dfgNodes)
    // {
    //     errs() << "\nInstruction: " << *node->getInstruction() << "\n";

    //     errs() << "  Used by:\n";
    //     for (auto &use : node->getUses())
    //     {
    //         errs() << "    -> " << *use->getInstruction() << "\n";
    //     }

    //     errs() << "  Uses definitions from:\n";
    //     for (auto &def : node->getDefs())
    //     {
    //         errs() << "    <- " << *def->getInstruction() << "\n";
    //     }
    // }

    return dfgNodes;
}

void findAllUses(IMPACT &impact, DFGNode::Ptr node, std::vector<DFGNode::Ptr> &visitedNodes)
{
    if (!node || std::find(visitedNodes.begin(), visitedNodes.end(), node) != visitedNodes.end())
    {
        return;
    }

    visitedNodes.push_back(node);

    for (auto &useNode : node->getUses())
    {
        if (useNode)
        {
            impact.addImpactedInst(useNode->getInstruction());
            findAllUses(impact, useNode, visitedNodes);
        }
    }
}