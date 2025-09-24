#include "catalog/catalog.h"

void Catalog::register_table(TableMeta table_meta) {
    tables_[table_meta.name] = std::move(table_meta);
}

const TableMeta* Catalog::get_table(const std::string& name) const {
    auto it = tables_.find(name);
    return it != tables_.end() ? &it->second : nullptr;
}

std::vector<std::string> Catalog::list_tables() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (std::unordered_map<std::string, TableMeta>::const_iterator it = tables_.begin(); it != tables_.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}