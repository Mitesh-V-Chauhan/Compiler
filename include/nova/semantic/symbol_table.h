#pragma once
#include "nova/semantic/type.h"
#include <string_view>
#include <unordered_map>
#include <memory>

namespace nova {

struct Symbol {
    std::string_view name;
    std::shared_ptr<Type> type;
    bool is_mutable = false;
};

class Scope {
public:
    Scope* parent;
    std::unordered_map<std::string_view, Symbol> symbols;

    explicit Scope(Scope* p = nullptr) : parent(p) {}

    bool define(const Symbol& sym) {
        if (symbols.find(sym.name) != symbols.end()) return false;
        symbols[sym.name] = sym;
        return true;
    }

    Symbol* lookup(std::string_view name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) return &it->second;
        if (parent) return parent->lookup(name);
        return nullptr;
    }
};

} // namespace nova
