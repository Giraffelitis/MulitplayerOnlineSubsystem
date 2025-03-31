// Copyright Grumpy Giraffe Games. All Rights Reserved.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_Types.h"

FUIListEntry::FUIListEntry(const FString &InId, const FText &InDisplayName)
	: Id(InId)
	, DisplayName(InDisplayName)
{
}

void FOSSInterfaceListEntry::SynchroniseToComboBoxList(
	const TArray<FOSSInterfaceListEntry> &InOptions,
	TArray<FUIListEntryPtr> &InOutComboBoxList)
{
	// Remove any combo list entries that aren't valid (somehow).
	for (int i = InOutComboBoxList.Num() - 1; i >= 0; i--)
	{
		if (!InOutComboBoxList[i].IsValid())
		{
			InOutComboBoxList.RemoveAt(i);
		}
	}

	// Convert arrays to maps so we can do faster lookups.
	TMap<FString, FText> TargetOptions;
	TMap<FString, FUIListEntryPtr> CurrentOptions;
	for (const auto &Option : InOptions)
	{
		TargetOptions.Add(Option.Id, Option.DisplayName);
	}
	for (const auto &ComboListEntry : InOutComboBoxList)
	{
		CurrentOptions.Add(ComboListEntry->Id, ComboListEntry);
	}

	// Check for additions or modifications.
	for (const auto &Option : TargetOptions)
	{
		if (!CurrentOptions.Contains(Option.Key))
		{
			// Add this item.
			InOutComboBoxList.Add(MakeShared<FUIListEntry>(Option.Key, Option.Value));
		}
		else if (!CurrentOptions[Option.Key]->DisplayName.EqualTo(Option.Value))
		{
			// Update this item.
			CurrentOptions[Option.Key]->DisplayName = Option.Value;
		}
	}

	// Check for removals.
	for (const auto &Option : CurrentOptions)
	{
		if (!TargetOptions.Contains(Option.Key))
		{
			InOutComboBoxList.Remove(Option.Value);
		}
	}

	// Sort the list by the key.
	Algo::Sort(InOutComboBoxList, [](const FUIListEntryPtr &A, const FUIListEntryPtr &B) -> bool {
		FString IdA = A.IsValid() ? A->Id : TEXT("");
		FString IdB = B.IsValid() ? B->Id : TEXT("");
		return IdA < IdB;
	});
}