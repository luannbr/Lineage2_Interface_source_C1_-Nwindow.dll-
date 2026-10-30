#pragma once
// NCObject - root of every NWindow class (C++ name NCObject, UClass "CObject").
//
// Besides debug output and texture lookup it wraps what windows need from the outside:
// the IME of the game window, MFC-style timers (run by the console, delivered as WM_TIMER
// to WindowProc), the mouse cursor and actor spawning.

#include "nwindow/NWindow.h"

class AActor;

class NCObject : public UObject
{
    NW_DECLARE_CLASS(NCObject, UObject, 0, 0x101228b0)

    NCObject();                                          // 0x10020ca0 (ICF: NCAbstractMain, NCCmdTarget)
    // ~NCObject: ConditionalDestroy, ~UObject           // 0x10001600

    virtual void Destroy();                              // 0x10020cc0 (vslot 10)

    // names ours
    INT VARARGS Debugf(const TCHAR* Fmt, ...);           // 0x10020cd0
    UTexture* FindTexture(const TCHAR* Name);            // 0x10020e00
    AActor* SpawnActor(const TCHAR* ClassName);          // 0x10020e70

    DWORD ImeGetProperty(DWORD Index);                                // 0x10020d60
    HIMC ImeGetContext();                                             // 0x10020d80
    BOOL ImeReleaseContext(HIMC hIMC);                                // 0x10020d90
    BOOL ImeIsUIMessage(UINT Msg, WPARAM wParam, LPARAM lParam);      // 0x10020db0

    INT SetTimer(UINT ID, UINT Elapse, void* TimerProc);              // 0x10020dd0
    UBOOL KillTimer(UINT ID);                                         // 0x10020de0
    FLOAT GetDeltaTime();                                             // 0x10020f00

    void SetCursor(INT Type, UTexture* Texture);                      // 0x10020f10
    void LockCursor();                                                // 0x10020f20
    void UnlockCursor();                                              // 0x10020f30
};
NW_ASSERT_SIZE(NCObject, 0x28);
