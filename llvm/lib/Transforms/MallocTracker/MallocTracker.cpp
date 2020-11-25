#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <random>
#include <time.h>
#include <vector>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "MallocTracker"

STATISTIC(MallocCounter, "should count mallocs");

// TODO clean up code, split up into methods so that code is less... spagetti
// TODO fix so that the prints (to errs()) are debug flag dependant
namespace {
struct MallocTracker : public FunctionPass { // TODO consider running on module
                                             // insted of on function
  static char ID;
  MallocTracker() : FunctionPass(ID) {
  }

  bool runOnFunction(Function &FF) {
    // Functions in LLVM consists of BasicBlocks that in turn consist of
    // instructions
    for (auto &BB : FF) {
      for (Instruction &I : BB) {
        // Check if the instruction is a Call Instruction (as malloc is an
        // external call)
        if (auto *C = dyn_cast<CallInst>(&I)) {
          Function *fun = C->getCalledFunction();

          if (fun->getName() ==
              "malloc") { // TODO find better way to identify mallocs
            addMallocCallback(C, &I);
          } else if (fun->getName() == "free") {
            addFreeCallback(C, &I);
          } else if (fun->getName() == "realloc") {
            addReallocCallback(C, &I);
          } else if (fun->getName() == "calloc") {
            addCallocCallback(C, &I);
          }
        }
      }
    }
    return false;
  }

  Constant *getCastTarget(Instruction *I) {
    Constant *castTargetPtr;
    IRBuilder<> builder(I);
    if (auto cast = dyn_cast<BitCastInst>(I)) {
      auto dest = cast->getDestTy();
      if (dest->isPointerTy()) {
        if (dest->getPointerElementType()->isStructTy()) {
          const char *castTarget =
              dest->getPointerElementType()->getStructName().data();
          castTargetPtr = builder.CreateGlobalStringPtr(castTarget);
        } else {
          std::string type_str;
          raw_string_ostream rso(type_str);
          dest->print(rso);
          castTargetPtr = builder.CreateGlobalStringPtr(rso.str());
          rso.flush(); // TODO this can't possibly be the best way to extract
                       // type data
        }
      }
    } else {
      castTargetPtr = builder.CreateGlobalStringPtr("void*");
    }
    return castTargetPtr;
  }

