/*
 * Copyright (C) 2010-2012 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_STATE_DESC_H
#define _FINI_STATE_DESC_H

#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/optional.hpp>

#include "fini_setup.h"
#include "fini_state_ortho_area.h"
#include "fini_event_result.h"
#include "fini_event.h"
#include "fini_check.h"

namespace Fini
{

class StateBase;
class EventBase;
class Rule;

typedef StateBase& (*CreateStateFunc)(uint8* poolPtr);
typedef boost::optional<EventResult> (*ApplyEvent)(StateBase&, const EventBase&);

///////////////////////////////////////////////////////////////////////////////

class StateDesc : boost::noncopyable
{
	struct Parent
	{
		StateDesc* stateDesc_;
		uint orthoIndex_;

		Parent(StateDesc& stateDesc, uint orthoIndex) 
			: stateDesc_(&stateDesc)
			, orthoIndex_(orthoIndex) 
		{}
	};

	boost::optional<Parent> parent_;
	// todo: replace with boost::intrusive_list (see StateOrthoArea::instance<>)
	std::vector<StateOrthoArea> orthoAreas_;
	uint stateSize_;
	uint parametersSize_;
#if !FINI_NO_RTTI
	const char* stateName_;
#endif
	CreateStateFunc createStateFunc_;
	std::vector<ApplyEvent> reactions_;
	// todo: replace with boost::intrusive_list ?
	boost::ptr_vector<Rule> rules_;

	// optimization: computeStateTreeSize() REALLY impacted performance!
	mutable boost::optional<uint> stateTreeSize_;
	
protected:
	StateDesc();

public:
	// отложенная инициализация
	void deferredInit(CreateStateFunc createStateFunc, uint stateSize, uint parametersSize
#if !FINI_NO_RTTI
		, const char* stateName
#endif
		);

	uint getStateSize() const;

#if !FINI_NO_RTTI
	const char* getStateName() const;
#endif

	uint getNumOrthoAreas() const;
	StateOrthoArea& getOrthoArea(uint areaIndex);
	const StateOrthoArea& getOrthoArea(uint areaIndex) const;
	StateOrthoArea& createOrthoArea(uint areaIndex);

	StateDesc* getParentStateDesc() const;
	bool hasParentStateDesc(const StateDesc& parentStateDesc) const;
	uint getParentOrthoArea() const;
	void setParentStateDesc(StateDesc& parentStateDesc, uint parentOrthoIndex);
	uint computeStateTreeSize() const;
	uint getStateDisplace() const;
	uint getParametersSize() const;

	uint getDepth() const;

	inline bool hasChildState(const StateDesc& stateDesc) const { return getChildStateArea(stateDesc); }
	bool hasChildStateInsteadOf(const StateDesc& stateDesc, const StateDesc& insteadOf) const;
	boost::optional<uint> getChildStateArea(const StateDesc& stateDesc) const;

	template <typename TState>
	static StateDesc& instance()
	{
		static StateDesc stateDesc;
		return stateDesc;
	}

	StateBase& createState(uint8* poolPtr) const;

	void registerReaction(ApplyEvent reaction);
	uint getNumReactions() const;
	ApplyEvent getReaction(uint reactIndex) const;

	void registerRule(Rule& rule);
	uint getNumRules() const;
	const Rule& getRule(uint ruleIndex) const;

	static StateBase& getStateBaseFromStatePtr(uint8* statePtr);
	
	template <typename TState>
	static TState& getDerivedStateFromStatePtr(uint8* statePtr)
	{
		// TODO? (problems on GCC due to sometimes unomptimized empty base)
		return *reinterpret_cast<TState*>(statePtr + FINI_ALIGNED_SIZEOF(uint));
		/*StateBase *stateBase = reinterpret_cast<StateBase*>(statePtr + FINI_ALIGNED_SIZEOF(uint));
		return *(dynamic_cast<TState *>(stateBase));*/
	}

	static StateDesc*& currentRegisteringStateDesc()
	{
		static StateDesc* stateDesc = NULL;
		return stateDesc;
	}
};

///////////////////////////////////////////////////////////////////////////////

template <typename TState>
StateBase& createStateFunc(uint8* poolPtr)
{
	TState* const state = new(poolPtr + FINI_ALIGNED_SIZEOF(uint)) TState();

	const int stateBaseDispInt = reinterpret_cast<int>(static_cast<StateBase*>(state)) - 
		reinterpret_cast<int>(state);

	FINI_CHECK(stateBaseDispInt >= 0);

	*reinterpret_cast<uint*>(poolPtr) = uint(stateBaseDispInt) + FINI_ALIGNED_SIZEOF(uint);

	state->postConstruct(); // TODO: exception-safety?

	return *state;
}

///////////////////////////////////////////////////////////////////////////////

template <typename TState, typename TEvent>
boost::optional<EventResult> applyEvent(StateBase& state, const EventBase& evt)
{
	TState& stateTyped = static_cast<TState&>(state);
	
	if (const TEvent* eventTyped = evt.castTo<TEvent>())
		return stateTyped.react(*eventTyped);

	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////

template <typename TState, typename TEvent>
boost::optional<EventResult> deferEvent(StateBase& state, const EventBase& evt)
{
	TState& stateTyped = static_cast<TState&>(state);

	if (const TEvent* eventTyped = evt.castTo<TEvent>())
	{
		stateTyped.deferEvent(*eventTyped);
		return EventResult::createDiscarded();
	}

	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////
}

#endif