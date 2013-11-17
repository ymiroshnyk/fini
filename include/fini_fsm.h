/*
 * Copyright (C) 2010 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_FSM_H
#define _FINI_FSM_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>

#include "fini_setup.h"
#include "fini_objects_queue.h"
#include "fini_castable.h"
#include "fini_state_desc.h"

namespace Fini
{

class StateDesc;
class EventBase;
class StateBase;
class StateOrthoArea;
class ParameterKeeper;
///////////////////////////////////////////////////////////////////////////////
	
class Fsm : boost::noncopyable
{
	template <typename TParameters>
	class ParameterKeeperTyped 
		: public CastableTyped<ParameterKeeper, ParameterKeeperTyped<TParameters> >
	{
	public:
		TParameters params_;

		ParameterKeeperTyped(const TParameters& params) : params_(params) {}
	};

	const StateDesc* topStateDesc_;
	boost::scoped_array<uint8> pool_;
	bool initiated_;
	int unstableCount_;
	const StateOrthoArea* lastStableArea_;
	uint eventProcessing_;
	ObjectsQueue<Castable<ParameterKeeper> > params_;
	uint paramsNextClearSize_;
	ObjectsQueue<EventBase> postEventsQueue0_;
	ObjectsQueue<EventBase> postEventsQueue1_;
	uint postEventsQueueIndex_;

public:
	Fsm(const StateDesc& topStateDesc, boost::optional<uint> postEventPoolSize = boost::none);
	virtual ~Fsm();

	void initiate();
	void terminate();

	bool isInitiated() const { return initiated_; }
	bool isEventProcessing() const { return eventProcessing_ != 0; }

	EventResult processEvent(const EventBase& evt);

	template <typename TState>
	void setParams(const typename TState::Parameters& params);

	template <typename TState>
	boost::optional<const typename TState::Parameters&> getParams();

	template <typename TState>
	const TState* stateCast() const;

	template <typename TEvent>
	void postEvent(const TEvent& evt);

private:
	StateBase& getState(const StateDesc& stateDesc);
	const StateBase& getState(const StateDesc& stateDesc) const;

	ObjectsQueue<EventBase>& getPostEventQueue();

	void processPostedEvents();

	EventResult processEventPrivate(const EventBase& evt);

	const StateDesc& transit(StateBase& sourceState, const StateDesc& targetStateDesc);

	void constructState(StateBase* parentState, const StateDesc& targetStateDesc, boost::optional<uint> areaOnly);
	void predestructChildren(StateBase& parentState, boost::optional<uint> areaOnly);
	void destructChildren(StateBase& parentState, boost::optional<uint> areaOnly);

	void recursiveConstruction(const StateDesc* parentStateDesc, const StateDesc& targetStateDesc, boost::optional<uint> areaOnly);
	void postConstructAll(StateBase& parentState, boost::optional<uint> areaOnly);

	const StateDesc* getFirstMutualParentStateDesc(const StateDesc& sourceStateDesc, const StateDesc& targetStateDesc);

	uint computeSpaceForParameters(const StateDesc& stateDesc) const;

	const StateBase* recursiveStateCast(const StateDesc& stateDesc) const;

	friend class StateBase;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
void Fsm::setParams(const typename TState::Parameters& params)
{
	if (!params_.pushBack(ParameterKeeperTyped<typename TState::Parameters>(params)))
	{
		FINI_ERROR(errParametersPoolExceeded);
	}
}

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
boost::optional<const typename TState::Parameters&> Fsm::getParams()
{
	for (ObjectsQueue<Castable<ParameterKeeper> >::Iterator it(params_); it; ++it)
	{
		if (const ParameterKeeperTyped<typename TState::Parameters>* paramsTyped = 
			(*it).castTo<ParameterKeeperTyped<typename TState::Parameters> >())
		{
			return paramsTyped->params_;
		}
	}

	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////

template <typename TEvent>
void Fsm::postEvent(const TEvent& evt)
{
	if (!getPostEventQueue().pushBack(evt))
	{
		FINI_ERROR(errPostEventQueueExceeded);
	}
}

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
const TState* Fsm::stateCast() const
{
	if (const StateBase* state = recursiveStateCast(StateDesc::instance<TState>()))
		return dynamic_cast<const TState*>(state);
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
}

#endif