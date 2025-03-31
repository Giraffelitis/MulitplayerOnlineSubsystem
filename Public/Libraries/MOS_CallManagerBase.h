// Copyright June Rhodes. All Rights Reserved.

#pragma once

#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_CallManagerBase.h"
#include "UObject/StrongObjectPtr.h"

namespace OSS::OnlineAPI
{

	class MULTIPLAYERONLINESUBSYSTEM_API FCallManagerBase
	{
	public:
		FCallManagerBase() = default;
		UE_NONCOPYABLE(FCallManagerBase);
		virtual ~FCallManagerBase() = default;

		FSimpleMulticastDelegate OnDestruction;
	};

} // namespace OSS::OnlineAPI
