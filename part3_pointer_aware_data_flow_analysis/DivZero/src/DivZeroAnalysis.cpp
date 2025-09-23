#include "DivZeroAnalysis.h"

namespace dataflow {

//===----------------------------------------------------------------------===//
// Dataflow Analysis Implementation
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
/* example:
  M1 = {"%x" → NonZero, "%y" → Zero}
  M2 = {"%x" → Zero, "%z" → MaybeZero}
  Result = {"%x" → MaybeZero, "%y" → Zero, "%z" → MaybeZero}*/
Memory* join(Memory *M1, Memory *M2) {
  Memory* Result = new Memory();
  /* Result will be the union of memories M1 and M2 */

  // first: add all variables from M1
  for(auto &pair : *M1){
    std::string var = pair.first; //get variable
    Domain *abstVal = pair.second; //get abst val of var

    //check if the var is in M2
    auto it = M2->find(var);
    if(it != M2->end()){ //if found
      (*Result)[var] = Domain::join(abstVal, it->second); //join the two abstract values
    } else{
      // var is only in M1, then just copy it
      (*Result)[var] = abstVal;
    }
  }
  // second, add varialbe that are only in M2
  for (auto &pair : *M2){
    std::string var = pair.first;
    if(Result->find(var) == Result->end()){ // if var not already in Result
      (*Result)[var] = pair.second; // just copy it
    }
  }
  return Result;
}

bool equal(Memory *M1, Memory *M2) {
  /* Return true if the two memories M1 and M2 are equal */
  if(M1->size() != M2->size()) return false; // if sizes are different, they are not equal
  for(auto &pair : *M1){
    std::string var1 = pair.first;
    Domain *abstVal_1 = pair.second;

    auto it = M2->find(var1); // look for var1 in M2
    if(it == M2->end()) return false; //if not found return false
    Domain *abstVal_2 = it->second;

    if(abstVal_1->Value != abstVal_2->Value) return false; // if abstract values are different, return false
  }
  return true; // if all variables and their abstract values are the same, return true
}


void DivZeroAnalysis::flowIn(Instruction *I, Memory *In) {
  /* Add your code here */
  //flow in is the union of all the predecessors' out memories
  Memory *InUnion = new Memory(); // start with an empty memory InUnion = < >
  std::vector<Instruction *> preds = getPredecessors(I); // get predecessors of I

  // get all the predecessors' of I in the block
  for(Instruction *P : preds){
    //OutMap[P] = current instruction P -> Memory* { "variable" → abstract value }
    //*POut = Memory* { "variable" → abstract value }
    Memory *POut = OutMap[P]; // get the out memory of the predecessor.
    if(!POut) continue; // if null, skip
    Memory *NewIn = join(InUnion, POut); // join the current InUnion with the predecessor's out memory
    delete InUnion; // free the memory
    InUnion = NewIn; // update InUnion to the new joined memory
  }
  *In = *InUnion; // assign the result to In
  delete InUnion; // free the memory
}


void DivZeroAnalysis::transfer(Instruction *I, const Memory *In, Memory *NOut,
                               PointerAnalysis *PA,
                               SetVector<Value *> PointerSet) {
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
      }else
        (*NOut)[inst] = &MZ; // a is constant, b is not constant
    } else if(ConstantInt *CB = dyn_cast<ConstantInt>(b)){
      (*NOut)[inst] = &MZ; // b is constant, a is not constant
    } else
      (*NOut)[inst] = &MZ; // both are not constants, we cannot determine the abstract value of the return value

  //------------------[BRANCH INSTRUCTION]------------------------//
  } else if (BranchInst *BI = dyn_cast<BranchInst>(I)){
    //do nothing 

  //------------------[isInput()]------------------------//
  } else if(CallInst *CI = dyn_cast<CallInst>(I)){
    if(isInput(CI) || CI->getType()->isIntegerTy())
      (*NOut)[inst] = &MZ;

  //------------------[STORE INSTRUCTION]------------------------//
  } else if(StoreInst *SI = dyn_cast<StoreInst>(I)){
    // example of store isnt: 
    //C:
      // int a = 5;
      // int *p = &a <-- STORE
    //IR:
      // %a = alloca i32
      // %p = alloca i32*
      //store i32* %a, i32** %p
    
    Value *valOp = SI->getValueOperand(); // value to be stored, example: %a
    Value *ptrOp = SI->getPointerOperand(); // pointer to where the value is
    
    // check if the vlaue we are storing is an int
    if(valOp->getType()->isIntegerTy()){
      Domain *abstVal = &MZ;
      //if so check if its a constant. ex: C: int* p = 5; IR: store i32 5, i32* %p
      if(ConstantInt *CI = dyn_cast<ConstantInt>(valOp)){
        abstVal = CI->isZero() ? &Z : &NZ;

      // if not constant, look up in In map. ex: C: int a = 5; int* p = &a; IR: %a = alloc i32; store i32* %a, i32** %p
      } else{
        auto it = In->find(variable(valOp));
        if(it != In->end())
          abstVal = it->second;
      }

      std::string ptrVar = variable(ptrOp); // get the pointer variable, example: %p
      (*NOut)[ptrVar] = abstVal; // update the abstract value of the pointer variable

      // check if the pointer is an alias of other pointers
      for(Value *P : PointerSet){
        std::string P_var = variable(P);
        std::string ptrOp_var = variable(ptrOp);
        if(P != ptrOp && PA->alias(ptrOp_var, P_var)) // if they alias i.e if 2 pointers have a possiblity of pointing to the same memory location
          (*NOut)[P_var] = abstVal; // update the abstract value of the aliased pointer since they point to the same memory location
      }
    }
  //------------------[LOAD INSTRUCTION]------------------------//
  } else if(LoadInst *LI = dyn_cast<LoadInst>(I)){
    // example of load instruction
    //C:
      // int a = 5;
      // *ptr = &a;
      // int c = *ptr <- LOAD
      // int d = 1/c
    //IR:
      // %a = alloca i32;
      // %ptr = alloca i32*
      // store i32 5; i32* %a
      // store i32* %a, i32** %p
      // %c = alloca i32;
      // %1 = load i32, i32** %ptr
      // %2 = load i32, i32* %1 <- LOAD
      // store i32** %2, i32* c
    
    Value *ptrOp = LI->getPointerOperand(); // get the pointer operand, example: %1

    //check if we are loading the pointer to an integer, ex: int c = *p; checks if c is an int, so we check the return type of the instruction
    // we cannot do a similar check in the store instruciton because the store instruction doesnt return a type, it just stores, so we got the type of value
    // we were storing
    if(LI->getType()->isIntegerTy()){
      Domain* abstVal = &MZ;
      std::string ptrVal = variable(ptrOp);

      auto it = In->find(ptrVal); // look up the pointer variable in In map
      if(it != In->end())
        abstVal = it->second; // get the abstract value of the pointer variable

      
      // now we need to see there is another pointer pointing to the same memory location as ptrOp
      // if so we get its abstract value, then we join all the abstract values of all the pointers
      //ex: 
      // int f(){
      //   int a;
      //   int *c = &a;  // c points to a
      //   int *d = &a;  // d points to a (c and d alias!)
      //   int *e = &a;  // e points to a (all three alias!)
      //   *c = 0;       // Store 0 through pointer c
      //   int x = 1 / *d; // Load through pointer d - what value do we get?
      //   return 0;
      // }
      // if we just looked up d, we would get MaybeZero, since we dont know what value a has
      // but since c and d alias, and we know c was used to store 0, we can conclude that a is Zero
      // so we need to look up all the pointers that alias ptrOp
      // so join(abstVal, abstVal_of_c, abstVal_of_e) = Zero
      // this will give us a sound approximation of the abstract value of the memory location being pointed to
      
      for(Value *P : PointerSet){
        std::string P_var = variable(P);
        if(P != ptrOp && PA->alias(ptrVal, P_var)){
          auto aliasIt = In->find(P_var);
          if(aliasIt != In->end())
            abstVal = Domain::join(abstVal, aliasIt->second);
        }
      }
      (*NOut)[inst] = abstVal; // update the abstract value of the loaded variable
    }   
  }
}

