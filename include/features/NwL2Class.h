/*=============================================================================
    NwL2Class.h - phase 6: UClasses only the NWindowL2 build registers.

    A feature window with fields of its own is a class of its own, registered like
    the 178 stock ones (NW_DECLARE_CLASS / NW_IMPLEMENT_CLASS in NWindow.h), so Core
    allocates its real size - but through these macros:
      * NW_DECLARE_L2_CLASS / NW_DECLARE_L2_ABSTRACT_CLASS: no stock UClass address;
        tools/verify_standalone.py (check "source") tells them from the stock
        declarations by the name;
      * NW_IMPLEMENT_L2_CLASS: the IMPLEMENT_CLASS and the autoclass<Class> export,
        as UE2's IMPLEMENT_CLASS has it (the stock classes get theirs, at the stock
        ordinals, from src/standalone/NWindow.def; these take the next ordinals). The
        linker's /EXPORT does not add the C name's '_' as the .def does: it is given.
    Every such class is a line of tests/standalone/l2_extras.csv. With --l2,
    tools/verify_standalone.py and tools/smoke_standalone.py accept exactly those
    classes on top of the stock ones; without it they stay as strict as for the
    faithful build, which has none.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

#include "nwindow/NWindow.h"

#define NW_DECLARE_L2_CLASS(TClass, TSuperClass, TStaticFlags)                               \
    NW_DECLARE_CLASS(TClass, TSuperClass, TStaticFlags, 0)
#define NW_DECLARE_L2_ABSTRACT_CLASS(TClass, TSuperClass, TStaticFlags)                      \
    NW_DECLARE_ABSTRACT_CLASS(TClass, TSuperClass, TStaticFlags, 0)
#define NW_IMPLEMENT_L2_CLASS(TClass, TSuperClass, TClassFlags)                              \
    NW_IMPLEMENT_CLASS(TClass, TSuperClass, TClassFlags)                                     \
    __pragma(comment(linker, "/EXPORT:autoclass" #TClass "=_autoclass" #TClass ",DATA"))

#endif  // NW_FEATURES
