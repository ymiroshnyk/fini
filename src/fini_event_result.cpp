/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_event_result.h"
#include "fini_check.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

EventResult::EventResult(EId id, const StateDesc* transitParent)
: id_(id)
, transitParent_(transitParent)
{
	FINI_CHECK(id_ != I_Transited || transitParent);
}

///////////////////////////////////////////////////////////////////////////////

EventResult EventResult::createForwarded()
{
	return EventResult(I_Forwarded);
}

///////////////////////////////////////////////////////////////////////////////

EventResult EventResult::createDiscarded()
{
	return EventResult(I_Discarded);
}

///////////////////////////////////////////////////////////////////////////////

EventResult EventResult::createTransited(const StateDesc& transitParent)
{
	return EventResult(I_Transited, &transitParent);
}

///////////////////////////////////////////////////////////////////////////////
}