# Cacheray LLVM fork

All Cacheray-related changes so far are on a branch called, simply, `cacheray`.

Use this branch to build a customized `clang` compiler or LLVM pass for `opt`
with support for additional `MallocTracker` instrumentation.

## For maintainers

To resync with upstream LLVM, use the following recipe:
```
$ cd llvm-project

# only do this if you don't already have a remote for upstream
$ git remote add upstream git@github.com:llvm/llvm-project.git

# resync master branch
$ git checkout master
$ git pull upstream master

# rebase cacheray branch
$ git checkout cacheray
$ git rebase master

# assuming conflicts resolved, force-push new cacheray HEAD to cacheray fork
# and gently push new master to master.
$ git push -f origin cacheray:cacheray
$ git push origin master:master
```
