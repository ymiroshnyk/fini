/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_EVENT_H
#define _FINI_EVENT_H

#include "fini_types.h"
#include "fini_typeid.h"

#include "boost/cast.hpp"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

class EventBase
{
	TypeInfo typeInfo_;
	mutable bool discarded_;

	EventBase(TypeInfo typeInfo)
		: typeInfo_(typeInfo), discarded_(false) {}

public:
	virtual ~EventBase() {}

	template <typename T>
	const bool isA() const
	{
		return typeInfo_ == typeId<T>();
	}

	template <typename T>
	const T* castTo() const
	{
		return (isA<T>() ? static_cast<const T*>(this) : NULL);
	}
	
	bool isDiscarded() const
	{
		return discarded_;
	}

private:
	void discard() const
	{
		discarded_ = true;
	}

	template <typename>	friend class Event;
	friend class StateBase;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TMostDerived>
class Event : public EventBase
{
protected:
	Event() : EventBase(typeId<TMostDerived>()) {}
};
	
///////////////////////////////////////////////////////////////////////////////
	
}

#endif