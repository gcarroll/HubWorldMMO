#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftItemDefinition.generated.h"

/**
 * Defines an item type in the RiftVault inventory system.
 *
 * URiftItemDefinition is a designer-authored PrimaryDataAsset that describes
 * what an item *type* is. It is never instantiated directly at runtime —
 * URiftItemInstance is created to represent one item in an inventory, holding
 * a reference back to its definition.
 *
 * Items are defined by composition. A sword might have:
 *   URiftFragment_Equippable, URiftFragment_Display, URiftFragment_Value,
 *   URiftFragment_Condition, URiftFragment_DynamicAttributes
 *
 * A potion might only have:
 *   URiftFragment_Stack, URiftFragment_Display
 *
 * No subclassing of URiftItemDefinition is required or encouraged.
 * New item behaviours are new fragments.
 *
 * The fragment cache (FragmentCache) is rebuilt on PostLoad and on editor
 * property changes for O(1) fragment lookup at runtime.
 */
UCLASS(BlueprintType)
class RIFTVAULTINVENTORY_API URiftItemDefinition : public UPrimaryDataAsset, public IGameplayTagAssetInterface
{
    GENERATED_BODY()

public:

    URiftItemDefinition();

    // -- Begin UPrimaryDataAsset interface
    virtual void PostLoad() override;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    // -- End UPrimaryDataAsset interface

    // -- Begin IGameplayTagAssetInterface
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
    // -- End IGameplayTagAssetInterface

    /** Internal name used for logging and debugging. Not player-facing — use URiftFragment_Display for UI. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    FORCEINLINE FName GetInternalName() const { return InternalName; }

    /**
     * Returns all gameplay tags for this item — both designer-set tags and
     * tags contributed dynamically by fragments.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    FGameplayTagContainer GetGameplayTags() const;

    /** Returns all fragment instances assigned to this definition. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    TArray<URiftItemFragment*> GetFragments() const;

    /** Returns true if this definition contains a fragment of the given class. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    bool HasFragmentOfClass(const TSubclassOf<URiftItemFragment>& FragmentClass) const;

    /**
     * Finds and returns a fragment by class. Returns null if not present.
     * Searches the cache first (exact match), then falls back to checking
     * subclasses for polymorphic queries.
     *
     * Use the StaticClass form to avoid MSVC C2275/C2059 template parse errors:
     *   Cast<UMyFragment>(Definition->FindFragmentByClass(UMyFragment::StaticClass()))
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Item Definition", meta = (DeterminesOutputType = FragmentClass))
    URiftItemFragment* FindFragmentByClass(TSubclassOf<URiftItemFragment> FragmentClass) const;

    /** Finds all fragments that implement the given interface. */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Item Definition")
    TArray<URiftItemFragment*> FindFragmentsByInterface(TSubclassOf<UInterface> FragmentInterface) const;

    /** Typed C++ template wrapper around FindFragmentByClass. */
    template<typename T>
    T* FindFragment() const
    {
        return Cast<T>(FindFragmentByClass(T::StaticClass()));
    }

    /**
     * Adds a fragment to this definition and refreshes caches.
     * Advanced — intended for editor tools and test automation only.
     */
    void AddFragment(URiftItemFragment* Fragment);

    /**
     * Appends tags to this definition's tag container.
     * Advanced — intended for editor tools and test automation only.
     */
    void AppendGameplayTags(const FGameplayTagContainer& NewTags);

#if WITH_EDITOR
    // -- Begin UObject editor interface
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    // -- End UObject editor interface
#endif

protected:

    /**
     * Internal name for this item type. Used in logs and debug tools.
     * For player-facing names use URiftFragment_Display.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Definition")
    FName InternalName;

    /** Designer-set gameplay tags that identify this item type. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Definition")
    FGameplayTagContainer GameplayTags;

    /**
     * Tags contributed dynamically by fragments. Rebuilt on PostLoad and on
     * editor fragment changes. Not directly edited by designers.
     */
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Definition")
    FGameplayTagContainer DynamicTags;

    /**
     * The fragments that define this item's capabilities.
     * Add fragments here to compose the item's behaviour.
     * No duplicate fragment classes are allowed.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Definition", Instanced, meta = (NoElementDuplicate))
    TArray<TObjectPtr<URiftItemFragment>> Fragments;

    /**
     * Rebuilds the DynamicTags container by collecting tags from all fragments.
     * Called on PostLoad and when fragments are changed in the editor.
     */
    virtual void RefreshDynamicTags();

    /**
     * Rebuilds the FragmentCache map for O(1) runtime lookup.
     * Called on PostLoad and when fragments are changed in the editor.
     */
    virtual void RefreshFragmentCache();

private:

    /** Class-keyed cache of fragment instances for O(1) lookup. Rebuilt on load/edit. */
    UPROPERTY()
    TMap<TSubclassOf<URiftItemFragment>, TObjectPtr<URiftItemFragment>> FragmentCache;
};
