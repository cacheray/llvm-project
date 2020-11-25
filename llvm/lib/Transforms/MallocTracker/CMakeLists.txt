add_llvm_component_library(LLVMMallocTracker
  MallocTracker.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Core
  Support
  )
