#include "Menu/LevelCatalog.h"

#include "Engine/World.h"

const FLevelCatalogEntry* ULevelCatalog::FindEntry(FName Id) const
{
	if (Id.IsNone())
	{
		return nullptr;
	}
	return Entries.FindByPredicate([Id](const FLevelCatalogEntry& Entry) { return Entry.Id == Id; });
}

const FLevelCatalogEntry* ULevelCatalog::FindEntryForWorld(const UWorld* World) const
{
	if (!World || !World->GetOutermost())
	{
		return nullptr;
	}
	const FString WorldPackage = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	return Entries.FindByPredicate([&WorldPackage](const FLevelCatalogEntry& Entry)
	{
		return !Entry.Map.IsNull() && Entry.Map.ToSoftObjectPath().GetLongPackageName() == WorldPackage;
	});
}

const FLevelCatalogEntry* ULevelCatalog::FindTutorialEntry() const
{
	const FLevelCatalogEntry* Best = nullptr;
	for (const FLevelCatalogEntry& Entry : Entries)
	{
		if (Entry.bIsTutorial && !Entry.Map.IsNull() && (!Best || Entry.SortOrder < Best->SortOrder))
		{
			Best = &Entry;
		}
	}
	return Best;
}

TArray<FLevelCatalogEntry> ULevelCatalog::GetSortedEntries() const
{
	TArray<FLevelCatalogEntry> Sorted = Entries;
	Sorted.StableSort([](const FLevelCatalogEntry& A, const FLevelCatalogEntry& B) { return A.SortOrder < B.SortOrder; });
	return Sorted;
}
