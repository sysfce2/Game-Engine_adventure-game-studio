//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#ifndef __AC_DEFINES_H
#define __AC_DEFINES_H

#include "core/platform.h"

// Some arbitrary return values, should be replaced with either
// simple boolean, or HError
#define EXIT_NORMAL 0
#define EXIT_CRASH  92
#define EXIT_ERROR  93

// Script name length limit for some game objects
#define MAX_SCRIPT_NAME_LEN 20
// Number of state-saved rooms
#define MAX_ROOMS 300

#if AGS_PLATFORM_OS_WINDOWS
#define AGS_INLINE inline
#else
// the linux compiler won't allow extern inline
#define AGS_INLINE
#endif

// object flags (currently only a char)
#define OBJF_NOINTERACT        1  // not clickable
#define OBJF_NOWALKBEHINDS     2  // ignore walk-behinds
#define OBJF_HASTINT           4  // the tint_* members are valid
#define OBJF_USEREGIONTINTS    8  // obey region tints/light areas
#define OBJF_USEROOMSCALING 0x10  // obey room scaling areas
#define OBJF_SOLID          0x20  // blocks characters from moving
#define OBJF_LEGACY_LOCKED  0x40  // object position is locked in the editor (OBSOLETE since 3.5.0)
#define OBJF_HASLIGHT       0x80  // the tint_light is valid and treated as brightness

// Animation flow mode
// NOTE: had to move to common_defines, because used by CharacterInfo
// Animates once and stops at the *last* frame
#define ANIM_ONCE              0
// Animates infinitely until stopped by command
#define ANIM_REPEAT            1
// Animates once and stops, resetting to the very first frame
#define ANIM_ONCERESET         2

#endif // __AC_DEFINES_H
