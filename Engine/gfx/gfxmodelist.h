//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Supported graphics mode interface
//
//=============================================================================
#ifndef __AGS_EE_GFX__GFXMODELIST_H
#define __AGS_EE_GFX__GFXMODELIST_H

#include "core/types.h"
#include "gfx/gfxdefines.h"

namespace AGS
{
namespace Engine
{

class IGfxModeList
{
public:
    virtual ~IGfxModeList() = default;
    virtual int  GetModeCount() const = 0;
    virtual bool GetMode(int index, DisplayMode &mode) const = 0;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__GFXMODELIST_H
