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
 *
 * Implements IGameplayTagAssetInterface so GameplayTag queries (e.g. container
 * compatibility checks) can work with the definition directly.
 */
UCLASS(BlueprintType)
class RIFTVAULTINVENTORY_API URiftItemDefinition : public UPrimaryDataAsset, public IGameplayTagAssetInterface
{
    GENERATED_BODY()

public:

    URiftItemDefinition();

    // -- Begin UPrimaryDataAsset interface
    /** Rebuilds DynamicTags and FragmentCache after the asset is loaded from disk. */
    virtual void PostLoad() override;
    /**
     * Returns the asset registry type/name used by the Asset Manager.
     * Type is "RiftItemDefinition"; name is the asset's FName.
     */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    // -- End UPrimaryDataAsset interface

    // -- Begin IGameplayTagAssetInterface
    /**
     * Populates TagContainer with all tags on this definition (designer tags + dynamic fragment tags).
     * Called by the Gameplay Tag system for tag queries against this asset.
     */
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
    // -- End IGameplayTagAssetInterface

    /**
     * Internal name used for logging and debugging. Not player-facing — use URiftFragment_Display for UI.
     * Exposed as a BlueprintPure function so Blueprint graphs can read it without touching the property.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    FORCEINLINE FName GetInternalName() const { return InternalName; }

    /**
     * Returns all gameplay tags for this item — both designer-set tags and
     * tags contributed dynamically by fragments.
     * Combines GameplayTags and DynamicTags into one container each call.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    FGameplayTagContainer GetGameplayTags() const;

    /**
     * Returns all fragment instances assigned to this definition.
     * Null entries in the internal Fragments array are skipped.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    TArray<URiftItemFragment*> GetFragments() const;

    /**
     * Returns true if this definition contains a fragment of the given class.
     * O(1) via FragmentCache exact-match lookup.
     *
     * @param FragmentClass  The fragment type to check for.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item Definition")
    bool HasFragmentOfClass(const TSubclassOf<URiftItemFragment>& FragmentClass) const;

    /**
     * Finds and returns a fragment by class. Returns null if not present.
     * Searches the cache first (exact match O(1)), then falls back to checking
     * subclasses for polymorphic queries (O(n) over cached entries).
     *
     * Use the StaticClass form to avoid MSVC C2275/C2059 template parse errors:
     *   Cast<UMyFragment>(Definition->FindFragmentByClass(UMyFragment::StaticClass()))
     *
     * @param FragmentClass  The fragment class to search for.
     * @return  The fragment instance, or null if absent.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Item Definition", meta = (DeterminesOutputType = FragmentClass))
    URiftItemFragment* FindFragmentByClass(TSubclassOf<URiftItemFragment> FragmentClass) const;

    /**
     * Finds all fragments that implement the given interface.
     * Iterates all fragments and checks ImplementsInterface — O(n).
     *
     * @param FragmentInterface  The interface class to search for.
     * @return  All matching fragment instances.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Item Definition")
    TArray<URiftItemFragment*> FindFragmentsByInterface(TSubclassOf<UInterface> FragmentInterface) const;

    /**
     * Typed C++ template wrapper around FindFragmentByClass.
     * Eliminates the need for a manual Cast at the call site.
     *
     * Usage:
     *   URiftFragment_Stack* Frag = Definition->FindFragment<URiftFragment_Stack>();
     */
    template<typename T>
    T* FindFragment() const
    {
        return Cast<T>(FindFragmentByClass(T::StaticClass()));
    }

    /**
     * Adds a fragment to this definition and refreshes caches.
     * Advanced — intended for editor tools and test automation only.
     * Guards against duplicate additions.
     *
     * @param Fragment  The fragment instance to add.
     */
    void AddFragment(URiftItemFragment* Fragment);

    /**
     * Appends tags to this definition's static tag container.
     * Advanced — intended for editor tools and test automation only.
     * No-ops if NewTags is invalid.
     *
     * @param NewTags  The tags to append to GameplayTags.
     */
    void AppendGameplayTags(const FGameplayTagContainer& NewTags);

#if WITH_EDITOR
    // -- Begin UObject editor interface
    /**
     * Refreshes DynamicTags and FragmentCache whenever the Fragments array is edited.
     * Ensures the cache is always consistent with the Fragments array visible in the editor.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    /**
     * Validates the asset for common authoring mistakes:
     *   - Missing InternalName (warning)
     *   - Empty GameplayTags (warning — item may be rejected by all containers)
     *   - Null fragment entries (error)
     */
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

    /**
     * Static gameplay tags assigned by the designer that identify this item type.
     * Tag_Rift_Item_Trait is added in the constructor so all items are accepted by
     * containers with a default compatibility query.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Definition")
    FGameplayTagContainer GameplayTags;

    /**
     * Tags contributed dynamically by fragments. Rebuilt on PostLoad and on editor
     * fragment changes via RefreshDynamicTags. Not directly edited by designers —
     * VisibleDefaultsOnly makes it read-only in the Details panel.
     */
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Item Definition")
    FGameplayTagContainer DynamicTags;

    /**
     * The fragments that define this item's capabilities.
     * Add fragments here to compose the item's behaviour.
     * NoElementDuplicate meta prevents the same fragment class appearing twice.
     * Instanced so fragment properties are serialized inline with this asset.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Definition", Instanced, meta = (NoElementDuplicate))
    TArray<TObjectPtr<URiftItemFragment>> Fragments;

    /**
     * Rebuilds the DynamicTags container by calling AppendFragmentTags(nullptr, ...) on each fragment.
     * Passing nullptr for the item indicates this is a definition-level (type-level) tag collection,
     * not an instance-level query. Called on PostLoad and when fragments change in the editor.
     */
    virtual void RefreshDynamicTags();

    /**
     * Rebuilds the FragmentCache map by re-keying every fragment by its class.
     * Called on PostLoad and when fragments change in the editor.
     * Guarantees O(1) lookup via FindFragmentByClass at runtime.
     */
    virtual void RefreshFragmentCache();

private:

    /**
     * Class-keyed cache of fragment instances for O(1) lookup.
     * Rebuilt on PostLoad and PostEditChangeProperty. Not saved separately —
     * always derived from the Fragments array.
     */
    UPROPERTY()
    TMap<TSubclassOf<URiftItemFragment>, TObjectPtr<URiftItemFragment>> FragmentCache;
};
