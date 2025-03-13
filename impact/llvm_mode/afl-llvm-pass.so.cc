/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "fstream"
#include "llvm/IR/DebugInfo.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/IR/Module.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace
{

  class AFLCoverage : public ModulePass
  {

  public:
    static char ID;
    AFLCoverage() : ModulePass(ID) {}

    bool runOnModule(Module &M) override;

    // StringRef getPassName() const override {
    //  return "American Fuzzy Lop Instrumentation";
    // }
  };

}

char AFLCoverage::ID = 0;

bool AFLCoverage::runOnModule(Module &M)
{

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET"))
  {

    SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");
  }
  else
    be_quiet = 1;

  /* Decide instrumentation ratio */

  char *inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str)
  {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");
  }

  // Remove or comment out the original instrumentation logic here
  llvm::StringRef moduleName = M.getName();
  // remove the .c from the module name
  moduleName = moduleName.drop_back(2);
  std::string fileName = moduleName.str();
  // 只要文件名
  size_t found = fileName.find_last_of("/");

  fileName = fileName.substr(found + 1);

  errs() << "fileName: " << fileName << "\n";

  std::string outputDir = "/root/CILiner/test/bcoutput/";

  std::string outputPath = outputDir + fileName + ".bc";

  // 使用 llvmsys::fs::exists 输出文件目录
  // Add bitcode generation logic at the end of the function
  std::error_code EC;
  raw_fd_ostream OS(outputPath, EC);
  if (!EC)
  {
    WriteBitcodeToFile(M, OS);
    OS.flush();
  }
  else
  {
    errs() << "Error: " << outputPath << "\n";
    errs() << "Error: " << EC.message() << "\n";
  }

  // 生成 .ll 文件
  std::string llOutDir = "/root/CILiner/test/lloutput/";
  std::string outputLLPath = llOutDir + fileName + ".ll";

  std::error_code EC2;
  raw_fd_ostream OS2(outputLLPath, EC2);
  if (!EC2)
  {
    M.print(OS2, nullptr);
    OS2.flush();
  }
  else
  {
    errs() << "Error: " << outputLLPath << "\n";
    errs() << "Error: " << EC2.message() << "\n";
  }

  return true;

  return true; // Indicates that the module was not modified
}

static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM)
{

  PM.add(new AFLCoverage());
}

static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
