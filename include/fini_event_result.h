/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_EVENT_RESULT_H
#define _FINI_EVENT_RESULT_H

#include "fini_setup.h"

namespace Fini
{

class StateDesc;
///////////////////////////////////////////////////////////////////////////////

class EventResult
{
	enum EId
	{
		I_Forwarded,
		I_Discarded,
		I_Transited,
	};

	EId id_;
	const StateDesc* transitParent_;

	EventResult(EId id, const StateDesc* transitParent = 0);

public:
	static EventResult createForwarded();
	static EventResult createDiscarded();
	static EventResult createTransited(const StateDesc& transitParent);

	bool isForwarded() const { return id_ == I_Forwarded; }
	bool isDiscarded() const { return id_ == I_Discarded; }
	const StateDesc* getTransited() const { return transitParent_; }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif