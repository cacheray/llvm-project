add_llvm_library(LLVMMallocTracker MODULE
  MallocTracker.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Core
  Support
  )