  void addMallocCallback(CallInst *C, Instruction *I) {
    errs() << "malloc recorded \n";
    Module *M = I->getModule();

    // Create the FunctionType. This needs to specify return type as well as the
    // types of the arguments of the function
    std::vector<Type *> params;
    params.push_back(IntegerType::getInt64Ty(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    FunctionType *ft = FunctionType::get(
        FunctionType::getVoidTy(I->getContext()), params, false);

    // Build the function prototype
    FunctionCallee c = M->getOrInsertFunction("__malloc_recorded", ft);
    Function *testing = cast<Function>(c.getCallee());

    // Extract what the void pointer that malloc returns is cast to (so we can
    // identify the struct casts)
    Constant *castTargetPtr = getCastTarget(I->getNextNonDebugInstruction());

    // Create the parameter values that will be put into the callback
    std::vector<Value *> arguments;
    // Extract the number of bytes allocated in this malloc
    Use *arg = C->arg_begin();
    if (auto CI = dyn_cast<ConstantInt>(arg)) {
      arguments.push_back(ConstantInt::get(
          IntegerType::getInt64Ty(M->getContext()), CI->getZExtValue()));
    } else {
      arguments.push_back(arg->get()); // TODO: make sure this is the right byte
                                       // size! On my system its a Int64Ty, but
                                       // that isnt necessarily always the case
    }
    arguments.push_back(castTargetPtr);
    arguments.push_back(C);

    // Create an instruction that contains the Function and parameters
    Instruction *newInst =
        CallInst::Create(testing->getFunctionType(), testing, arguments, "", I);
    newInst->removeFromParent();

    // Inject our new instruction
    newInst->insertAfter(
        I); // TODO: consider more closely where the instruction should be
            // inserted! Should it be inserted after "I" instead if there was not
            // cast of the void pointer?
  }

  // this method is extremely similar to "addMallocCallback", basically a ctrl+c
  // and ctrl+v
  void addCallocCallback(CallInst *C, Instruction *I) {
    errs() << "calloc recorded \n";
    Module *M = I->getModule();

    std::vector<Type *> params;
    params.push_back(IntegerType::getInt64Ty(I->getContext()));
    params.push_back(IntegerType::getInt64Ty(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    FunctionType *ft = FunctionType::get(
        FunctionType::getVoidTy(I->getContext()), params, false);

    FunctionCallee c = M->getOrInsertFunction("__calloc_recorded", ft);
    Function *testing = cast<Function>(c.getCallee());

    Constant *castTargetPtr = getCastTarget(I->getNextNonDebugInstruction());

    std::vector<Value *> arguments;
    // add both calloc arguments to the callback
    Use *arg1 = C->arg_begin();
    if (auto CI = dyn_cast<ConstantInt>(arg1)) {
      arguments.push_back(ConstantInt::get(
          IntegerType::getInt64Ty(M->getContext()), CI->getZExtValue()));
    } else {
      arguments.push_back(arg1->get());
    }
    Use *arg2 = C->arg_begin();
    arg2++; // get second argument in the argument list
    if (auto CI = dyn_cast<ConstantInt>(arg2)) {
      arguments.push_back(ConstantInt::get(
          IntegerType::getInt64Ty(M->getContext()), CI->getZExtValue()));
    } else {
      arguments.push_back(arg2->get());
    }
    arguments.push_back(castTargetPtr);
    arguments.push_back(C);

    Instruction *newInst =
        CallInst::Create(testing->getFunctionType(), testing, arguments, "", I);
    newInst->removeFromParent();

    newInst->insertAfter(I);
  }

  // this method is extremely similar to "addMallocCallback", basically a ctrl+c
  // and ctrl+v
  void addReallocCallback(CallInst *C, Instruction *I) {
    errs() << "realloc recorded \n";
    Module *M = I->getModule();

    std::vector<Type *> params;
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    params.push_back(IntegerType::getInt64Ty(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    FunctionType *ft = FunctionType::get(
        FunctionType::getVoidTy(I->getContext()), params, false);

    FunctionCallee c = M->getOrInsertFunction("__realloc_recorded", ft);
    Function *testing = cast<Function>(c.getCallee());

    Constant *castTargetPtr = getCastTarget(I->getNextNonDebugInstruction());

    std::vector<Value *> arguments;
    // add both calloc arguments to the callback
    Use *arg1 = C->arg_begin();
    if (!arg1->get()->getType()->isPointerTy()) {
      errs() << "Something went wrong trying to handle a \"realloc\" call. The "
                "first value passed to realloc appears to not be a pointer";
    }
    arguments.push_back(arg1->get());
    Use *arg2 = C->arg_begin();
    arg2++; // get second argument in the argument list
    if (auto CI = dyn_cast<ConstantInt>(arg2)) {
      arguments.push_back(ConstantInt::get(
          IntegerType::getInt64Ty(M->getContext()), CI->getZExtValue()));
    } else {
      arguments.push_back(arg2->get());
    }
    arguments.push_back(castTargetPtr);
    arguments.push_back(C);

    Instruction *newInst =
        CallInst::Create(testing->getFunctionType(), testing, arguments, "", I);
    newInst->removeFromParent();

    newInst->insertAfter(I);
  }

  void addFreeCallback(CallInst *C, Instruction *I) {
    Module *M = I->getModule();
    errs() << "found a free\n";

    std::vector<Type *> params;
    params.push_back(Type::getInt8PtrTy(I->getContext()));
    FunctionType *ft = FunctionType::get(
        FunctionType::getVoidTy(I->getContext()), params, false);

    FunctionCallee c = M->getOrInsertFunction("__free_recorded", ft);
    Function *testing = cast<Function>(c.getCallee());

    Use *arg = C->arg_begin();
    if (!arg->get()->getType()->isPointerTy()) {
      errs() << "Something went wrong trying to handle a \"free\" call. The "
                "value passed to free appears to not be a pointer";
    }

    std::vector<Value *> arguments;
    arguments.push_back(arg->get());

    Instruction *newInst =
        CallInst::Create(testing->getFunctionType(), testing, arguments, "", I);
    newInst->removeFromParent();

    newInst->insertAfter(I);
  }
};
} // namespace

char MallocTracker::ID = 0;
static RegisterPass<MallocTracker> X("mallocTr", "Test pass to find mallocs");

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new MallocTracker());
                                });
