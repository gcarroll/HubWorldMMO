#include "ViewModels/URiftViewModel.h"

URiftViewModel::URiftViewModel()
{
    // Default to non-singleton so each widget gets a fresh instance unless the
    // subclass explicitly opts in by setting bIsSingleton = true in
    // InitializeViewModel_Implementation.
    bIsSingleton = false;
}

void URiftViewModel::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
    // Base implementation is intentionally empty.
    // Subclasses override this to bind to gameplay component delegates and
    // optionally set bIsSingleton = true.
}

void URiftViewModel::ShutdownViewModel_Implementation()
{
    // Base implementation is intentionally empty.
    // Subclasses override this to unbind delegates and clear cached state.
}
