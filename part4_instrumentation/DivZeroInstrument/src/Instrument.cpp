#include "Instrument.h"

using namespace llvm;

namespace instrument {

static const char *SanitizerFunctionName = "__sanitize__";
static const char *CoverageFunctionName = "__coverage__";

/*
 * Implement divide-by-zero sanitizer.
 */
void instrumentSanitize(Module *M, Function &F, Instruction &I) {
  /* Add you code here */
}

/*
 * Implement code coverage instrumentation.
 */
void instrumentCoverage(Module *M, Function &F, Instruction &I) {
  /* Add you code here */
}

bool Instrument::runOnFunction(Function &F) {
  Module *M = F.getParent();
  // iterate over the basic blocks in function F
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    // iterate over the instructions in basic block BB
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      //if instruction has debug info create CallInst to __coverage__
      if(I->getDebugLoc())
        instrumentCoverage(M, F, *I);
      if(BinaryOperator *BO = dyn_cast<BinaryOperator>(I)){
        if(BO->getOpcode() == Instruction::SDiv || BO->getOpcode() == Instruction::UDiv){
          //create CallInst to __sanitize__
          instrumentSanitize(M, F, *I);
        }
      }
    }
  }
  return true;
}

char Instrument::ID = 1;
static RegisterPass<Instrument>
    X("Instrument", "Instrumentations for Dynamic Analysis", false, false);

} // namespace instrument
