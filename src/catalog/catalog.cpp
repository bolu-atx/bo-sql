#include "catalog/catalog.h"

namespace bosql {

void Catalog::register_table(Table table, TableMeta&& table_meta) {
    std::string name = table_meta.name;  // copy name before moving
    tables_[name] = {std::move(table), std::move(table_meta)};
}

OptionalRef<const Table> Catalog::get_table_data(const std::string& name) const {
    auto it = tables_.find(name);
    return it != tables_.end() ? OptionalRef<const Table>(it->second.first) : OptionalRef<const Table>();
}

OptionalRef<const TableMeta> Catalog::get_table_meta(const std::string& name) const {
    auto it = tables_.find(name);
    return it != tables_.end() ? OptionalRef<const TableMeta>(it->second.second) : OptionalRef<const TableMeta>();
}

std::vector<std::string> Catalog::list_tables() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (const auto& pair : tables_) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace bosql