// Copyright Grumpy Giraffe Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "MOS_Types.generated.h"

/***********/
/*  ENUMS  */
/***********/

UENUM(BlueprintType)
enum class EOSSInterfaceVoiceChatConnectionStatus : uint8
{
	/** Not connected to voice chat. */
	NotConnected,

	/** Connecting to voice chat. */
	Connecting,

	/** Connected to voice chat. */
	Connected,
};

UENUM(BlueprintType)
enum class EOSSFriendsFriendInvitationStatus : uint8
{
	Unknown,
	Accepted,
	PendingInbound,
	PendingOutbound,
	Blocked,
	Suggested,
};

UENUM(BlueprintType)
enum class EOSSFriendsFriendPresenceStatus : uint8
{
	Online,
	Offline,
	Away,
	ExtendedAway,
	DoNotDisturb,
	Chat,
};

/***********/
/* STRUCTS */
/***********/

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSSessionsSearchResult
{
	GENERATED_BODY()

	/** All advertised session information */
	FOnlineSession Session;
	/** Ping to the search result, MAX_QUERY_PING is unreachable */
	int32 PingInMs;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSLeaderboardsLeaderboardEntry
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	double CurrentValue = 0.0;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSAchievementsAchievementState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	bool bUnlocked = false;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSEcommerceOffer
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText Title;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText CurrentPrice;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSEcommerceEntitlement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText Title;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Status;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSStatsStatState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	double CurrentValue = 0.0;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSEcommerceReceipt
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FString Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText Title;
};

struct MULTIPLAYERONLINESUBSYSTEM_API FUIListEntry
{
	FString Id;
	FText DisplayName;

	FUIListEntry(const FString &InId, const FText &InDisplayName);
	FUIListEntry(const FUIListEntry &) = delete;
	FUIListEntry(FUIListEntry &&) = delete;
};

typedef TSharedPtr<FUIListEntry> FUIListEntryPtr;

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSInterfaceListEntry
{
	GENERATED_BODY()
	
	/** The unique identifier for this list entry. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "List Entry"))
	FString Id;

	/** The display name for this list entry, as it will be displayed on screen. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "List Entry"))
	FText DisplayName;

	static void SynchroniseToComboBoxList(
		const TArray<FOSSInterfaceListEntry> &InOptions,
		TArray<FUIListEntryPtr> &InOutComboBoxList);
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSFriendsFriendState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FUniqueNetIdRepl Id;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString DisplayName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString RealName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    EOSSFriendsFriendInvitationStatus InvitationStatus = EOSSFriendsFriendInvitationStatus::Unknown;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FUniqueNetIdRepl PresenceSessionId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString PresencePartyId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsOnline = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsPlaying = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsPlayingThisGame = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsJoinable = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceHasVoiceSupport = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FDateTime PresenceLastOnline;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString PresenceStatusString;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    EOSSFriendsFriendPresenceStatus PresenceStatusState = EOSSFriendsFriendPresenceStatus::Offline;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    TMap<FString, FString> PresenceStatusProperties;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    TMap<FString, FString> Attributes;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSFriendsRecentPlayerState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
	FUniqueNetIdRepl Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
	FString DisplayName;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERONLINESUBSYSTEM_API FOSSFriendsBlockedPlayerState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
	FUniqueNetIdRepl Id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
	FString DisplayName;
};