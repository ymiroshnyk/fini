/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_STATE_REGISTRANT_H
#define _FINI_STATE_REGISTRANT_H

#include "fini_setup.h"

namespace Fini
{

class VoidParentState;

///////////////////////////////////////////////////////////////////////////////

template <typename TParentState, typename TState, int orthoAreaIndex, bool defaultState>
class StateRegistrant : boost::noncopyable
{
public:
	StateRegistrant()
	{
		StateDesc& parentStateDesc = StateDesc::instance<TParentState>();
		StateOrthoArea& orthoArea = parentStateDesc.createOrthoArea(orthoAreaIndex);				
		StateDesc& stateDesc = StateDesc::instance<TState>();
		stateDesc.deferredInit(&createStateFunc<TState>, FINI_ALIGNED_SIZEOF(TState), 
			boost::is_same<typename TState::Parameters, StateBase::Parameters>() ? 0 : FINI_ALIGNED_SIZEOF(typename TState::Parameters)
#if !FINI_NO_RTTI
			,typeid(TState).name()
#endif
			);

		FINI_CHECK(!StateDesc::currentRegisteringStateDesc());
		StateDesc::currentRegisteringStateDesc() = &stateDesc;

		TState::reactions();
		TState::rules();

		StateDesc::currentRegisteringStateDesc() = NULL;
		
		orthoArea.registerState(stateDesc);

		// в переменную, чтобы не ворнинговало
		bool set = defaultState;
		if (set)
			orthoArea.setDefaultState(stateDesc);
	}

	inline void nop() {}
};

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
class StateRegistrant<VoidParentState, TState, 0, false> : boost::noncopyable
{
public:
	StateRegistrant()
	{
		StateDesc& stateDesc = StateDesc::instance<TState>();
		stateDesc.deferredInit(&createStateFunc<TState>, FINI_ALIGNED_SIZEOF(TState), 
			boost::is_same<typename TState::Parameters, StateBase::Parameters>() ? 0 : FINI_ALIGNED_SIZEOF(typename TState::Parameters)
#if !FINI_NO_RTTI
			,typeid(TState).name()
#endif
			);

		FINI_CHECK(!StateDesc::currentRegisteringStateDesc());
		StateDesc::currentRegisteringStateDesc() = &stateDesc;

		TState::reactions();
		TState::rules();

		StateDesc::currentRegisteringStateDesc() = NULL;
	}

	inline void nop() {}
};

///////////////////////////////////////////////////////////////////////////////
}

#endif