void DivZeroAnalysis::flowOut(Instruction *I, Memory *Pre, Memory *Post,  SetVector <Instruction *> &WorkSet) {
  // first check if the memory state has changed after the transfer function
  if(!equal(Pre,Post)){
    // if changed, we need to reanalyze all the successors of I
    OutMap[I] = Post; // update the OutMap with the new state

    std::vector<Instruction *> succs = getSuccessors(I);
    for(Instruction *S : succs){
      WorkSet.insert(S); // add the successor to the workset
    }
    }else{
      // if not changed, just update OutMap
      OutMap[I] = Post;
    }
}

void DivZeroAnalysis::doAnalysis(Function &F, PointerAnalysis *PA) {
  SetVector<Value *> PointerSet;
  SetVector<Instruction *> WorkSet;

  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    InMap[&(*I)] = new Memory();
    OutMap[&(*I)] = new Memory();

    WorkSet.insert(&(*I));
    //only if instruction is a pointer, add to pointer set
    if(I->getType()->isPointerTy())
      PointerSet.insert(&(*I));
  }

  /* Add your code here */
  /* Basic Workflow-
       Visit instruction in WorkSet
       For each visited instruction I, construct its In memory by joining all memory sets of incoming flows (predecessors of I)
       Based on the type of instruction I and the In memory, populate the NOut memory. Take the pointer analysis into consideration for this step
       Based on the previous Out memory and the current Out memory, check if there is a difference between the two and
         flow the memory set appropriately to all successors of I and update WorkSet accordingly
  */ 
   //chaotic iteration algorithm

  for (Argument &arg : F.args()){
    std::string argVar = variable(&arg);
    Instruction *firstInst = &*(inst_begin(F));
    if(arg.getType()->isIntegerTy()){
      // if the argument is an integer, we initialize it to MaybeZero
      (*InMap[firstInst])[argVar] = &MZ;
    } else if(arg.getType()->isPointerTy()){
      // if the argument is a pointer, we initialize it to MaybeZero as well
      PointerSet.insert(&arg); // add the argument to the pointer set as well
      (*InMap[firstInst])[argVar] = &NZ; // we assume that the pointer argument is non-zero 
    }
  }
  while(!WorkSet.empty()){
    Instruction *I = *WorkSet.begin();
    WorkSet.remove(I);

    Memory *In = InMap[I];
    Memory *OldOut = OutMap[I];
    Memory *NewOut = new Memory();

    flowIn(I, In);
    transfer(I, In, NewOut, PA, PointerSet);
    flowOut(I, OldOut, NewOut, WorkSet);
  }
}

