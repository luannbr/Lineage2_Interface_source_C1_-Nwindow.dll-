/*=============================================================================
    CraftBalloon.h - phase 6: the private store balloon of a manufacture store
    (store type 5), NWindow_C1 src/inject/CraftBalloon.cpp; its docs
    CRAFT_BALLOON_ACTIVATION.md, FEATURE_CRAFT_BALLOON.md,
    MANUFACTURE_BALLOON_SHOWS_BUY.md. Notes: docs/notas/F6_Craft.md.

    The balloon over a sitting store owner is drawn by Engine.dll, not NWindow:
    FDynamicActor::Render (RVA 0x27D680) reads the owner's store type (User +0x1F8),
    types 1 and 2 get the sell label and colour, every other type the buy ones - so
    the server's type 5 showed "<Private Store - Buy>". This stays a runtime patch of
    Engine.dll, as in NWindow_C1: six sites of that function, each installed only
    when its bytes are the expected ones (else logged and left stock). With type 5
    the label becomes "<Private Workshop - Craft>" in cyan, the second line (the
    store's title) kept.
=============================================================================*/
#pragma once

#include "features/NwFeatures.h"

#if NW_FEATURES

// the console tick: installs the Engine patch the first time (the balloons are drawn in the
// world only, long after the first tick)
void CraftBalloon_OnTick();

#endif  // NW_FEATURES
