#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <utility>
#include <set>
#include "types.h"
#include "storage/table.h"
#include "catalog/catalog.h"

namespace bosql {

std::pair<Table, TableMeta> load_csv(const std::string& filename);

} // namespace bosql