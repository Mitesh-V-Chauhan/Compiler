#pragma once
#include "nova/semantic/type.h"
#include <unordered_map>
#include <vector>

namespace nova {

struct StructLayout {
    int size = 0;
    int alignment = 1;
    std::unordered_map<std::string, int> field_offsets;
};

class MemoryLayout {
public:
    static int getSize(std::shared_ptr<Type> type);
    static int getAlignment(std::shared_ptr<Type> type);
    static StructLayout calculateStructLayout(const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& fields);
};

} // namespace nova
