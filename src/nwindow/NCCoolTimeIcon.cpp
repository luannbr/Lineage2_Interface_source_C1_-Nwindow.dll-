/*=============================================================================
    NCCoolTimeIcon.cpp - the cool time drawn over a skill icon. Stock: in the middle of
    NCItemWnd's range (0x10016730 .. 0x10016850, 0x100170e0 .. 0x100172a0, 0x100173c0 ..
    0x10017550).
=============================================================================*/
#include "nwindow/NCCoolTimeIcon.h"

#include <intrin.h>
#include <stdio.h>

// UE2's appSeconds (UnVcWin32.h): the time stamp counter in seconds, plus 2^24 "to make bugs
// apparent where the value is passed to a FLOAT" (stock: inline everywhere here, one copy out
// of line at 0x10016620 for the pawn viewer)
static FORCEINLINE DOUBLE appSeconds()
{
    unsigned __int64 Cycles = __rdtsc();
    DWORD L = (DWORD)Cycles;
    DWORD H = (DWORD)(Cycles >> 32);
    return ((DOUBLE)L + 4294967296.0 * (DOUBLE)H) * GSecondsPerCycle + 16777216.0;
}

// 0x100173c0 - the first icon loads the frames. NWindow's texture loading keeps a "failed"
// flag: after a miss the rest are not even looked up. A missing rotation frame leaves the
// texture NULL, and the flag write below goes through it (the client crashes); a missing end
// frame is only left out.
NCCoolTimeIcon::NCCoolTimeIcon()
{
    Flags &= ~3;
    bRunning = 0;
    bFlashing = 0;
    FlashTime = 0.5;
    bSoundDue = 0;
    if (GCoolTimeTextures.Num() == 0)
    {
        TCHAR Name[256];
        UBOOL bFailed = 0;
        UTexture* Frame = NULL;
        for (INT i = 0; i < 360; i++)
        {
            _swprintf(Name, TEXT("%s%d"), TEXT("L2_SkillTime.Rotation_Type.cooltime"), i);
            if (!bFailed)
            {
                Frame = (UTexture*)UObject::StaticFindObject(UTexture::StaticClass(), ANY_PACKAGE, Name);
                if (!Frame)
                    Frame = (UTexture*)UObject::StaticLoadObject(UTexture::StaticClass(), NULL, Name, NULL, 0, NULL);
                if (Frame)
                    Frame->SetFlags(RF_Native);
            }
            if (!Frame || bFailed)
                bFailed = 1;
            Frame->TextureFlags |= 2;
            GCoolTimeTextures.AddItem(Frame);
        }
        for (INT i = 1; i < 10; i++)
        {
            _swprintf(Name, TEXT("L2_SkillTime.CooltimeEnd.CooltimeEnd00%d"), i);
            if (!bFailed)
            {
                Frame = (UTexture*)UObject::StaticFindObject(UTexture::StaticClass(), ANY_PACKAGE, Name);
                if (!Frame)
                    Frame = (UTexture*)UObject::StaticLoadObject(UTexture::StaticClass(), NULL, Name, NULL, 0, NULL);
                if (Frame)
                    Frame->SetFlags(RF_Native);
            }
            if (!Frame || bFailed)
                bFailed = 1;
            if (Frame)
                GCoolTimeEndTextures.AddItem(Frame);
        }
    }
}
NW_HOOK_CTOR(0x100173c0, NCCoolTimeIcon)

// 0x10016730 - Total seconds of the cool time, Remaining of them still to go (0: all); the
// frames already gone are skipped
void NCCoolTimeIcon::SetTime(DOUBLE Total, DOUBLE Remaining)
{
    StartFrame = 0;
    Time = 0.0f;
    Duration = Total;
    FrameTime = Total / GCoolTimeTextures.Num();
    bRunning = 0;
    if (Remaining == 0.0)
        Remaining = Total;
    StartFrame = (INT)((Total - Remaining) / FrameTime);
    Duration = Remaining;
    StartTime = appSeconds();
}
NW_HOOK_METHOD(0x10016730, NCCoolTimeIcon, SetTime)

// 0x10016800
void NCCoolTimeIcon::Start(DOUBLE Total, DOUBLE Remaining)
{
    SetTime(Total, Remaining);
    bRunning = 1;
    bFlashing = 0;
}
NW_HOOK_METHOD(0x10016800, NCCoolTimeIcon, Start)

