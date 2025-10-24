#include "Widgets/Grid/ContainerGrid.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "CommonTileView.h"

void UContainerGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Hinweis: Entry-Widgets werden über den UMG-Designer gesetzt.
	// Diese C++-Klasse kümmert sich primär um Bindung und Metadaten (Rows/Cols).
}

void UContainerGrid::BindToContainer(URpg_ContainerComponent* InComponent, int32 InContainerIndex)
{
	ContainerComponent = InComponent;
	ContainerIndex = InContainerIndex;
	CacheFromDefinition();

	// Hinweis: Das tatsächliche Füllen des TileViews wird absichtlich den Blueprints überlassen,
	// damit Layout/Styling frei erfolgen kann. Diese Klasse stellt nur die Metadaten bereit.
}

void UContainerGrid::CacheFromDefinition()
{
	CachedRows = 0;
	CachedCols = 0;

	if (!ContainerComponent.IsValid())
	{
		return;
	}

	const URpg_ContainerComponent* Comp = ContainerComponent.Get();
	if (!Comp)
	{
		return;
	}

	if (!Comp->Containers.IsValidIndex(ContainerIndex))
	{
		return;
	}

	const FInvContainer& C = Comp->Containers[ContainerIndex];
	CachedRows = FMath::Max(1, C.Rows);
	CachedCols = FMath::Max(1, C.Cols);
}
