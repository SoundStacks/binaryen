/*
 * Copyright 2021 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Reduces the amount of calls to functions that only run once. A "run-once"
// or "once" function is a function guarded by a global to make sure it runs a
// single time:
//
//  global foo$once = 0;
//
//  function foo() {
//    if (foo$once) return;
//    foo$once = 1;
//    ..do some work..
//  }
//
// If we verify that there are no other kind of sets to that global - that is,
// it is only used to guard this code - then we can remove subsequent calls to
// the function,
//
//  foo();
//  ..stuff..
//  foo(); // this call can be removed
//
// The latter call can be removed since it has definitely run by then.
//

#include <atomic>

#include "cfg/domtree.h"
#include "ir/utils.h"
#include "pass.h"
#include "support/unique_deferring_queue.h"
#include "wasm-builder.h"
#include "wasm.h"

namespace wasm {

namespace {

struct OptInfo {
  // Maps global names to whether they are possible indicators of "once"
  // functions. A "once" global has these properties:
  //
  //  * They are only ever written to with non-zero values.
  //  * They are never read from except in the beginning of a "once" function
  //    (otherwise, execution might be affected by the specific values of the
  //    global, instead of just using it to guard the "once" function).
  //
  // Those properties ensure that the global is monotonic in the sense that it
  // begins at zero and, if they are written to, will only receive a non-zero
  // value - they never return to 0.
  std::unordered_map<Name, std::atomic<bool>> onceGlobals;

  // Maps functions to whether they are "once", by indicating the global that
  // they use for that purpose. An empty name means they are not "once".
  std::unordered_map<Name, Name> onceFuncs;

  // For each function, the "once" globals that are definitely set after calling
  // it. If the function is "once" itself, that is included, but it also
  // includes any other "once" functions we definitely call, and so forth.
  // The "new" version is written to in each iteration, and then swapped with
  // the main one (to avoid reading and writing in parallel).
  std::unordered_map<Name, std::unordered_set<Name>> onceGlobalsSetInFuncs,
    newOnceGlobalsSetInFuncs;
};

struct Scanner : public WalkerPass<PostWalker<Scanner>> {
  bool isFunctionParallel() override { return true; }

  Scanner(OptInfo& optInfo) : optInfo(optInfo) {}

  Scanner* create() override { return new Scanner(optInfo); }

  // All the globals we read from. Any read of a global prevents us from
  // optimizing, unless it is the single read at the top of an "only" function
  // (as other reads might be used to check for the value of the global in
  // complex ways that we do not want to try to reason about).
  std::unordered_map<Name, Index> readGlobals;

  void visitGlobalGet(GlobalGet* curr) { readGlobals[curr->name]++; }

  void visitGlobalSet(GlobalSet* curr) {
    if (!curr->value->type.isInteger()) {
      // This is either a type we don't care about, or an unreachable set which
      // we also don't care about.
      return;
    }

    if (auto* c = curr->value->dynCast<Const>()) {
      if (c->value.getInteger() > 0) {
        // This writes a non-zero constant, which is what we hoped for.
        return;
      }
    }

    // This is not a constant, or it is zero - failure.
    optInfo.onceGlobals.at(curr->name) = false;
  }

  void visitFunction(Function* curr) {
    // TODO: support params and results?
    if (curr->getParams() == Type::none && curr->getResults() == Type::none) {
      auto global = getOnceGlobal(curr->body);
      if (global.is()) {
        // This is a "once" function, as best we can tell for now. Further
        // information may cause a problem, say, if the global is used in a bad
        // way in another function, so we may undo this.
        optInfo.onceFuncs.at(curr->name) = global;

        // We can ignore the get in the "once" pattern at the top of the
        // function.
        readGlobals[global]--;
      }
    }

    for (auto& kv : readGlobals) {
      auto global = kv.first;
      auto count = kv.second;
      if (count > 0) {
        // This global has reads we cannot reason about, so do not optimize it.
        optInfo.onceGlobals.at(global) = false;
      }
    }
  }

  // Check if a function body is in the "once" pattern. Return the name of the
  // global if so, or an empty name otherwise.
  //
  // TODO: If the "once" function is inlined, this pattern can show up in random
  //       places, and we can look for it there as well.
  Name getOnceGlobal(Expression* body) {
    // Look the pattern mentioned above:
    //
    //  function foo() {
    //    if (foo$once) return;
    //    foo$once = 1;
    //    ...
    //
    auto* block = body->dynCast<Block>();
    if (!block) {
      return Name();
    }
    auto& list = block->list;
    if (list.size() < 2) {
      return Name();
    }
    auto* iff = list[0]->dynCast<If>();
    if (!iff) {
      return Name();
    }
    auto* get = iff->condition->dynCast<GlobalGet>();
    if (!get) {
      return Name();
    }
    if (!iff->ifTrue->is<Return>() || iff->ifFalse) {
      return Name();
    }
    auto* set = list[1]->dynCast<GlobalSet>();

    // Note that we have already checked the set's value earlier, but we do need
    // it to not be unreachable (so it is actually set).
    if (!set || set->name != get->name || set->type == Type::unreachable) {
      return Name();
    }
    return get->name;
  }

private:
  OptInfo& optInfo;
};

// Information in a basic block. We track relevant expressions, which are calls
// calls to "once" functions, and writes to "once" globals.
struct BlockInfo {
  std::vector<Expression*> exprs;
};

// Performs optimization in all functions. This reads onceGlobalsSetInFuncs in
// order to know what "once" globals are written by each function (so that when
// we see a call, we can infer things), and when it is finishes with a function
// it has learned which "once" globals it must set, and it then writes out
// newOnceGlobalsSetInFuncs with that result. Later iterations will then use
// those values in place of onceGlobalsSetInFuncs, which propagates things to
// callers. This in effect mixes local optimization with the global
// propagation - as we need to run the full local optimization in order to infer
// the new values for onceGlobalsSetInFuncs, that is unavoidable (in principle,
// we could also do a full propagation to a fixed point in between running
// local optimization, but that would require more code - it might be more
// efficient, though).
struct Optimizer
  : public WalkerPass<CFGWalker<Optimizer, Visitor<Optimizer>, BlockInfo>> {
  bool isFunctionParallel() override { return true; }

  Optimizer(OptInfo& optInfo) : optInfo(optInfo) {}

  Optimizer* create() override { return new Optimizer(optInfo); }

  void visitGlobalSet(GlobalSet* curr) {
    if (currBasicBlock) {
      currBasicBlock->contents.exprs.push_back(curr);
    }
  }

  void visitCall(Call* curr) {
    if (currBasicBlock) {
      currBasicBlock->contents.exprs.push_back(curr);
    }
  }

  void doWalkFunction(Function* func) {
    using Parent =
      WalkerPass<CFGWalker<Optimizer, Visitor<Optimizer>, BlockInfo>>;

    // Walk the function to builds the CFG.
    Parent::doWalkFunction(func);

    // Build a dominator tree, which then tells us what to remove: if a call
    // appears in block A, then we do not need to make any calls in any blocks
    // dominated by A.
    DomTree<Parent::BasicBlock> domTree(basicBlocks);

    // Perform the work by going through the blocks in reverse postorder and
    // filling out which "once" globals have been written to.
    auto numBlocks = basicBlocks.size();
    if (numBlocks == 0) {
      return;
    }

    // Each index in this vector is the set of "once" globals written to in the
    // basic block with the same index.
    std::vector<std::unordered_set<Name>> onceGlobalsWrittenVec;
    onceGlobalsWrittenVec.resize(numBlocks);

    for (Index i = 0; i < basicBlocks.size(); i++) {
      auto* block = basicBlocks[i].get();

      // Note that we take a reference here, which is how the data we accumulate
      // ends up stored. The blocks we dominate will see it later.
      auto& onceGlobalsWritten = onceGlobalsWrittenVec[i];

      // Note information from our immediate dominator.
      // TODO: we could also intersect information from all of our preds.
      auto parent = domTree.iDoms[i];
      if (parent == domTree.nonsense) {
        // This is either the entry node (which we need to process), or an
        // unreachable block (which we do not - leave that to DCE).
        if (i > 0) {
          continue;
        }
      } else {
        // This block has an immediate dominator, so we know that everything
        // written to there can be assumed written.
        onceGlobalsWritten = onceGlobalsWrittenVec[parent];
      }

      // Process the block's expressions.
      auto& exprs = block->contents.exprs;
      for (auto* expr : exprs) {
        // Given the name of a "once" global that is written by this
        // instruction, optimize.
        auto optimizeOnce = [&](Name globalName) {
          assert(optInfo.onceGlobals.at(globalName));
          if (onceGlobalsWritten.count(globalName)) {
            // This global has already been written, so this expr is not needed,
            // regardless of whether it is a global.set or a call.
            //
            // Note that assertions below verify that there are no children that
            // we need to keep around, and so we can just nop the entire node.
            ExpressionManipulator::nop(expr);
          } else {
            // From here on, this global is set, hopefully allowing us to
            // optimize away others.
            onceGlobalsWritten.insert(globalName);
          }
        };

        if (auto* set = expr->dynCast<GlobalSet>()) {
          if (optInfo.onceGlobals.at(set->name)) {
            // This global is written.
            assert(set->value->is<Const>());
            optimizeOnce(set->name);
          }
        } else if (auto* call = expr->dynCast<Call>()) {
          if (optInfo.onceFuncs.at(call->target).is()) {
            // The global used by the "once" func is written.
            assert(call->operands.empty());
            optimizeOnce(optInfo.onceFuncs.at(call->target));
            continue;
          }

          // This is not a call to a "once" func. However, we may have inferred
          // that it definitely sets some "once" globals before it returns, and
          // we can use that information.
          for (auto globalName :
               optInfo.onceGlobalsSetInFuncs.at(call->target)) {
            onceGlobalsWritten.insert(globalName);
          }
        } else {
          WASM_UNREACHABLE("invalid expr");
        }
      }
    }

    // As a result of the above optimization, we know which "once" globals are
    // definitely written in this function. Regardless of whether this is a
    // "once" function itself, that set of globals can be used in further
    // optimizations, as any call to this function must set those.
    // TODO: Aside from the entry block, we could intersect all the exit blocks.
    optInfo.newOnceGlobalsSetInFuncs[func->name] =
      std::move(onceGlobalsWrittenVec[0]);
  }

private:
  OptInfo& optInfo;
};

} // anonymous namespace

struct OnceReduction : public Pass {
  void run(PassRunner* runner, Module* module) override {
    OptInfo optInfo;

    // Fill out the initial data.
    for (auto& global : module->globals) {
      // For a global to possibly be "once", it must be initialized to a
      // constant. As we scan code we will turn this into false if we see
      // anything that proves the global is not "once".
      //   * Note that we don't check that the constant is zero - that is fine
      //     for us to optimize, though it does indicate that the once function
      //     will never ever run, which we could optimize further. TODO
      //   * TODO: non-integer types?
      optInfo.onceGlobals[global->name] = global->type.isInteger() &&
                                          !global->imported() &&
                                          global->init->is<Const>();
    }
    for (auto& func : module->functions) {
      // Fill in the map so that it can be operated on in parallel.
      optInfo.onceFuncs[func->name] = Name();
    }

    // Scan the module to find out which globals and functions are "once".
    Scanner(optInfo).run(runner, module);

    // Combine the information. We found which globals appear to be "once", but
    // other information may have proven they are not so, in fact. Specifically,
    // for a function to be "once" we need its global to also be such.
    for (auto& kv : optInfo.onceFuncs) {
      Name& onceGlobal = kv.second;
      if (onceGlobal.is() && !optInfo.onceGlobals[onceGlobal]) {
        onceGlobal = Name();
      }
    }

    // Optimize using what we found. Keep iterating while we find things to
    // optimize, which we estimate using a counter of the total number of once
    // globals set by functions: as that increases, it means we are propagating
    // useful information.
    // TODO: limit # of iterations?
    Index lastOnceGlobalsSet = 0;

    // First, initialize onceGlobalsSetInFuncs for the first iteration, by
    // ensuring each item is present, and adding the "once" global for "once"
    // funcs.
    bool foundOnce = false;
    for (auto& func : module->functions) {
      // Either way, at least fill the data structure for parallel operation.
      auto& set = optInfo.onceGlobalsSetInFuncs[func->name];

      auto global = optInfo.onceFuncs[func->name];
      if (global.is()) {
        set.insert(global);
        foundOnce = true;
      }
    }

    if (!foundOnce) {
      // Nothing to optimize.
      return;
    }

    while (1) {
      // Initialize all the items in the new data structure that will be
      // populated.
      for (auto& func : module->functions) {
        optInfo.newOnceGlobalsSetInFuncs[func->name];
      }

      Optimizer(optInfo).run(runner, module);

      optInfo.onceGlobalsSetInFuncs =
        std::move(optInfo.newOnceGlobalsSetInFuncs);

      // Count how many once globals are set, and see if we have any more work
      // to do.
      Index currOnceGlobalsSet = 0;
      for (auto& kv : optInfo.onceGlobalsSetInFuncs) {
        auto& globals = kv.second;
        currOnceGlobalsSet += globals.size();
      }
      assert(currOnceGlobalsSet >= lastOnceGlobalsSet);
      if (currOnceGlobalsSet > lastOnceGlobalsSet) {
        lastOnceGlobalsSet = currOnceGlobalsSet;
        continue;
      }
      return;
    }
  }
};

Pass* createOnceReductionPass() { return new OnceReduction(); }

} // namespace wasm
