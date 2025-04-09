//  Copyright Grumpy Giraffe Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include "VoiceChat.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_AsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_CallScopedOjectPtr.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_GetAvatarAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_ListAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_QueryAchievementsAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_QueryLeaderboardsAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_QueryStatsAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_ReadFileSaveGameAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_ReadFileStringAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_TextAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_Types.h"

#include "MOS_GameInstanceSubsystem.generated.h"

class UMOS_SessionsFindSessionsAsyncResult;
class FOnlineSessionSearchResult;
class IOnlineSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(MOSGameInstanceSubsystem, Log, All);

DECLARE_DYNAMIC_DELEGATE(FMOSFindSessionsCompleteDelegate);

UCLASS(Blueprintable)
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_GameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

    // @note: We use this to track the last session ID that we followed a party into, so that when we get subsequent
    // "party data updated" events we don't join the session again.
    FString CachedLastPartySessionId;

    // @note: This is the voice chat user that has currently been created for interacting with voice chat.
    IVoiceChatUser *CachedVoiceChatUser;
	
	bool bDoesAutoLogin = true;
	bool bIsAttemptingLogin = false;
	TArray<FOnlineSessionSearchResult> SessionResults;
	
public:

	FMOSFindSessionsCompleteDelegate MOSFindSessionsCompleteDelegate;

	UPROPERTY(BlueprintReadOnly, Category = "MOS")
	TArray<FMOSSessionsSearchResult> CachedFindSessionResults;
	UPROPERTY(BlueprintReadOnly, Category = "MOS")
	UMOS_AsyncResult* AsyncResult;
	UPROPERTY(BlueprintReadOnly, Category = "MOS")
	UMOS_SessionsFindSessionsAsyncResult* FindSessionsAsyncResult;
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	int PingInMs = 999;
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	FString SessionInfo = "";
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	FString SessionUniqueNetID = "";
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	FString SessionOwnerName = "";
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	int OpenPublicConnections = 999;
	UPROPERTY(BlueprintReadOnly, Category = "MOS|Sessions")
	int OpenPrivateConnections = 999;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (DisplayName = "LocalUserNum", Category = "MOS"))
	int32 LocalUserNum = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOS|Sessions")
	int32 SessionAvailableSlots = 2;
	/*This must match the session name in the game mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOS|Sessions")
	FName MOSSessionName = "MyLocalSessionName";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOS")
	TSubclassOf<AGameModeBase> TravelGameMode = TSubclassOf<AGameModeBase>(AGameModeBase::StaticClass());
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOS")
	FName ListenLevel = "Game/Maps/TransitionMap";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOS")
	FName TravelLevel = "Game/Maps/ExampleMap";
	UFUNCTION(BlueprintCallable, Category = "MOS")
	void SetTravelParameters(const FName InListenLevel, const FName InTravelLevel, const TSubclassOf<AGameModeBase> InGameMode);
	
	
    // @note: This is overridden so we can delete the voice chat user when this instance is destroyed.
    ~UMOS_GameInstanceSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void RegisterEvents();

	UFUNCTION(BlueprintCallable)
	bool GetIsAttemptingLogin() {return bIsAttemptingLogin;}
	UFUNCTION(BlueprintCallable)
	bool GetDoesAutoLogin() {return bDoesAutoLogin;}
	
    /* AUTH */
	
    FString GetCurrentUserDisplayName() const;
    FUniqueNetIdRepl GetCurrentUserId() const;
    FString GetCurrentUserSecondaryId() const;
    bool ShouldRenderUserSecondaryIdField() const;
    FString GetCurrentUserAuthAttribute(const FString &Key) const;
    FUniqueNetIdRepl GetIdentityUniqueNetId(const FString &UniqueNetId);
	
    UFUNCTION(BlueprintCallable)
    bool GetAuthIsLoggedIn() const;

	UFUNCTION(BlueprintCallable)
	void ExecuteAuthAutoLogin(UMOS_AsyncResult *Result);
	void OnLoginCompleted(int InUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	
    UFUNCTION(BlueprintCallable)
    void ExecuteAuthLogout(UMOS_AsyncResult *Result);
	void OnLogoutCompleted(int InUserNum, bool bWasSuccessful);
	
    bool GetAuthCanLinkCrossPlatformAccount() const;
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetAuthCanLinkCrossPlatformAccount", Category = "Auth"))
	void GetSessionInfo(const FMOSSessionsSearchResult &SearchResult);
	
    /* SESSIONS */
	
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsFindSessions(UMOS_SessionsFindSessionsAsyncResult *Result);
	void OnFindSessionsComplete(bool bWasSuccessful);
	
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsStartListenServer(int32 AvailableSlots);
	void OnMapListening(const UWorld::FActorsInitializedParams& ActorInitParams);
	void StartListenServer(int32 AvailableSlots);
	UFUNCTION(BlueprintCallable)
    void ExecuteSessionsCreateSession(FName SessionName, UMOS_AsyncResult *Result);
	void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsJoinSession(const FMOSSessionsSearchResult &SearchResult, FName SessionName, UMOS_AsyncResult *Result);
	void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
	
    void ExecuteSessionsJoinSessionWithParty(const FMOSSessionsSearchResult &SearchResult, FName SessionName, UMOS_AsyncResult *Result);
    FString GetSessionId(FName SessionName) const;
    TArray<FUniqueNetIdRepl> GetSessionRegisteredPlayerIds(FName SessionName) const;
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsStartSession(FName SessionName, UMOS_AsyncResult *Result);
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsEndSession(FName SessionName, UMOS_AsyncResult *Result);
    void ExecuteSessionsOpenInviteUI(FName SessionName);
    UFUNCTION(BlueprintCallable)
    void ExecuteSessionsDestroySession(FName SessionName, UMOS_AsyncResult *Result);
	UFUNCTION(BlueprintCallable)
    void ExecuteSessionsRegisterPlayer(FName SessionName, const FUniqueNetIdRepl &PlayerId, bool bWasFromInvite, UMOS_AsyncResult *Result);
	void OnRegisterPlayersComplete(FName SessionName, const TArray<FUniqueNetIdRef> &PlayerIds, bool bWasSuccessful);
    void ExecuteSessionsUnregisterPlayer(FName SessionName, const FUniqueNetIdRepl &PlayerId, UMOS_AsyncResult *Result);
    void ExecuteSessionsReturnToMainMenu();
	
    /* ECOMMERCE */
	
    void ExecuteEcommerceQueryOffers(UMOS_AsyncResult *Result);
    TArray<FMOSEcommerceOffer> GetEcommerceCachedOffers() const;
    void ExecuteEcommerceStartPurchase(UMOS_AsyncResult *Result, const TMap<FString, int32> &OfferIdsToQuantities);
    void ExecuteEcommerceQueryEntitlements(UMOS_AsyncResult *Result);
    TArray<FMOSEcommerceEntitlement> GetEcommerceCachedEntitlements() const;
    void ExecuteEcommerceQueryReceipts(UMOS_AsyncResult *Result);
    TArray<FMOSEcommerceReceipt> GetEcommerceCachedReceipts() const;
	
    /* FRIENDS */
	
    UFUNCTION(BlueprintCallable)
    void ExecuteFriendsQueryFriends(UMOS_AsyncResult *Result);
	TMulticastDelegate<void()> FriendsOnFriendsChange;
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RaiseFriendsOnFriendsChange", Category = "Friends"))
	void RaiseFriendsOnFriendsChange();
    UFUNCTION(BlueprintCallable)
    TArray<FUniqueNetIdRepl> GetFriendsCurrentFriends() const;
    FMOSFriendsFriendState GetFriendsFriendState(const FUniqueNetIdRepl &UserId) const;
    void ExecuteFriendsSetFriendAlias(const FUniqueNetIdRepl &UserId, const FString &Alias, UMOS_AsyncResult *Result);
    void ExecuteFriendsDeleteFriendAlias(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsSendInvite(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsAcceptInvite(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsRejectInvite(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsDeleteFriend(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsQueryRecentPlayers(UMOS_AsyncResult *Result);
    TArray<FMOSFriendsRecentPlayerState> GetFriendsCurrentRecentPlayers() const;
    void ExecuteFriendsBlockPlayer(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsUnblockPlayer(const FUniqueNetIdRepl &UserId, UMOS_AsyncResult *Result);
    void ExecuteFriendsQueryBlockedPlayers(UMOS_AsyncResult *Result);
    TArray<FMOSFriendsBlockedPlayerState> GetFriendsCurrentBlockedPlayers() const;
    void ExecuteFriendsSendInviteByFriendCode(const FString &FriendCode, UMOS_AsyncResult *Result);
    FString GetFriendsCurrentFriendCode() const;
	
	/* AVATAR */
	
    void ExecuteAvatarGetAvatar(const FUniqueNetIdRepl &UserId, UMOS_GetAvatarAsyncResult *Result);
	
    /* VOICE CHAT */
	
    bool GetVoiceChatIsLoggedIn() const;

    FString GetVoiceChatLoggedInPlayerName() const;

    void ExecuteVoiceChatLogin(UMOS_AsyncResult *Result);
    void ExecuteVoiceChatLogout(UMOS_AsyncResult *Result);
    TArray<FMOSInterfaceListEntry> GetVoiceChatInputDevices() const;
    FString GetVoiceChatCurrentInputDevice() const;
    void SetVoiceChatCurrentInputDevice(const FString &Id);
    float GetVoiceChatInputVolume() const;
    void SetVoiceChatInputVolume(float Volume);
    bool GetVoiceChatInputMuted() const;
    void SetVoiceChatInputMuted(bool bIsMuted);
    TArray<FMOSInterfaceListEntry> GetVoiceChatOutputDevices() const;
    FString GetVoiceChatCurrentOutputDevice() const;
    void SetVoiceChatCurrentOutputDevice(const FString &Id);
    float GetVoiceChatOutputVolume() const;
    void SetVoiceChatOutputVolume(float Volume);
    bool GetVoiceChatOutputMuted() const;
    void SetVoiceChatOutputMuted(bool bIsMuted);
    EMOSInterfaceVoiceChatConnectionStatus GetVoiceChatConnectionStatus() const;
    FString GetVoiceChatSetting(const FString &SettingKey) const;
    void SetVoiceChatSetting(const FString &SettingKey, const FString &SettingValue);
    TArray<FMOSInterfaceListEntry> GetVoiceChatChannels() const;
	
	/* PARTIES */

	TArray<FMOSInterfaceListEntry> GetPartiesJoinedParties() const;
    void ExecutePartiesCreateParty(UMOS_AsyncResult *Result);
    TArray<FMOSInterfaceListEntry> GetPartiesCurrentInvites() const;
    void ExecutePartiesAcceptInvite(const FMOSInterfaceListEntry &Invite, UMOS_AsyncResult *Result);
    TArray<FUniqueNetIdRepl> GetPartiesCurrentMembers(const FString &PartyId) const;
    void ExecutePartiesLeaveParty(const FString &PartyId, UMOS_AsyncResult *Result);
    void ExecutePartiesKickMember( const FString &PartyId, const FUniqueNetIdRepl &MemberId, UMOS_AsyncResult *Result);
    void ExecutePartiesInviteFriend(const FString &PartyId, const FUniqueNetIdRepl &MemberId, UMOS_AsyncResult *Result);
	TMulticastDelegate<void()> PartiesOnPartiesStateChanged;
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RaisePartiesOnPartiesStateChanged", Category = "Friends"))
	void RaisePartiesOnPartiesStateChanged();
	
    /* PRESENCE */
	
	TArray<FText> GetPresenceAvailableStatuses() const;
	void ExecutePresenceSetPresence(const FText &NewStatus, const FString &NewStatusTextNamespace, const FString &NewStatusTextKey, UMOS_AsyncResult *Result);

	/* USERS */
	
	void ExecuteUsersQueryUserInfo(const FString &UserId, UMOS_TextAsyncResult *Result);
	void ExecuteUsersQueryUserByDisplayName(const FString &DisplayName, UMOS_TextAsyncResult *Result);
	void ExecuteUsersQueryExternalIds(const FString &PlatformName, const TArray<FString> &ExternalIds, UMOS_TextAsyncResult *Result);
	
	/* STATS */

	void ExecuteStatsQueryStats(UMOS_QueryStatsAsyncResult *Result);
	void ExecuteStatsIngestStat(const FString &StatName, double StatValue, UMOS_AsyncResult *Result);

	/* ACHIEVEMENTS */

	void ExecuteAchievementsQueryAchievements(UMOS_QueryAchievementsAsyncResult *Result);
	void ExecuteAchievementsUnlockAchievement(const FString &Id, UMOS_AsyncResult *Result);

	/* LEADERBOARDS */

    void ExecuteLeaderboardsQueryGlobalLeaderboards(UMOS_QueryLeaderboardsAsyncResult *Result);
	void ExecuteLeaderboardsQueryFriendsLeaderboards(UMOS_QueryLeaderboardsAsyncResult *Result);

	/* USER CLOUD */

	void ExecuteUserCloudQueryFiles(UMOS_ListAsyncResult *Result);
	void ExecuteUserCloudWriteStringToFile(const FString &FileName, const FString &FileContents, UMOS_AsyncResult *Result);
	void ExecuteUserCloudWriteSaveGameToFile(const FString &FileName, double SaveGameNumber, UMOS_AsyncResult *Result);
	void ExecuteUserCloudReadStringFromFile(const FString &FileName, UMOS_ReadFileStringAsyncResult *Result);
	void ExecuteUserCloudReadSaveGameFromFile(const FString &FileName, UMOS_ReadFileSaveGameAsyncResult *Result);
	/* TITLE FILE */

	void ExecuteTitleFileQueryFiles(UMOS_ListAsyncResult *Result);
	void ExecuteTitleFileReadStringFromFile(const FString &FileName, UMOS_ReadFileStringAsyncResult *Result);

	FDelegateHandle WorldInitHandle;
};