// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "OnlineSubsystemUtils.h"

UMOS_GameInstanceSubsystem::~UMOS_GameInstanceSubsystem()
{
    if (this->CachedVoiceChatUser != nullptr)
    {
        auto *VC = IVoiceChat::Get();
        if (VC != nullptr)
        {
            VC->ReleaseUser(this->CachedVoiceChatUser);
            this->CachedVoiceChatUser = nullptr;
        }
    }
}

bool UMOS_GameInstanceSubsystem::GetVoiceChatIsLoggedIn() const
{

    return CachedVoiceChatUser != nullptr && CachedVoiceChatUser->IsLoggedIn();
}

FString UMOS_GameInstanceSubsystem::GetVoiceChatLoggedInPlayerName() const
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the signed in player name.
    return CachedVoiceChatUser->GetLoggedInPlayerName();
}

void UMOS_GameInstanceSubsystem::ExecuteVoiceChatLogin(UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));

    // Get the cross-call storage.
/*    auto *LocalStorage = Cast<UMOS_GameInstanceSubsystem>(this->GetStorage());
    if (!IsValid(LocalStorage))
    {
        Result->OnResult(false, TEXT("No cross-call storage available."));
        return;
    } */

    // Get the voice chat implementation.
    auto *VC = IVoiceChat::Get();
    if (VC == nullptr)
    {
        Result->OnResult(false, TEXT("Voice chat service not available."));
        return;
    }

    // If we don't already have a voice chat user, create one.
    if (CachedVoiceChatUser == nullptr)
    {
        CachedVoiceChatUser = VC->CreateUser();
        if (CachedVoiceChatUser == nullptr)
        {
            Result->OnResult(false, TEXT("Unable to create voice chat user."));
            return;
        }
    }

    // Attempt to log the voice chat user in.
    CachedVoiceChatUser->Login(
        Identity->GetPlatformUserIdFromLocalUserNum(this->LocalUserNum),
        Identity->GetUniquePlayerId(this->LocalUserNum)->ToString(),
        TEXT(""),
        FOnVoiceChatLoginCompleteDelegate::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                 Result)](const FString &, const FVoiceChatResult &VoiceChatResult) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(VoiceChatResult.IsSuccess(), VoiceChatResult.ErrorDesc);
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteVoiceChatLogout(UMOS_AsyncResult *Result)
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        Result->OnResult(false, TEXT("Not signed in."));
        return;
    }

    // Attempt to log the voice chat user out.
    CachedVoiceChatUser->Logout(FOnVoiceChatLogoutCompleteDelegate::CreateWeakLambda(
        this,
        [ResultWk =
             TSoftObjectPtr<UMOS_AsyncResult>(Result)](const FString &, const FVoiceChatResult &VoiceChatResult) {
            // Make sure the result callback is still valid.
            if (!ResultWk.IsValid())
            {
                return;
            }

            // Return the result.
            ResultWk->OnResult(VoiceChatResult.IsSuccess(), VoiceChatResult.ErrorDesc);
        }));
}

TArray<FOSSInterfaceListEntry> UMOS_GameInstanceSubsystem::GetVoiceChatInputDevices() const
{
    // Make sure our voice chat user is signed in.
    
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Return the voice chat input devices.
    TArray<FOSSInterfaceListEntry> Entries;
    for (const auto &InputDevice : CachedVoiceChatUser->GetAvailableInputDeviceInfos())
    {
        FOSSInterfaceListEntry Entry;
        Entry.Id = InputDevice.Id;
        Entry.DisplayName = FText::FromString(InputDevice.DisplayName);
        Entries.Add(Entry);
    }
    return Entries;
}

FString UMOS_GameInstanceSubsystem::GetVoiceChatCurrentInputDevice() const
{
    // Make sure our voice chat user is signed in.
    
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the current input device ID.
    return CachedVoiceChatUser->GetInputDeviceInfo().Id;
}

void UMOS_GameInstanceSubsystem::SetVoiceChatCurrentInputDevice(const FString &Id)
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the input device ID.
    CachedVoiceChatUser->SetInputDeviceId(Id);
}

float UMOS_GameInstanceSubsystem::GetVoiceChatInputVolume() const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr ||
        CachedVoiceChatUser->IsLoggedIn())
    {
        return 0.0f;
    }

    // Return the volume.
    return CachedVoiceChatUser->GetAudioInputVolume();
}

