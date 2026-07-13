#pragma once
#include "nova/ir/ir.h"

namespace nova {

class Optimizer {
public:
    void optimize(IRProgram& program);

private:
    void optimizeFunction(IRFunction& func);
    bool constantFolding(IRFunction& func);
    bool constantPropagation(IRFunction& func);
    bool deadCodeElimination(IRFunction& func);
    bool controlFlowSimplification(IRFunction& func);
};

} // namespace nova
