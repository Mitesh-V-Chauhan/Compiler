#include "nova/backend/memory_layout.h"

namespace nova {

int MemoryLayout::getSize(std::shared_ptr<Type> type) {
    if (!type) return 0;
    switch (type->kind) {
        case TypeKind::Int: return 8; // Assuming 64-bit integers for simplicity on x86-64
        case TypeKind::Float: return 8; // Double precision
        case TypeKind::Bool: return 1;
        case TypeKind::Char: return 1;
        case TypeKind::String: return 8; // Pointer to string
        case TypeKind::Function: return 8; // Function pointer
        case TypeKind::Array:
            // For this basic layout, assume array size is unknown or fixed later.
            // Wait, we don't have array sizes in Type! For now, let's treat it as a pointer (8 bytes).
            // Proper arrays need sizes.
            return 8;
        case TypeKind::Struct:
            // We would need to look up the struct definition to get its size.
            // For now, this is problematic if we don't pass the struct info.
            return 8; // Fallback for pointers/references to structs
        default: return 0;
    }
}

int MemoryLayout::getAlignment(std::shared_ptr<Type> type) {
    if (!type) return 1;
    switch (type->kind) {
        case TypeKind::Int: return 8;
        case TypeKind::Float: return 8;
        case TypeKind::Bool: return 1;
        case TypeKind::Char: return 1;
        case TypeKind::String: return 8;
        case TypeKind::Array: return 8;
        case TypeKind::Struct: return 8;
        default: return 1;
    }
}

StructLayout MemoryLayout::calculateStructLayout(const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& fields) {
    StructLayout layout;
    int current_offset = 0;
    int max_alignment = 1;
    
    for (const auto& field : fields) {
        int align = getAlignment(field.second);
        int size = getSize(field.second);
        
        // Pad to alignment
        if (current_offset % align != 0) {
            current_offset += align - (current_offset % align);
        }
        
        layout.field_offsets[field.first] = current_offset;
        current_offset += size;
        
        if (align > max_alignment) {
            max_alignment = align;
        }
    }
    
    // Final padding for the struct itself
    if (current_offset % max_alignment != 0) {
        current_offset += max_alignment - (current_offset % max_alignment);
    }
    
    layout.size = current_offset;
    layout.alignment = max_alignment;
    
    return layout;
}

} // namespace nova
