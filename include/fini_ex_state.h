/*
* Copyright (C) 2010 Yuriy Miroshnyk
* For conditions of distribution and use, see copyright notice in fini.h
*/

#ifndef _FINI_EX_STATE_H
#define _FINI_EX_STATE_H

#include "fini_setup.h"
#include "fini_state_base.h"
#include "fini_state_registrant.h"

namespace Fini
{
namespace Ex
{

///////////////////////////////////////////////////////////////////////////////
// applyEvent with dynamic_cast<>() for virtual inheritance

template <typename TState, typename TEvent>
boost::optional<EventResult> applyEvent(StateBase& state, const EventBase& evt)
{
	TState& stateTyped = dynamic_cast<TState&>(state);

	if (const TEvent* eventTyped = evt.castTo<TEvent>())
		return stateTyped.react(*eventTyped);

	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////
// deferEvent with dynamic_cast<>() for virtual inheritance

template <typename TState, typename TEvent>
boost::optional<EventResult> deferEvent(StateBase& state, const EventBase& evt)
{
	TState& stateTyped = dynamic_cast<TState&>(state);

	if (const TEvent* eventTyped = evt.castTo<TEvent>())
	{
		stateTyped.deferEvent(*eventTyped);
		return EventResult::createDiscarded();
	}

	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
class AbstractState : public virtual StateBase
{
public:
	AbstractState()
	{
		initStateBase(StateDesc::instance<TState>(),
			reinterpret_cast<uint8*>(static_cast<TState*>(this)));
	}

	template <typename TEvent>
	static void registerReact()
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerReaction(&applyEvent<TState, TEvent>);
	}

	template <typename TEvent>
	static void registerDeferral()
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerReaction(&Fini::Ex::deferEvent<TState, TEvent>);
	}

	template <typename TEvent, typename TFunctor>
	static void registerRule(const TFunctor& functor)
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerRule(*new RuleFunctor<TState, TEvent, TFunctor>(functor));
	}
};

///////////////////////////////////////////////////////////////////////////////

template <typename TParentState, typename TState, int orthoAreaIndex, bool defaultState>
class StateImpl : public AbstractState<TState>
{
public:
	typedef TParentState ParentState;
	typedef TState ThisState;

private:
	static StateRegistrant<TParentState, TState, orthoAreaIndex, defaultState> registrant_;

protected:
	StateImpl() 
	{
	}

public:
	virtual ~StateImpl() { registrant_.nop(); }
};

template <typename TParentState, typename TState, int orthoAreaIndex, bool defaultState>
StateRegistrant<TParentState, TState, orthoAreaIndex, defaultState> StateImpl<TParentState, TState, orthoAreaIndex, defaultState>::registrant_;

///////////////////////////////////////////////////////////////////////////////

template <typename TParentState, typename TState, int orthoAreaIndex = 0>
class State : public StateImpl<TParentState, TState, orthoAreaIndex, false>
{
};

///////////////////////////////////////////////////////////////////////////////

template <typename TParentState, typename TState, int orthoAreaIndex = 0>
class DefaultState : public StateImpl<TParentState, TState, orthoAreaIndex, true>
{
};

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
class TopState : public StateImpl<VoidParentState, TState, 0, false>
{
};

///////////////////////////////////////////////////////////////////////////////
}
}

#endif