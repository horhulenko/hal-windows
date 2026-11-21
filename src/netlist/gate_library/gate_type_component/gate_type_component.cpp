#include "hal_core/netlist/gate_library/gate_type_component/gate_type_component.h"

namespace hal
{
    

    GateTypeComponent* GateTypeComponent::get_component(const std::function<bool(const GateTypeComponent*)>& filter) const
    {
        std::vector<GateTypeComponent*> components = this->get_components(filter);

        if (components.size() == 1)
        {
            return *components.begin();
        }

        return nullptr;
    }
}    // namespace hal