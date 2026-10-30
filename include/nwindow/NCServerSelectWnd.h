#pragma once
// NCServerSelectWnd - the server list of the login screen (UClass "CServerSelectWnd", 0xC8;
// NCLoginWnd +0xA0). The login server's list arrives a row at a time (AddServer: name, load,
// ping, type) and ends with OnServerListEnd; OK (or Enter, or a double click) logs in to the
// selected server, Cancel goes back to the login box, "Quality" orders the list: the server
// played last on top, the others by ping and, within a ping class, by users. Pings are asked
// of the network handler and kept in s_info.ini (section = server index, key "Time"), with
// the time they were taken ([Date] Time); the old ones are asked again.

#include "nwindow/NCWnd.h"
#include "nwindow/NCListCtrl.h"

class NCButton;
class L2ParamStack;

// NCServerSelectListCtrl - the list itself (UClass "CServerSelectListCtrl", 0x20C): sorting by
// a column also keeps the window's LastServerRow on its server
class NCServerSelectListCtrl : public NCListCtrl
{
    NW_DECLARE_CLASS(NCServerSelectListCtrl, NCListCtrl, 0, 0x1014a200)

    NCServerSelectListCtrl();                                        // 0x100a3320
    NCServerSelectListCtrl(INT InMaxRows, INT InVisibleRows, INT InMaxColumns, INT Unused); // 0x100a33c0
    // ~NCServerSelectListCtrl                                       // 0x100a3370

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100a1e60
    virtual INT SortByColumn(INT Column);                            // 68 0x100a30d0
};
NW_ASSERT_SIZE(NCServerSelectListCtrl, 0x20C);

class NCServerSelectWnd : public NCWnd
{
    NW_DECLARE_CLASS(NCServerSelectWnd, NCWnd, 0, 0x10149cd8)

    UTexture* BackTex;               // +0x94 "L2UI.lobbywnd.serverback"
    NCServerSelectListCtrl* ServerList; // +0x98
    NCButton* OKButton;              // +0x9C
    NCButton* CancelButton;          // +0xA0
    NCButton* QualityButton;         // +0xA4
    INT LastServerID;                // +0xA8 server the account played on last (login server)
    INT LastServerRow;               // +0xAC its row, -1 = none
    INT PendingPings;                // +0xB0 ping answers still awaited
    INT TimerResult;                 // +0xB4 what SetTimer returned (never read)
    UBOOL bRequested;                // +0xB8 a server login went out (cleared by the timer)
    UBOOL bPingCacheOld;             // +0xBC the saved pings are too old: ask every server
    UBOOL bPingSaved;                // +0xC0 a ping went to s_info.ini
    UBOOL UnknownC4;                 // +0xC4 set at the end of the list when bPingSaved (never
                                     //   read)

    NCServerSelectWnd();                                             // 0x100a2360
    // ~NCServerSelectWnd                                            // 0x100a23d0

    virtual INT WindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);  // 24 0x100a1ed0
    virtual INT OnTimer(INT TimerID);                                // 34 0x100a2080
    virtual INT OnCreate();                                          // 35 0x100a33f0
    virtual INT OnDestroy();                                         // 36 0x100367a0 (folded)
    virtual INT OnPaint(NCGDevice* Device);                          // 50 0x100a2420
    virtual INT OnKeyDown(WPARAM wParam, LPARAM lParam);             // 53 0x100a2050
    // names ours
    virtual INT OnOK();                                              // 62 0x100a2b30
    virtual INT OnCancel();                                          // 63 0x100a2cb0
    virtual INT SortByQuality();                                     // 64 0x100a2d30
    virtual INT OnListNotify(INT Row);                               // 65 0x100a2040

    // names ours
    void SetLastServerID(INT ServerID);                              // 0x100a1fa0
    void OnServerListEnd();                                          // 0x100a1fb0
    void ClearServers();                                             // 0x100a2010
    void AddServer(L2ParamStack& Params);                            // 0x100a2500
    void SetPing(INT IP, INT Ping);                                  // 0x100a2a50
    UBOOL IsPingCacheOld();                                          // 0x100a2220
    UBOOL HasNoPing(INT ServerIndex);                                // 0x100a2270
    void SavePing(INT ServerIndex, INT Ping);                        // 0x100a22d0
    void SavePingTime();                                             // 0x100a2320

    // calendar of the ping cache (names ours)
    static UBOOL IsLeapYear(INT Year);                               // 0x100a20a0
    static INT DaysInYears(INT Years);                               // 0x100a20e0
    static INT DaysInMonths(INT Year, INT Month);                    // 0x100a2120
    static INT HoursInDays(INT Day, INT Hour);                       // 0x100a2190
    static INT GetTimeStamp();                                       // 0x100a21c0
};
NW_ASSERT_OFFSET(NCServerSelectWnd, ServerList, 0x98);
NW_ASSERT_OFFSET(NCServerSelectWnd, LastServerID, 0xA8);
NW_ASSERT_OFFSET(NCServerSelectWnd, bRequested, 0xB8);
NW_ASSERT_OFFSET(NCServerSelectWnd, UnknownC4, 0xC4);
NW_ASSERT_SIZE(NCServerSelectWnd, 0xC8);
