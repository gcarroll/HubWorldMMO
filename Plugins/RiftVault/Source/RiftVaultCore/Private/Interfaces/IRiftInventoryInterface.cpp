#include "Interfaces/IRiftInventoryInterface.h"

/**
 * Default implementation of GetInventoryComponent.
 *
 * Returns nullptr to indicate "this actor does not have an inventory component."
 * Concrete actor classes that own a URiftInventoryComponent override this to
 * return a pointer to their component subobject.
 *
 * UHT requires that BlueprintNativeEvent functions have a C++ _Implementation
 * body even when the default behavior is trivial — hence this file.
 */
UActorComponent* IRiftInventoryInterface::GetInventoryComponent_Implementation() const
{
    // Base implementation: no inventory component on this actor.
    // Subclasses override this to return their URiftInventoryComponent*.
    return nullptr;
}
