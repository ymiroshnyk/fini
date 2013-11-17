/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_EVENT_H
#define _FINI_EVENT_H

#include "fini_types.h"
#include "fini_setup.h"

#include "boost/cast.hpp"

namespace Fini
{

class StateBase;
	
///////////////////////////////////////////////////////////////////////////////

class EventBase
{
	uint eventId_;
	mutable bool discarded_;

	EventBase(const uint eventId);

public:
	virtual ~EventBase() {}

	template <typename T>
	const bool isA() const;

	template <typename T>
	const T* castTo() const
	{
		return (isA<T>() ? 
#if FINI_NO_RTTI
			static_cast<const T*>(this) 
#else
			boost::polymorphic_downcast<const T*>(this) 
#endif
			: NULL);
	}


	bool isDiscarded() const;

private:
	void discard() const;
	static const uint retrieveNewEventId();

	template <typename>	friend class Event;
	friend class StateBase;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TMostDerived>
class Event : public EventBase
{
protected:
	Event() : EventBase(getEventId()) {}

private:
	static const uint getEventId()
	{
		static uint eventId = retrieveNewEventId();
		return eventId;
	}

	friend class EventBase;
};

///////////////////////////////////////////////////////////////////////////////
	
template <typename T>
const bool EventBase::isA() const
{
	return eventId_ == Event<T>::getEventId();
}
	
///////////////////////////////////////////////////////////////////////////////
	
}

#endif