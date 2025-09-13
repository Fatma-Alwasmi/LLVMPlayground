#include "DivZeroAnalysis.h"

namespace dataflow {

//===----------------------------------------------------------------------===//
// DivZero Analysis Implementation
//===----------------------------------------------------------------------===//

/**
 * Implement your data-flow analysis.
 * 1. Define "flowIn" that joins the memory set of all incoming flows
 * 2. Define "transfer" that computes the semantics of each instruction.
 * 3. Define "flowOut" that flows the memory set to all outgoing flows
 * 4. Define "doAnalysis" that stores your results in "InMap" and "OutMap".
 * 5. Define "check" that checks if a given instruction is erroneous or not.
 */
static Domain MZ(Domain::MaybeZero);  // Maybe Zero
static Domain Z(Domain::Zero);        // Zero
static Domain NZ(Domain::NonZero);    // Non Zero
static Domain U(Domain::Uninit);      // Uninitialized

// define the following functions if needed (not compulsory to do so)
Memory* join(Memory *M1, Memory *M2) {
  Memory* Result = new Memory();
  /* Add your code here */

  /* Result will be the union of memories M1 and M2 */

  return Result;
}

bool equal(Memory *M1, Memory *M2) {
  /* Add your code here */
  /* Return true if the two memories M1 and M2 are equal */
}


void DivZeroAnalysis::flowIn(Instruction *I, Memory *In) {
  /* Add your code here */
}

void DivZeroAnalysis::transfer(Instruction *I, const Memory *In, Memory *NOut) {
  // memory = <std::string, Domain*> -> <opVar, abstract val> ex: <x, +>
  *NOut = *In; // assign NOut to In first
  std::string inst = variable(I); // this gets the whole ir instruction, example: %conv = zext i1 %cmp to i32. 
  //------------------[BINARY OPERATOR]------------------------//
  if (BinaryOperator *BO = dyn_cast<BinaryOperator>(I)){
    Value *a = BO->getOperand(0);
    Value *b = BO->getOperand(1);
    Domain *abstVal_a = &MZ;
    Domain *abstVal_b = &MZ;
    Domain *abstVal = &MZ;
    
    // check operand a:
    if(ConstantInt *CA = dyn_cast<ConstantInt>(a)){
      abstVal_a = CA->isZero() ? &Z: &NZ; 
    }else{
      auto it = In->find(variable(a));
      abstVal_a = (it != In->end()) ? it->second : &MZ;
    }

    // check operand b:
    if(ConstantInt *CB = dyn_cast<ConstantInt>(b)){
      abstVal_b = CB->isZero() ? &Z: &NZ; 
    }else{
      auto it = In->find(variable(b));
      abstVal_b = (it != In->end()) ? it->second : &MZ;
    }

    switch (BO->getOpcode()){
      case Instruction::Add:
        abstVal = Domain::add(abstVal_a, abstVal_b);
        break;
      case Instruction::Sub:
        abstVal = (a == b) ? &Z : Domain::sub(abstVal_a, abstVal_b);
        break;
      case Instruction::Mul:
        abstVal = Domain::mul(abstVal_a, abstVal_b);
        break;
      case Instruction::SDiv:
        abstVal = Domain::div(abstVal_a, abstVal_b);
        break;
      case Instruction::UDiv:
        abstVal = Domain::div(abstVal_a, abstVal_b);
        break;
      default:
        abstVal = &MZ;
      break;
    }
    (*NOut)[inst] = abstVal;
  //------------------[CAST INSTRUCTION]------------------------//
  } else if (CastInst *CAI = dyn_cast<CastInst>(I)){
    Value *Op = CAI->getOperand(0); // this gives the operand
    std::string opVar = variable(Op); // this stores the operand in opVar, example %cmp
    Domain *abstVal = &MZ;
    //is the operand constant? ex: %conv = zext i1 0 to i32. 
    if(ConstantInt *CI = dyn_cast<ConstantInt>(Op)){
      if(CI->isZero()) // is it zero?
        abstVal = &Z;
      else
        abstVal = &NZ;   
    } else{
      auto it = In->find(opVar);
      if(it != In->end()) // if iterator found the value
        abstVal = it->second; 
      else 
        abstVal = &MZ;
    }
    (*NOut)[inst] = abstVal;

  //------------------[COMPARE INSTRUCTION]------------------------//
  } else if (CmpInst *CMI = dyn_cast<CmpInst>(I)){ 
    /*for comapre instructions, we need to check if the operands are constants, if they are not
    we connot determine the abstract value of the return value since we wont know the values of the
    operands until run time.*/
    Value *a = CMI->getOperand(0);
    Value *b = CMI->getOperand(1);


    if(ConstantInt *CA = dyn_cast<ConstantInt>(a)){
      if(ConstantInt *CB = dyn_cast<ConstantInt>(b)){
        const APInt &A_val = CA->getValue();
        const APInt &B_val = CB->getValue();
        bool isTrue = false;
        switch (CMI->getPredicate())
        {
        case CmpInst::ICMP_EQ:
          isTrue = A_val.eq(B_val);
          break;
        case CmpInst::ICMP_NE:
          isTrue = A_val.ne(B_val);
          break;
        case CmpInst::ICMP_UGT:
          isTrue = A_val.ugt(B_val);
          break;
        case CmpInst::ICMP_UGE:
          isTrue = A_val.uge(B_val);
          break;
        case CmpInst::ICMP_ULT:
          isTrue = A_val.ult(B_val);
          break;
        case CmpInst::ICMP_ULE:
          isTrue = A_val.ule(B_val);
          break;
        case CmpInst::ICMP_SGT:
          isTrue = A_val.sgt(B_val);
          break;
        case CmpInst::ICMP_SGE:
          isTrue = A_val.sge(B_val);
          break;
        case CmpInst::ICMP_SLT:
          isTrue = A_val.slt(B_val);
          break;
        case CmpInst::ICMP_SLE:
          isTrue = A_val.sle(B_val);
          break;              
        default: break;
        }
        (*NOut)[inst] = isTrue ? &NZ: &Z;
      }
    }

  //------------------[BRANCH INSTRUCTION]------------------------//
  } else if (BranchInst *BI = dyn_cast<BranchInst>(I)){
    //do nothing 

  //------------------[isInput()]------------------------//
  } else if(CallInst *CI = dyn_cast<CallInst>(I)){
    if(isInput(CI))
      (*NOut)[inst] = &MZ;
  }
}

void DivZeroAnalysis::flowOut(Instruction *I, Memory *Pre, Memory *Post,  SetVector <Instruction *> &WorkSet) {
  /* Add your code here */
}

void DivZeroAnalysis::doAnalysis(Function &F) { 
  SetVector<Instruction *> WorkSet;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    WorkSet.insert(&(*I));
  }

  /* Add your code here */
  /* Basic Workflow-
       Visit instruction in WorkSet
       For each visited instruction I, construct its In memory by joining all memory sets of incoming flows (predecessors of I)
       Based on the type of instruction I and the In memory, populate the NOut memory
       Based on the previous Out memory and the current Out memory, check if there is a difference between the two and
         flow the memory set appropriately to all successors of I and update WorkSet accordingly
  */ 

}

bool DivZeroAnalysis::check(Instruction *I) {
  /* Add your code here */
  return false;
}

char DivZeroAnalysis::ID = 1;
static RegisterPass<DivZeroAnalysis> X("DivZero", "Divide-by-zero Analysis",
                                       false, false);
} // namespace dataflow
