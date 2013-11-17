/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_STATE_H
#define _FINI_STATE_H

#include "fini_setup.h"
#include "fini_state_base.h"
#include "fini_state_registrant.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

template <typename TParentState, typename TState, int orthoAreaIndex, bool defaultState>
class StateImpl : public StateBase
{
public:
	typedef TParentState ParentState;
	typedef TState ThisState;

private:
	static StateRegistrant<TParentState, TState, orthoAreaIndex, defaultState> registrant_;

protected:
	StateImpl()
	{
		initStateBase(StateDesc::instance<TState>(),
			reinterpret_cast<uint8*>(static_cast<TState*>(this)));
	}

public:
	template <typename TEvent>
	static void registerReact()
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerReaction(&Fini::applyEvent<TState, TEvent>);
	}

	template <typename TEvent>
	static void registerDeferral()
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerReaction(&Fini::deferEvent<TState, TEvent>);
	}

	template <typename TEvent, typename TFunctor>
	static void registerRule(const TFunctor& functor)
	{
		StateDesc* registeringState = StateDesc::currentRegisteringStateDesc();
		FINI_CHECK(registeringState);
		registeringState->registerRule(*new RuleFunctor<TState, TEvent, TFunctor>(functor));
	}

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

#endif