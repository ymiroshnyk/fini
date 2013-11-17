/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_OBJECTS_QUEUE_H
#define _FINI_OBJECTS_QUEUE_H

#include <vector>
#include <boost/optional.hpp>

#include "fini_types.h"
#include "fini_check.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

struct ObjectsQueuePoolDesc
{
	uint8* pool_;
	uint poolSize_;
	ObjectsQueuePoolDesc(uint8* pool, uint poolSize) : pool_(pool), poolSize_(poolSize) {}
};

///////////////////////////////////////////////////////////////////////////////

// todo: make copyable

template <typename TObjectBase, typename TLinkType = uint>
class ObjectsQueue : boost::noncopyable
{
public:
	class Iterator
	{
		ObjectsQueue* queue_;
		uint8* ptr_;
	public:
		Iterator(ObjectsQueue& queue) 
			: queue_(&queue)
			, ptr_(queue.size() == 0 ? NULL : queue.ptr(0)) 
		{
		}

		operator bool () const
		{
			return ptr_ && ptr_ < queue_->ptr(0) + queue_->size();
		}

		TObjectBase& operator * ()
		{
			FINI_CHECK(*this);
			return *reinterpret_cast<TObjectBase*>(ptr_ + FINI_ALIGNED_SIZEOF(TLinkType));
		}

		void operator ++()
		{
			FINI_CHECK(*this);
			ptr_ += *reinterpret_cast<TLinkType*>(ptr_) + FINI_ALIGNED_SIZEOF(TLinkType);
		}
	};

private:
	struct StaticPool
	{
		uint8* front_;
		uint capacity_;
		uint size_;

		StaticPool(uint8* front, uint capacity) : front_(front), capacity_(capacity), size_(0) {}
	};

	uint8 poolObject_[sizeof(StaticPool) > sizeof(std::vector<uint8>) ?
		FINI_ALIGNED_SIZEOF(StaticPool) : FINI_ALIGNED_SIZEOF(std::vector<uint8>)];
	bool staticallyPooled_;

public:
	ObjectsQueue(boost::optional<ObjectsQueuePoolDesc> poolDesc = boost::none) 
		: staticallyPooled_(poolDesc)
	{
		if (poolDesc)
			new (poolObject_) StaticPool(poolDesc->pool_, poolDesc->poolSize_);
		else
			new (poolObject_) std::vector<uint8>();
	}

	~ObjectsQueue()
	{
		clear(0);

		if (staticallyPooled_)
			reinterpret_cast<StaticPool*>(poolObject_)->~StaticPool();
		else
			reinterpret_cast<std::vector<uint8>*>(poolObject_)->~vector();
	}

	template <typename TObject>
	bool pushBack(const TObject& object)
	{
		const uint objectSize = FINI_ALIGNED_SIZEOF(TObject);
		const uint oldSize = size();
		const uint linkSize = FINI_ALIGNED_SIZEOF(TLinkType);

		// is TLinkType able to store object size?
		FINI_CHECK(objectSize <= std::numeric_limits<TLinkType>::max());
		
		if (staticallyPooled_ && (oldSize + objectSize + linkSize > capacity()))
			return false;

		resize(oldSize + objectSize + linkSize);
		*reinterpret_cast<TLinkType*>(ptr(oldSize)) = TLinkType(objectSize);
		new (reinterpret_cast<void*>(ptr(oldSize + linkSize))) TObject(object);

		return true;
	}

	bool empty() const
	{
		return size() == 0;
	}

	void clear(uint newSize)
	{
		FINI_CHECK(newSize <= size());

		uint i = 0;
		for (Iterator it(*this); it; ++it, ++i)
		{
			if (i >= newSize)
				(*it).~TObjectBase();
		}

		resize(newSize);
	}

	uint size() const
	{
		if (staticallyPooled_)
			return reinterpret_cast<const StaticPool*>(poolObject_)->size_;
		else
			return reinterpret_cast<const std::vector<uint8>*>(poolObject_)->size();
	}

private:
	uint8* ptr(uint displace)
	{
		if (staticallyPooled_)
			return reinterpret_cast<StaticPool*>(poolObject_)->front_ + displace;
		else
			return &(*reinterpret_cast<std::vector<uint8>*>(poolObject_))[displace];
	}

	uint capacity() const
	{
		if (staticallyPooled_)
			return reinterpret_cast<const StaticPool*>(poolObject_)->capacity_;
		else
			return reinterpret_cast<const std::vector<uint8>*>(poolObject_)->capacity();
	}

	void resize(uint size)
	{
		if (staticallyPooled_)
		{
			FINI_CHECK(size <= reinterpret_cast<StaticPool*>(poolObject_)->capacity_);
			reinterpret_cast<StaticPool*>(poolObject_)->size_ = size;
		}
		else
			reinterpret_cast<std::vector<uint8>*>(poolObject_)->resize(size);
	}

	friend class Iterator;
};

///////////////////////////////////////////////////////////////////////////////
}

#endif