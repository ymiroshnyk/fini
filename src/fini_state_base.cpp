/*
 * Copyright (C) 2010-2012 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_state_base.h"
#include "fini_rule.h"
#include "fini_state_desc.h"
#include "fini_event.h"
#include "fini_check.h"
#include "fini_fsm.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

class RuleDefault : public Rule
{
	virtual bool isEventSuitable(const Event&) const 
	{ 
		return true; 
	}

	virtual boost::optional<uint> rule(StateBase& state, const Event&, uint orthoIndex) const
	{
		if (orthoIndex < state.getStateDesc().getNumOrthoAreas())
			return orthoIndex;
		else
			return boost::none;
	}
};

///////////////////////////////////////////////////////////////////////////////

StateBase::StateBase()
: parentState_(NULL)
, stateDesc_(NULL)
, statePtr_(NULL)
{
	
}

///////////////////////////////////////////////////////////////////////////////

void StateBase::initStateBase(StateDesc& stateDesc, uint8* thisPtr)
{
	FINI_CHECK(!stateDesc_ || statePtr_ <= thisPtr - FINI_ALIGNED_SIZEOF(uint));

	if (!stateDesc_)
	{
		stateDesc_ = &stateDesc;
		statePtr_ = thisPtr - FINI_ALIGNED_SIZEOF(uint);

		if (const StateDesc* parentStateDesc = stateDesc.getParentStateDesc())
		{
			FINI_CHECK(stateDesc.getStateDisplace() > parentStateDesc->getStateDisplace());

			parentState_ = &StateDesc::getStateBaseFromStatePtr(
				statePtr_ - (stateDesc.getStateDisplace() - parentStateDesc->getStateDisplace()));
		}
		else
			parentState_ = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////

StateBase::~StateBase()
{
	for (EventsQueue::Iterator it(deferredEvents_); it; ++it)
	{
		(*it).postEvent(*this);
	}
}

///////////////////////////////////////////////////////////////////////////////

EventResult StateBase::processTreeEvent(const Event& evt)
{
	FINI_CHECK(stateDesc_);

	// сохраним, на случай, если стейт удалится при транзите, а воспользоваться нужно будет
	const StateDesc& stateDesc = *stateDesc_;
	bool discarded = false;

	static RuleDefault ruleDefault;
	const Rule* selectedRule = &ruleDefault;

	const uint numRules = stateDesc.getNumRules();
	for (uint ruleIndex = 0; ruleIndex < numRules; ++ruleIndex)
	{
		const Rule& rule = stateDesc.getRule(ruleIndex);
		if (rule.isEventSuitable(evt))
		{
			selectedRule = &rule;
			break;
		}
	}

	for (uint iterationIndex = 0; ; ++iterationIndex)
	{
		boost::optional<uint> areaIndex = selectedRule->rule(*this, evt, iterationIndex);
		if (!areaIndex)
			break;

		const StateOrthoArea& area = stateDesc.getOrthoArea(*areaIndex);
		
		StateBase& childAreaState = StateDesc::getStateBaseFromStatePtr(
			statePtr_ + stateDesc.getStateSize() + area.getIndexDisplace());

		EventResult result = childAreaState.processTreeEvent(evt);

		if (const StateDesc* transitParent = result.getTransited())
		{
			if (stateDesc.hasParentStateDesc(*transitParent))
				return result;

			discarded = true;
		}
		else if (result.isDiscarded())
		{
			discarded = true;
		}
	}

	if (discarded)
		return EventResult::createDiscarded();
	else
	{
		for (uint reactIndex = stateDesc.getNumReactions(); reactIndex--; )
			if (boost::optional<EventResult> result = stateDesc.getReaction(reactIndex)(*this, evt))
			{
				if (result->isDiscarded() || result->getTransited())
					evt.discard();

				return *result;
			}

		return EventResult::createForwarded();
	}
}

///////////////////////////////////////////////////////////////////////////////

const StateBase* StateBase::getContextState(const StateDesc& contextStateDesc) const
{
	FINI_CHECK(stateDesc_);

	const StateBase* ctx = parentState_;
	while (ctx)
	{
		if (&ctx->getStateDesc() == &contextStateDesc)
			return ctx;
		ctx = ctx->parentState_;
	}
	return ctx;
}

///////////////////////////////////////////////////////////////////////////////

const StateBase* StateBase::getContextState() const 
{
	FINI_CHECK(stateDesc_);

	return parentState_; 
}

///////////////////////////////////////////////////////////////////////////////

const StateDesc& StateBase::getStateDesc() const 
{ 
	FINI_CHECK(stateDesc_);
	return *stateDesc_; 
}

///////////////////////////////////////////////////////////////////////////////

Fsm& StateBase::fsm()
{
	FINI_CHECK(stateDesc_);

	Fsm** fsmPtr = reinterpret_cast<Fsm**>(
		statePtr_ - stateDesc_->getStateDisplace() - FINI_ALIGNED_SIZEOF(Fsm*));

	return **fsmPtr;
}

///////////////////////////////////////////////////////////////////////////////

EventResult StateBase::transit(const StateDesc& stateDesc)
{
	const StateDesc& transitParent = fsm().transit(*this, stateDesc);
	return EventResult::createTransited(transitParent);
}

///////////////////////////////////////////////////////////////////////////////

void StateBase::processEvent(const Event& evt)
{
	fsm().processEvent(evt);
}

///////////////////////////////////////////////////////////////////////////////
}
