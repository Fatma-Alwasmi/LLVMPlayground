#include "Instrument.h"

using namespace llvm;

namespace instrument {

static const char *SanitizerFunctionName = "__sanitize__";
static const char *CoverageFunctionName = "__coverage__";

/*
 * Implement divide-by-zero sanitizer.
 */
void instrumentSanitize(Module *M, Function &F, Instruction &I) {
  Value *divisor = I.getOperand(1);
  int line = I.getDebugLoc().getLine();
  int col = I.getDebugLoc().getCol();

  // declare __sanatize__ in the module (its like funciton prototype in C)
  // since we will be calling __sanatize__ in that module, i need to declare 
  // the types that the funciton expected in the context of that module,
  // that why i do Type::getVoidTy(M->getContext())
  FunctionCallee sanitizerFuncDecl = M->getOrInsertFunction(
      SanitizerFunctionName,
      Type::getVoidTy(M->getContext()),
      Type::getInt32Ty(M->getContext()),
      Type::getInt32Ty(M->getContext()),
      Type::getInt32Ty(M->getContext())
  );

  //create constants for the line and col
  Value *lineVal = ConstantInt::get(Type::getInt32Ty(M->getContext()), line);
  Value *colVal = ConstantInt::get(Type::getInt32Ty(M->getContext()), col);
  //create call to __sanatize__
  //this will a call instruction in the module before instruction I passing
  //lineVal, colVal and divisor as arguments
  CallInst::Create(sanitizerFuncDecl, {divisor, lineVal, colVal}, "", &I);

}

/*
 * Implement code coverage instrumentation.
 */
void instrumentCoverage(Module *M, Function &F, Instruction &I) {
  int line = I.getDebugLoc().getLine();
  int col = I.getDebugLoc().getCol();

  // declare __coverage__ in the module (its like funciton prototype in C)
  // since we will be calling __coverage__ in that module, i need to declare 
  // the types that the funciton expected in the context of that module,
  // that why i do Type::getVoidTy(M->getContext())
  FunctionCallee coverageFuncDecl = M->getOrInsertFunction(
      CoverageFunctionName,
      Type::getVoidTy(M->getContext()),
      Type::getInt32Ty(M->getContext()),
      Type::getInt32Ty(M->getContext())
  );

  //create constants for the line and col
  Value *lineVal = ConstantInt::get(Type::getInt32Ty(M->getContext()), line);
  Value *colVal = ConstantInt::get(Type::getInt32Ty(M->getContext()), col);
  //create call to __coverage__
  //this will a call instruction in the module before instruction I passing
  //lineVal and colVal as arguments
  CallInst::Create(coverageFuncDecl, {lineVal, colVal}, "", &I);

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
