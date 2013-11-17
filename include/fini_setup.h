/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_SETUP_H
#define _FINI_SETUP_H

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

// if project doesn't use rtti set this to 1
#define FINI_NO_RTTI 0

// alignment for all data structures inside pools (in bytes)
#define FINI_ALIGNMENT 4

// sizeof() aligned with FINI_ALIGNMENT
#define FINI_ALIGNED_SIZEOF(x) (((sizeof(x) + FINI_ALIGNMENT - 1) / FINI_ALIGNMENT) * FINI_ALIGNMENT)

///////////////////////////////////////////////////////////////////////////////
}

#endif