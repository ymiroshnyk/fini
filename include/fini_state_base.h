/*
 * Copyright (C) 2010-2012 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_STATE_BASE_H
#define _FINI_STATE_BASE_H

#include <boost/noncopyable.hpp>

#include "fini_setup.h"
#include "fini_event_result.h"
#include "fini_objects_queue.h"
#include "fini_state_desc.h"
#include "fini_fsm.h"

namespace Fini
{

class StateDesc;
class EventBase;
class Fsm;
///////////////////////////////////////////////////////////////////////////////

class StateBase : boost::noncopyable
{
	class EventPoster
	{
	public:
		virtual ~EventPoster() {}
		virtual void postEvent(StateBase& state) = 0;
	};

	template <typename TEvent>
	class EventPosterTyped : public EventPoster
	{
		TEvent event_;

	public:
		EventPosterTyped(const TEvent& evt)
			: event_(evt) {}

		virtual void postEvent(StateBase& state)
		{
			state.postEvent(event_);
		}
	};

	StateBase* parentState_;
	const StateDesc* stateDesc_;
	uint8* statePtr_;

	typedef ObjectsQueue<EventPoster> EventsQueue;
	EventsQueue deferredEvents_;

protected:
	StateBase();

	void initStateBase(StateDesc& stateDesc, uint8* thisPtr);

public:
	struct Parameters : boost::noncopyable
	{
	};

	virtual ~StateBase();

	EventResult processTreeEvent(const EventBase& evt);

	const StateBase* getContextState(const StateDesc& contextStateDesc) const;
	StateBase* getContextState(const StateDesc& contextStateDesc) { return const_cast<StateBase*>(const_cast<const StateBase*>(this)->getContextState(contextStateDesc)); }
	
	const StateBase* getContextState() const;
	StateBase* getContextState() { return const_cast<StateBase*>(const_cast<const StateBase*>(this)->getContextState()); }
	
	const StateDesc& getStateDesc() const;

	uint8* getStatePtr() const { return statePtr_; }

	Fsm& fsm();

	template <typename TContextState>
	const TContextState& context() const
	{
		const StateBase* contextState = getContextState(StateDesc::instance<TContextState>());
		return StateDesc::getDerivedStateFromStatePtr<TContextState>(contextState->getStatePtr());
	}

	template <typename TContextState>
	inline TContextState& context()
	{
		return const_cast<TContextState&>(const_cast<const StateBase*>(this)->context<TContextState>());
	}

	// postConstruct - called immediately after constructing a state.
	//                 FSM might still be unstable at this point.
	//                 Good for virtual calls during construction,
	//                 any results are accessible to child state constructors.
	//
	// postConstructStable - called for every constructed state,
	//                       just after the whole states subtree is constructed
	//                       (i.e. once FSM is guaranteed to be stable).
	//                       Good for processEvent during construction.
	//
	// preDestruct - called for every state that is about to be destructed,
	//               just before destructing ANY of the states in tree
	//               (i.e. FSM is still stable at this point)
	//               Good for virtual calls and/or processEvent during destruction.
	//
	// For states derived from abstract states,
	// both methods should call base versions if overridden.
	//
	// Calling order is the same as for constructors and destructors.

	void postConstruct() {}
	virtual void postConstructStable() {}
	virtual void preDestruct() {}

	static void reactions() {}
	static void rules() {}

	EventResult forwardEvent() { return EventResult::createForwarded(); }
	EventResult discardEvent() { return EventResult::createDiscarded(); }

	EventResult transit(const StateDesc& stateDesc);

	template <typename TTargetState>
	EventResult transit()
	{
		const StateDesc& transitParent = fsm().transit(*this, StateDesc::instance<TTargetState>());
		return EventResult::createTransited(transitParent);
	}

	void processEvent(const EventBase& evt);

	template <typename TEvent>
	void postEvent(const TEvent& evt)
	{
		fsm().postEvent(evt);
	}


	template <typename TEvent>
	void deferEvent(const TEvent& evt)
	{
		deferredEvents_.pushBack(EventPosterTyped<TEvent>(evt));
	}
};

///////////////////////////////////////////////////////////////////////////////
}

#endif