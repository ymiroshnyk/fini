/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_CASTABLE_H
#define _FINI_CASTABLE_H

#include <boost/cast.hpp>

#include "fini_types.h"
#include "fini_setup.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

template <typename TTag>
class Castable
{
	uint classId_;

	Castable(const uint classId) : classId_(classId) {}

public:
	virtual ~Castable() {}

	template <typename T>
	const bool isA() const;

	template <typename T>
	T* castTo()
	{
		return (isA<T>() ? 
#if FINI_NO_RTTI
			static_cast<T*>(this) 
#else
			boost::polymorphic_downcast<T*>(this) 
#endif
			: NULL);
	}

	template <typename T>
	const T* castTo() const
	{
		return const_cast<Castable<TTag>&>(*this).castTo<T>();
	}

private:
	static const uint retrieveNewClassId()
	{
		static uint lastClassId = 0;
		return lastClassId++;
	}

	template <typename, typename>
	friend class CastableTyped;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TTag, typename T>
class CastableTyped : public Castable<TTag>
{
protected:
	CastableTyped() : Castable<TTag>(getClassId()) {}

private:
	static const uint getClassId()
	{
		static uint classId = Castable<TTag>::retrieveNewClassId();
		return classId;
	}

	template <typename>
	friend class Castable;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TTag>
template <typename T>
const bool Castable<TTag>::isA() const
{
	return classId_ == CastableTyped<TTag, T>::getClassId();
}
	
///////////////////////////////////////////////////////////////////////////////

}

#endif