// 0x10016830
void NCCoolTimeIcon::Stop()
{
    bRunning = 0;
}
NW_HOOK_METHOD(0x10016830, NCCoolTimeIcon, Stop)

// 0x10016840
UBOOL NCCoolTimeIcon::IsIdle()
{
    return !bRunning;
}
NW_HOOK_METHOD(0x10016840, NCCoolTimeIcon, IsIdle)

// 0x10016850 - the end frames over Seconds
void NCCoolTimeIcon::StartFlash(DOUBLE Seconds)
{
    Time = 0.0f;
    Duration = Seconds;
    FrameTime = Seconds / GCoolTimeEndTextures.Num();
    bRunning = 0;
    bSoundDue = 1;
    StartTime = appSeconds();
}
NW_HOOK_METHOD(0x10016850, NCCoolTimeIcon, StartFlash)

// 0x100170e0
void NCCoolTimeIcon::EndCoolTime()
{
    bRunning = 0;
    StartFlash(FlashTime);
    bFlashing = 1;
}
NW_HOOK_METHOD(0x100170e0, NCCoolTimeIcon, EndCoolTime)

// 0x10017110 - picks the frame of TimeSeconds into the animation: the one whose span holds
// it, or the last one once past the end (unless the frames loop). The cool time frames start
// at StartFrame, the end frames at the first. The running sum stays in an x87 register in the
// stock code (never rounded to FLOAT), hence DOUBLE.
void NCCoolTimeIcon::PreSetMaterial(DOUBLE TimeSeconds)
{
    guard(NCCoolTimeIcon::PreSetMaterial);
    if (Flags & 2)
        return;
    Time = (FLOAT)TimeSeconds;
    if ((Flags & 1) && Duration > 0.0 && Time > Duration)
    {
        Time = (FLOAT)(Time - appFloor((FLOAT)(Time / Duration)) * Duration);
        while (Time > Duration)
            Time = (FLOAT)(Time - Duration);
    }
    DOUBLE FrameStart = 0.0f;
    if (bRunning == 1)
    {
        INT Num = GCoolTimeTextures.Num();
        for (INT i = StartFrame; i < Num; i++)
        {
            if (FrameStart <= Time && (Time < FrameStart + FrameTime || (!(Flags & 1) && i == Num - 1)))
            {
                Texture = GCoolTimeTextures(i);
                break;
            }
            FrameStart += FrameTime;
        }
    }
    else if (bFlashing == 1)
    {
        INT Num = GCoolTimeEndTextures.Num();
        for (INT i = 0; i < Num; i++)
        {
            if (FrameStart <= Time && (Time < FrameStart + FrameTime || (!(Flags & 1) && i == Num - 1)))
            {
                Texture = GCoolTimeEndTextures(i);
                break;
            }
            FrameStart += FrameTime;
        }
    }
    unguard;
}
NW_HOOK_METHOD(0x10017110, NCCoolTimeIcon, PreSetMaterial)

// 0x100172a0 - the end frame of now, NULL once the flash is over
UTexture* NCCoolTimeIcon::GetFlashTexture()
{
    DOUBLE Now = appSeconds();
    DOUBLE Elapsed = Now - StartTime;
    if (Elapsed < 0.0)
        Elapsed = 0.0;
    if (Duration + StartTime < Now)
    {
        bFlashing = 0;
        return NULL;
    }
    PreSetMaterial(Elapsed);
    return Texture;
}
NW_HOOK_METHOD(0x100172a0, NCCoolTimeIcon, GetFlashTexture)

// 0x10017550 - the cool time frame of now, NULL once over: then the flash starts, unless the
// end is more than half a second gone (the icon was not drawn meanwhile)
UTexture* NCCoolTimeIcon::GetTexture()
{
    DOUBLE Now = appSeconds();
    DOUBLE Elapsed = Now - StartTime;
    if (Elapsed < 0.0)
        Elapsed = 0.0;
    DOUBLE End = Duration + StartTime;
    if (0.5 + End < Now)
    {
        Stop();
        return NULL;
    }
    if (End < Now)
    {
        EndCoolTime();
        return NULL;
    }
    PreSetMaterial(Elapsed);
    return Texture;
}
NW_HOOK_METHOD(0x10017550, NCCoolTimeIcon, GetTexture)
