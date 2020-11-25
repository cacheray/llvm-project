# MallocTracker

This custom LLVM pass generates compiler instrumentation to associate an address
with a type based on heuristic inspection of `malloc`, `calloc`, `realloc` and
`free` calls.

The pass generates calls to one runtime hook for every allocator func:

* `void __malloc_recorded(long byte_size, char *type_name, void *addr);`
* `void __calloc_recorded(long nmemb, long size, char *name, void *addr);`
* `void __realloc_recorded(void *old_addr, long size, char *name, void *addr);`
* `void __free_recorded(void *addr);`

These are implemented by the Cacheray runtime to annotate a memory region with a
type name.

Note that MallocTracker is not strictly required for Cacheray, but it can
improve type awareness significantly.

TODO: Describe command-line usage