bool DivZeroAnalysis::check(Instruction *I) {
  if (BinaryOperator *BO = dyn_cast<BinaryOperator>(I)){
    if(BO->getOpcode() == Instruction::SDiv || BO->getOpcode() == Instruction::UDiv){
      Memory *In = InMap[I]; //get the abstract state just before running the instruction
      if(!In) return false;
      Value *denominator = BO->getOperand(1);

      if(ConstantInt *CI = dyn_cast<ConstantInt>(denominator))
        return CI->isZero(); // if constant, check if its zero 
      
      // non constant, we look up in the In memory 
      std::string div_var = variable(denominator);
      Domain *abstVal = &MZ; // fallback
      auto it = In->find(div_var); // look for the variable in the In memory
      if(it != In->end()) // if iterator found the value
        abstVal = it->second; // get the abstract value

      if(abstVal->Value == Domain::Zero || abstVal->Value == Domain::MaybeZero || abstVal->Value == Domain::Uninit)
        return true; // potential divide by zero
      
      return false; // not a divide by zero
    }
    return false; // not a div instruction
  }
  return false; // not a binary operator
}

char DivZeroAnalysis::ID = 1;
static RegisterPass<DivZeroAnalysis> X("DivZero", "Divide-by-zero Analysis",
                                       false, false);
} // namespace dataflow
