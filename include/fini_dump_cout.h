/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_DUMP_COUT_H
#define _FINI_DUMP_COUT_H

#include "fini_setup.h"

namespace Fini
{

class StateDesc;
///////////////////////////////////////////////////////////////////////////////

#if !FINI_NO_RTTI
void dumpCout(StateDesc& stateDesc);
#endif

///////////////////////////////////////////////////////////////////////////////
}

#endif