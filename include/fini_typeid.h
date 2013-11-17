#pragma once

#include "fini_setup.h"
#include "fini_types.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////
#if FINI_NO_RTTI

class TypeInfo
{
	static uint lastId_;
	uint id_;
	
	TypeInfo(uint id) : id_(id) {}

public:
	inline bool operator == (const TypeInfo& other) const
	{
		return id_ == other.id_;
	}
	
	template <typename>
	friend TypeInfo typeId();
};

template <typename Type>
TypeInfo typeId()
{
	static uint id = TypeInfo::lastId_++;
	return TypeInfo(id);
}

#else

class TypeInfo
{
	const type_info* typeInfo_;
	
	TypeInfo(const type_info& typeInfo) : typeInfo_(&typeInfo) {}

public:
	bool operator == (const TypeInfo& other) const
	{
		return *typeInfo_ == *other.typeInfo_;
	}
	
	template <typename>
	friend TypeInfo typeId();
};

template <typename Type>
TypeInfo typeId()
{
	return TypeInfo(typeid(Type));
}

#endif
///////////////////////////////////////////////////////////////////////////////
}
