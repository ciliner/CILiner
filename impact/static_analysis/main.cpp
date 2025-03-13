#include "cfg.h"
#include "dfg.h"
#include "sdg.h"

int main(int argc, char **argv)
{
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

    llvm::LLVMContext context;

    IA ia;
    bool writeIFiles = ia.argsHandle(argc, argv);
    ia.parseFiles(context);
    ia.compareChanges();
    auto start = std::chrono::high_resolution_clock::now();
    ia.getAllGVs();
    ia.analyzeAllCallInsts();
    getAllFuncInfo(ia);
    analyzeAllICalls(ia);
    ia.parseICallInfos();
    ia.parseDirectCalls();
    checkChanges(ia);
    ia.parseSourceLine();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    // ia.writeSourceLineInfo();
    if (writeIFiles)
    {
        ia.writeAllInfoToFile();
    }
    else
    {
        ia.write_impacts();
    }
    for (auto impact : ia.getImpacts())
    {
        impact.writeFinalResult();
    }

    std::cout << "Elapsed time: " << elapsed.count() << " s\n";
    return 0;
}
