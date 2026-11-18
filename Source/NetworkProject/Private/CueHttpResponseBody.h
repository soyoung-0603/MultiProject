#pragma once

#include "CueJson.h"
#include "Misc/ScopeLock.h"

// Shared by one request's HTTP-thread stream and its game-thread completion.
class FCueHttpResponseBody
{
public:
    void Receive(void* Data, int64& Length)
    {
        FScopeLock Guard(&Mutex);
        if (bLimitExceeded || Length < 0 || Length > CueJson::MaxBodyBytes - Bytes.Num())
        {
            bLimitExceeded = true;
            Length = 0; // Signals the HTTP stream to abort, without buffering the overflow.
            return;
        }
        Bytes.Append(static_cast<const uint8*>(Data), static_cast<int32>(Length));
    }

    bool Read(FString& Body, FString& Error) const
    {
        FScopeLock Guard(&Mutex);
        if (bLimitExceeded) { Error = TEXT("Server response exceeds the 4 MiB limit."); return false; }
        if (Bytes.IsEmpty()) { Body.Reset(); return true; }
        const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
        Body = FString(Converted.Length(), Converted.Get());
        return true;
    }

private:
    mutable FCriticalSection Mutex;
    TArray<uint8> Bytes;
    bool bLimitExceeded = false;
};
