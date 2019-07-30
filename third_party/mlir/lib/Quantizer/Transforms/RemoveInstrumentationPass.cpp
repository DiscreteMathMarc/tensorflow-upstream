//===- RemoveInstrumentationPass.cpp - Removes instrumentation ------------===//
//
// Copyright 2019 The MLIR Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================
//
// This file defines a pass to remove any instrumentation ops. It is often one
// of the final steps when performing quantization and is run after any
// decisions requiring instrumentation have been made.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/QuantOps/QuantOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Quantizer/Transforms/Passes.h"

using namespace mlir;
using namespace mlir::quantizer;
using namespace mlir::quant;

namespace {

class RemoveInstrumentationPass
    : public FunctionPass<RemoveInstrumentationPass> {
  void runOnFunction() override;
};

template <typename OpTy>
class RemoveIdentityOpRewrite : public RewritePattern {
public:
  RemoveIdentityOpRewrite(MLIRContext *context)
      : RewritePattern(OpTy::getOperationName(), 1, context) {}

  PatternMatchResult matchAndRewrite(Operation *op,
                                     PatternRewriter &rewriter) const override {
    assert(op->getNumOperands() == 1);
    assert(op->getNumResults() == 1);

    rewriter.replaceOp(op, op->getOperand(0));
    return matchSuccess();
  }
};

} // end anonymous namespace

void RemoveInstrumentationPass::runOnFunction() {
  OwningRewritePatternList patterns;
  auto func = getFunction();
  auto *context = &getContext();
  patterns.push_back(
      llvm::make_unique<RemoveIdentityOpRewrite<StatisticsOp>>(context));
  patterns.push_back(
      llvm::make_unique<RemoveIdentityOpRewrite<StatisticsRefOp>>(context));
  patterns.push_back(
      llvm::make_unique<RemoveIdentityOpRewrite<CoupledRefOp>>(context));
  applyPatternsGreedily(func, std::move(patterns));
}

FunctionPassBase *mlir::quantizer::createRemoveInstrumentationPass() {
  return new RemoveInstrumentationPass();
}

static PassRegistration<RemoveInstrumentationPass>
    pass("quantizer-remove-instrumentation",
         "Removes instrumentation and hints which have no effect on final "
         "execution");
