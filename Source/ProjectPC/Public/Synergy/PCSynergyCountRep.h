// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "PCSynergyCountRep.generated.h"
/**
 * 
 */

USTRUCT()
struct FSynergyCountEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;
	UPROPERTY()
	int32 Count = 0;
};

USTRUCT()
struct FSynergyCountArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSynergyCountEntry> Entries;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize(Entries, DeltaParams, *this);
	}

	void SetCount(const FGameplayTag& Tag, int32 NewCount)
	{
		if (!Tag.IsValid())
			return;

		if (FSynergyCountEntry* Found = Entries.FindByPredicate(
			[&](const FSynergyCountEntry& Entry){ return Entry.Tag == Tag; }))
		{
			if (Found->Count != NewCount)
			{
				Found->Count = NewCount;
				MarkItemDirty(*Found);
			}
			return;
		}

		FSynergyCountEntry& Added = Entries.AddDefaulted_GetRef();
		Added.Tag = Tag;
		Added.Count = NewCount;
		MarkItemDirty(Added);
	}

	void RemoveByTag(const FGameplayTag& Tag)
	{
		const int32 Idx = Entries.IndexOfByPredicate(
			[&](const FSynergyCountEntry& Entry){ return Entry.Tag == Tag; });
		if (Idx != INDEX_NONE)
		{
			Entries.RemoveAtSwap(Idx);
			MarkArrayDirty();
		}
	}

	void RemoveByTags(const FGameplayTagContainer& Tags)
	{
		if (!Tags.IsEmpty())
		{
			for (const auto& Tag : Tags)
			{
				const int32 Idx = Entries.IndexOfByPredicate(
					[&](const FSynergyCountEntry& Entry){ return Entry.Tag == Tag; });
				if (Idx != INDEX_NONE)
				{
					Entries.RemoveAtSwap(Idx);
				}
			}

			MarkArrayDirty();
		}
	}

	void UpdateToMap(const TMap<FGameplayTag, int32>& Map)
	{
		// Map을 순회하여 Synergy Count 갱신
		for (const auto& KV : Map)
		{
			SetCount(KV.Key, KV.Value);
		}

		// Entries 배열을 순회하여 Map에 존재하지 않는 시너지 태그를 추출
		FGameplayTagContainer RemoveTags;
		for (const auto& Entry : Entries)
		{
			const FGameplayTag& SynergyTag = Entry.Tag;
			if (!Map.Contains(SynergyTag))
			{
				RemoveTags.AddTag(SynergyTag);
			}
		}
		if (RemoveTags.IsEmpty())
		{
			RemoveByTags(RemoveTags);
		}
	}
};

template<> struct TStructOpsTypeTraits<FSynergyCountArray> : public TStructOpsTypeTraitsBase2<FSynergyCountArray>
{
	enum { WithNetDeltaSerializer = true };
};