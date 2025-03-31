// Copyright June Rhodes. All Rights Reserved.

#pragma once

#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_CallManagerBase.h"
#include "UObject/StrongObjectPtr.h"

namespace OSS::OnlineAPI
{

template <typename T> class TCallScopedObjectPtr
{
    template <typename OtherObjectType> friend class TCallScopedObjectPtr;

private:
    TWeakPtr<FCallManagerBase> Owner;
    TStrongObjectPtr<T> Ptr;
    FDelegateHandle Handle;

    void Cleanup()
    {
        this->Ptr.Reset();
        if (this->Handle.IsValid())
        {
            auto OwnerPinned = this->Owner.Pin();
            if (OwnerPinned.IsValid())
            {
                OwnerPinned->OnDestruction.Remove(this->Handle);
                this->Handle.Reset();
            }
        }
    }

public:
    TCallScopedObjectPtr()
        : Owner()
        , Ptr()
        , Handle()
    {
    }
    TCallScopedObjectPtr(const TSharedRef<FCallManagerBase> &InOwner, T *InPtr)
        : Owner(InOwner)
        , Ptr(InPtr)
    {
        this->Handle = InOwner->OnDestruction.AddLambda([this]() {
            this->Cleanup();
        });
    }
    TCallScopedObjectPtr(const TCallScopedObjectPtr &InOther)
        : Owner(InOther.Owner)
        , Ptr(InOther.Ptr)
    {
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
    }
    TCallScopedObjectPtr(TCallScopedObjectPtr &&InOther)
        : Owner(InOther.Owner)
        , Ptr(InOther.Ptr)
    {
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
        InOther.Cleanup();
    }
    template <typename OtherObjectType UE_REQUIRES(std::is_convertible_v<OtherObjectType *, T *>)>
    TCallScopedObjectPtr(const TCallScopedObjectPtr<OtherObjectType> &InOther)
        : Owner(InOther.Owner)
        , Ptr(InOther.Ptr)
    {
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
    }
    ~TCallScopedObjectPtr()
    {
        this->Cleanup();
    }

    TCallScopedObjectPtr &operator=(const TCallScopedObjectPtr &InOther)
    {
        this->Cleanup();
        this->Ptr = InOther.Ptr;
        this->Owner = InOther.Owner;
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
        return *this;
    }

    template <typename OtherObjectType UE_REQUIRES(std::is_convertible_v<OtherObjectType *, T *>)>
    TCallScopedObjectPtr &operator=(const TCallScopedObjectPtr<OtherObjectType> &InOther)
    {
        this->Cleanup();
        this->Ptr = InOther.Ptr;
        this->Owner = InOther.Owner;
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
        return *this;
    }

    TCallScopedObjectPtr &operator=(TCallScopedObjectPtr &&InOther)
    {
        this->Cleanup();
        this->Ptr = InOther.Ptr;
        this->Owner = InOther.Owner;
        auto OwnerPinned = this->Owner.Pin();
        if (OwnerPinned.IsValid())
        {
            this->Handle = OwnerPinned->OnDestruction.AddLambda([this]() {
                this->Cleanup();
            });
        }
        InOther.Cleanup();
        return *this;
    }

    T *operator->() const
    {
        return this->Ptr.Get();
    }

    T &operator*() const
    {
        checkf(this->IsValid(), TEXT("Expected pointer to be valid before using * operator!"));
        return *this->Ptr;
    }

    T *Get() const
    {
        return this->Ptr.Get();
    }

    bool IsValid() const
    {
        return this->Owner.IsValid() && this->Ptr.IsValid();
    }

    void Reset()
    {
        this->Cleanup();
    }

    friend uint32 GetTypeHash(const TCallScopedObjectPtr &Value)
    {
        return GetTypeHashHelper(Value.Ptr);
    }

    bool operator==(const TCallScopedObjectPtr &InOther) const
    {
        return this->Ptr == InOther.Ptr;
    }
};

} // namespace OSS::OnlineAPI


