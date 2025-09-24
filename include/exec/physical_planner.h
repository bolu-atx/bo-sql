#pragma once

#include <memory>
#include "logical/logical.h"
#include "catalog/catalog.h"
#include "operator.hpp"

// Direct mapping from logical to physical operators
std::unique_ptr<Operator> build_physical_plan(const LogicalOp* logical, const Catalog& catalog);