void UMOS_GameInstanceSubsystem::SetVoiceChatInputVolume(float Volume)
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the volume.
    CachedVoiceChatUser->SetAudioInputVolume(Volume);
}

bool UMOS_GameInstanceSubsystem::GetVoiceChatInputMuted() const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return false;
    }

    // Return the mute status.
    return CachedVoiceChatUser->GetAudioInputDeviceMuted();
}

void UMOS_GameInstanceSubsystem::SetVoiceChatInputMuted(bool bIsMuted)
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the mute status.
    CachedVoiceChatUser->SetAudioInputDeviceMuted(bIsMuted);
}

TArray<FOSSInterfaceListEntry> UMOS_GameInstanceSubsystem::GetVoiceChatOutputDevices() const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Return the voice chat output devices.
    TArray<FOSSInterfaceListEntry> Entries;
    for (const auto &OutputDevice : CachedVoiceChatUser->GetAvailableOutputDeviceInfos())
    {
        FOSSInterfaceListEntry Entry;
        Entry.Id = OutputDevice.Id;
        Entry.DisplayName = FText::FromString(OutputDevice.DisplayName);
        Entries.Add(Entry);
    }
    return Entries;
}

FString UMOS_GameInstanceSubsystem::GetVoiceChatCurrentOutputDevice() const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the current output device ID.
    return CachedVoiceChatUser->GetOutputDeviceInfo().Id;
}

void UMOS_GameInstanceSubsystem::SetVoiceChatCurrentOutputDevice(const FString &Id)
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the output device ID.
    CachedVoiceChatUser->SetOutputDeviceId(Id);
}

float UMOS_GameInstanceSubsystem::GetVoiceChatOutputVolume() const
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return 0.0f;
    }

    // Return the volume.
    return CachedVoiceChatUser->GetAudioOutputVolume();
}

void UMOS_GameInstanceSubsystem::SetVoiceChatOutputVolume(float Volume)
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the volume.
    CachedVoiceChatUser->SetAudioOutputVolume(Volume);
}

bool UMOS_GameInstanceSubsystem::GetVoiceChatOutputMuted() const
{
    // Make sure our voice chat user is signed in.

    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return false;
    }

    // Return the mute status.
    return CachedVoiceChatUser->GetAudioOutputDeviceMuted();
}

void UMOS_GameInstanceSubsystem::SetVoiceChatOutputMuted(bool bIsMuted)
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the mute status.
    CachedVoiceChatUser->SetAudioOutputDeviceMuted(bIsMuted);
}

EOSSInterfaceVoiceChatConnectionStatus UMOS_GameInstanceSubsystem::GetVoiceChatConnectionStatus() const
{
    // Make sure we have a voice chat implementation.
    auto *VC = IVoiceChat::Get();
    if (VC == nullptr)
    {
        return EOSSInterfaceVoiceChatConnectionStatus::NotConnected;
    }

    // Return the connection status.
    if (VC->IsConnecting())
    {
        return EOSSInterfaceVoiceChatConnectionStatus::Connecting;
    }
    else if (VC->IsConnected())
    {
        return EOSSInterfaceVoiceChatConnectionStatus::Connected;
    }
    else
    {
        return EOSSInterfaceVoiceChatConnectionStatus::NotConnected;
    }
}

FString UMOS_GameInstanceSubsystem::GetVoiceChatSetting(const FString &SettingKey) const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the voice chat setting.
    return CachedVoiceChatUser->GetSetting(SettingKey);
}

void UMOS_GameInstanceSubsystem::SetVoiceChatSetting(const FString &SettingKey, const FString &SettingValue)
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the voice chat setting.
    CachedVoiceChatUser->SetSetting(SettingKey, SettingValue);
}

TArray<FOSSInterfaceListEntry> UMOS_GameInstanceSubsystem::GetVoiceChatChannels() const
{
    // Make sure our voice chat user is signed in.
    if (CachedVoiceChatUser == nullptr || CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Return the voice chat channels.
    TArray<FOSSInterfaceListEntry> Entries;
    for (const auto &Channel : CachedVoiceChatUser->GetChannels())
    {
        FOSSInterfaceListEntry Entry;
        Entry.Id = Channel;
        Entry.DisplayName = FText::FromString(Channel);
        Entries.Add(Entry);
    }
    return Entries;
}