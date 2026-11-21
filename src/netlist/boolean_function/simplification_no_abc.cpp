#include "hal_core/netlist/boolean_function/simplification.h"

namespace hal
{
    // Used when HAL is built without Berkeley ABC (HAL_ENABLE_ABC=OFF): no global optimization, input is returned as is.
    Result<BooleanFunction> Simplification::abc_simplification(const BooleanFunction& function)
    {
        return OK(function.clone());
    }
}    // namespace hal
