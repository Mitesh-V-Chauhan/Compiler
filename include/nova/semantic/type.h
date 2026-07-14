#pragma once
#include <string>
#include <memory>
#include <vector>

namespace nova {

enum class TypeKind {
    Unknown, Void, Int, Float, Bool, String, Char, Array, Struct, Function
};

class Type {
public:
    TypeKind kind;
    std::string name;
    
    std::shared_ptr<Type> element_type = nullptr; // For Array
    
    std::shared_ptr<Type> return_type = nullptr;  // For Function
    std::vector<std::shared_ptr<Type>> param_types;
    
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> struct_fields; // For Struct
    
    int pointer_depth = 0;

    Type(TypeKind k, std::string n = "") : kind(k), name(std::move(n)) {}
    
    bool operator==(const Type& other) const {
        if (kind != other.kind || pointer_depth != other.pointer_depth) return false;
        if (kind == TypeKind::Array) return *element_type == *other.element_type;
        if (kind == TypeKind::Struct) return name == other.name;
        if (kind == TypeKind::Function) {
            if (param_types.size() != other.param_types.size()) return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (!(*param_types[i] == *other.param_types[i])) return false;
            }
            return *return_type == *other.return_type;
        }
        return true;
    }
    bool operator!=(const Type& other) const { return !(*this == other); }
    
    static std::shared_ptr<Type> getVoid() { static auto t = std::make_shared<Type>(TypeKind::Void, "void"); return t; }
    static std::shared_ptr<Type> getInt() { static auto t = std::make_shared<Type>(TypeKind::Int, "int"); return t; }
    static std::shared_ptr<Type> getFloat() { static auto t = std::make_shared<Type>(TypeKind::Float, "float"); return t; }
    static std::shared_ptr<Type> getBool() { static auto t = std::make_shared<Type>(TypeKind::Bool, "bool"); return t; }
    static std::shared_ptr<Type> getString() { static auto t = std::make_shared<Type>(TypeKind::String, "string"); return t; }
    static std::shared_ptr<Type> getChar() { static auto t = std::make_shared<Type>(TypeKind::Char, "char"); return t; }
    static std::shared_ptr<Type> getUnknown() { static auto t = std::make_shared<Type>(TypeKind::Unknown, "unknown"); return t; }
};

} // namespace nova
