/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_RULE_H
#define _FINI_RULE_H

#include <boost/optional.hpp>

#include "fini_setup.h"
#include "fini_types.h"
#include "fini_state_base.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

class Rule
{
public:
	virtual ~Rule() {}
	virtual bool isEventSuitable(const EventBase& evt) const = 0;
	virtual boost::optional<uint> rule(StateBase& state, const EventBase& evt, uint iterationIndex) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TState, typename TEvent, typename TFunctor>
class RuleFunctor : public Rule
{
	TFunctor functor_;

public:
	RuleFunctor(TFunctor functor) : functor_(functor) {}

	virtual bool isEventSuitable(const EventBase& evt) const
	{
		return evt.isA<TEvent>();
	}

	virtual boost::optional<uint> rule(StateBase& state, const EventBase& evt, uint iterationIndex) const
	{
		//FINI_CHECK(&state.getStateDesc() == &StateDesc::instance<TState>());
		FINI_CHECK(evt.isA<TEvent>());

		// TODO? (problems on GCC due to sometimes unomptimized empty base)
		TState& stateTyped = *dynamic_cast<TState *>(&StateDesc::getStateBaseFromStatePtr(state.getStatePtr()));
		//TState& stateTyped = StateDesc::getDerivedStateFromStatePtr<TState>(state.getStatePtr());
		return functor_(stateTyped, *evt.castTo<TEvent>(), iterationIndex);
	}
};

///////////////////////////////////////////////////////////////////////////////
}

#endif