#include "re/NwHook.h"

#include "re/NwLog.h"
#include "re/NwPrologue.h"

#include <string.h>

extern "C" DWORD g_nwBase = 0;

namespace nw {

static HookEntry* g_first = NULL;

HookReg::HookReg(DWORD va, void* fn, const char* name, const char* file)
{
    // runs during our static initialization; no allocation, the list is intrusive
    e.va = va;
    e.fn = fn;
    e.name = name;
    e.file = file;
    e.state = HOOK_PENDING;
    e.trampoline = NULL;
    e.next = g_first;
    g_first = &e;
}

HookEntry* FirstHook() { return g_first; }

static VtableCheck* g_firstVtable = NULL;

VtableReg::VtableReg(const char* name, const void* const* ours, DWORD stockVA, int slots)
{
    v.name = name;
    v.ours = ours;
    v.stockVA = stockVA;
    v.slots = slots;
    v.next = g_firstVtable;
    g_firstVtable = &v;
}

VtableCheck* FirstVtable() { return g_firstVtable; }

const HookEntry* FindHook(DWORD va)
{
    for (HookEntry* h = g_first; h; h = h->next)
        if (h->va == va && h->state == HOOK_INSTALLED)
            return h;
    return NULL;
}

static const Prologue* FindPrologue(DWORD va)
{
    unsigned lo = 0, hi = kPrologueCount;
    while (lo < hi)
    {
        unsigned mid = (lo + hi) / 2;
        if (kPrologues[mid].va < va)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (lo < kPrologueCount && kPrologues[lo].va == va) ? &kPrologues[lo] : NULL;
}

// One block of executable memory holds every trampoline (<= 3561 x ~16 bytes).
static BYTE* g_pool = NULL;
static DWORD g_poolUsed = 0;
static const DWORD kPoolSize = 0x20000;

static BYTE* AllocExec(DWORD n)
{
    if (!g_pool)
        g_pool = (BYTE*)VirtualAlloc(NULL, kPoolSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!g_pool || g_poolUsed + n > kPoolSize)
        return NULL;
    BYTE* p = g_pool + g_poolUsed;
    g_poolUsed += (n + 15) & ~15u;
    return p;
}

static bool StockBytesMatch(const Prologue& p, const BYTE* code)
{
    for (int i = 0; i < 8; i++)
        if (!(p.relmask & (1 << i)) && code[i] != p.expect[i])
            return false;
    return true;
}

// Copy of the stolen instructions + jmp back, so the stock implementation stays callable.
static BYTE* MakeTrampoline(const Prologue& p, const BYTE* code)
{
    if (p.flags & PF_NOTRAMP)
        return NULL;
    BYTE* t = AllocExec(p.steal + 5);
    if (!t)
        return NULL;
    memcpy(t, code, p.steal);
    if (p.flags & PF_REL32)
    {
        // call/jmp rel32 (E8/E9) or jcc rel32 (0F 8x): keep the absolute target
        int opLen = (code[p.relat] == 0x0F) ? 2 : 1;
        const BYTE* insEnd = code + p.relat + opLen + 4;
        const BYTE* target = insEnd + *(const INT*)(code + p.relat + opLen);
        *(INT*)(t + p.relat + opLen) = (INT)(target - (t + p.relat + opLen + 4));
    }
    if (!(p.flags & PF_ENDS))
    {
        t[p.steal] = 0xE9;
        *(INT*)(t + p.steal + 1) = (INT)((code + p.steal) - (t + p.steal + 5));
    }
    return t;
}

int InstallHooks()
{
    int installed = 0;
    for (HookEntry* h = g_first; h; h = h->next)
    {
        if (h->state != HOOK_PENDING)
            continue;
        for (HookEntry* o = h->next; o; o = o->next)
            if (o->va == h->va)
            {
                o->state = HOOK_ERR_DUPLICATE;
                Log("  ERROR duplicate hook %08lx: %s (%s) and %s (%s)", h->va, h->name, h->file, o->name, o->file);
            }
        if (!GetConfig().hooksEnabled || IsDisabledByConfig(h->va, h->name))
        {
            h->state = HOOK_DISABLED;
            continue;
        }
        const Prologue* p = FindPrologue(h->va);
        if (!p)
        {
            h->state = HOOK_ERR_UNKNOWN;
            continue;
        }
        BYTE* code = (BYTE*)Addr(h->va);
        if (!StockBytesMatch(*p, code))
        {
            h->state = HOOK_ERR_BYTES;
            continue;
        }
        if (p->room < 5)
        {
            h->state = HOOK_ERR_SPACE;
            continue;
        }
        h->trampoline = MakeTrampoline(*p, code);
        DWORD old;
        if (!VirtualProtect(code, 5, PAGE_EXECUTE_READWRITE, &old))
        {
            h->state = HOOK_ERR_PROTECT;
            continue;
        }
        code[0] = 0xE9;
        *(INT*)(code + 1) = (INT)((BYTE*)h->fn - (code + 5));
        VirtualProtect(code, 5, old, &old);
        FlushInstructionCache(GetCurrentProcess(), code, 5);
        h->state = HOOK_INSTALLED;
        installed++;
    }
    return installed;
}

void* Stock(DWORD va)
{
    const HookEntry* h = FindHook(va);
    if (h)
        return h->trampoline;  // NULL when the prologue could not be relocated
    return Addr(va);
}

}  // namespace nw
