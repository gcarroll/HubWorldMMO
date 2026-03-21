#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "URiftBaseWidget.generated.h"

/**
 * Base class for all RiftVault UI widgets.
 * Groups widgets under the "RiftVault" category in the UMG palette.
 */
UCLASS(Abstract)
class RIFTVAULTUI_API URiftBaseWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    URiftBaseWidget(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
    virtual const FText GetPaletteCategory() override
    {
        return FText::FromString(TEXT("RiftVault"));
    }
#endif
};
