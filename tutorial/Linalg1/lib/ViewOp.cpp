//===- ViewOp.cpp - Implementation of the linalg ViewOp operation -------===//
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
// This file implements a simple IR operation to create a new ViewType in the
// linalg dialect.
//
//===----------------------------------------------------------------------===//

#include "linalg1/Ops.h"
#include "linalg1/Types.h"
#include "mlir/EDSC/Helpers.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/StandardTypes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Twine.h"

using llvm::ArrayRef;
using llvm::SmallVector;
using llvm::Twine;

using namespace mlir;
using namespace linalg;

void linalg::ViewOp::build(Builder *b, OperationState *result, Value *memRef,
                           ArrayRef<Value *> indexings) {
  MemRefType memRefType = memRef->getType().cast<MemRefType>();
  result->addOperands({memRef});
  assert(indexings.size() == memRefType.getRank() &&
         "unexpected number of indexings (must match the memref rank)");

  result->addOperands(indexings);
  unsigned rank = memRefType.getRank();
  for (auto *v : indexings) {
    if (!v->getType().isa<RangeType>()) {
      rank--;
    }
  }
  Type elementType = memRefType.getElementType();
  result->addTypes({linalg::ViewType::get(b->getContext(), elementType, rank)});
}

LogicalResult linalg::ViewOp::verify() {
  if (llvm::empty(getOperands()))
    return emitOpError(
        "requires at least a memref operand followed by 'rank' indices");
  auto memrefType = getOperand(0)->getType().dyn_cast<MemRefType>();
  unsigned memrefRank = memrefType.getRank();
  if (!memrefType)
    return emitOpError("first operand must be of MemRefType");
  unsigned index = 0;
  for (auto indexing : getIndexings()) {
    if (!indexing->getType().isa<RangeType>() &&
        !indexing->getType().isa<IndexType>()) {
      return emitOpError(Twine(index) +
                         "^th index must be of range or index type");
    }
    ++index;
  }
  if (llvm::size(getIndexings()) != memrefRank) {
    return emitOpError("requires at least a memref operand followed by " +
                       Twine(memrefRank) + " indices");
  }
  unsigned rank = memrefRank;
  for (auto *v : getIndexings()) {
    if (!v->getType().isa<RangeType>()) {
      rank--;
    }
  }
  if (getRank() != rank) {
    return emitOpError("the rank of the view must be the number of its range "
                       "indices: " +
                       Twine(rank));
  }
  return success();
}

// Parsing of the linalg dialect is not supported in this tutorial.
bool linalg::ViewOp::parse(OpAsmParser *parser, OperationState *result) {
  llvm_unreachable("Parsing linalg dialect is not supported in this tutorial");
}

// A ViewOp prints as:
//
// ```{.mlir}
//   linalg.view %0[%1, %2] : !linalg<"view<f32xf32>">
// ```
//
// Where %0 is an ssa-value holding a MemRef, %1 and %2 are ssa-value each
// holding a range.
void linalg::ViewOp::print(OpAsmPrinter *p) {
  *p << getOperationName() << " " << *getSupportingMemRef() << "[";
  unsigned numRanges = llvm::size(getIndexings());
  unsigned index = 0;
  for (auto indexing : getIndexings()) {
    *p << *indexing << ((index++ == numRanges - 1) ? "" : ", ");
  }
  *p << "] : " << getType();
}

Type linalg::ViewOp::getElementType() { return getViewType().getElementType(); }

ViewType linalg::ViewOp::getViewType() { return getType().cast<ViewType>(); }

unsigned linalg::ViewOp::getRank() { return getViewType().getRank(); }

// May be something else than a MemRef in the future.
Value *linalg::ViewOp::getSupportingMemRef() {
  auto *res = getOperand(0);
  assert(res->getType().isa<MemRefType>());
  return res;
}

SmallVector<mlir::Value *, 8> linalg::ViewOp::getRanges() {
  llvm::SmallVector<mlir::Value *, 8> res;
  for (auto *operand : getIndexings()) {
    if (!operand->getType().isa<mlir::IndexType>()) {
      res.push_back(operand);
    }
  }
  return res;
}

Value *linalg::ViewOp::getIndexing(unsigned rank) {
  SmallVector<Value *, 1> ranges(getIndexings().begin(), getIndexings().end());
  return ranges[rank];
}

mlir::Operation::operand_range linalg::ViewOp::getIndexings() {
  return {operand_begin() + ViewOp::FirstIndexingOperand, operand_end()};
}
