/*
 * Copyright (C) 2010 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_state_desc.h"
#include "fini_check.h"
#include "fini_rule.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

StateDesc::StateDesc()
: stateSize_(0)
#if !FINI_NO_RTTI
, stateName_(NULL)
#endif
, parametersSize_(0)
, createStateFunc_(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////

void StateDesc::deferredInit(CreateStateFunc createStateFunc, uint stateSize, uint parametersSize
#if !FINI_NO_RTTI
, const char* stateName
#endif
)
{
	createStateFunc_ = createStateFunc;
	stateSize_ = sizeof(uint) + stateSize;
#if !FINI_NO_RTTI
	stateName_ = stateName;
#endif
	parametersSize_ = parametersSize;
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getStateSize() const 
{ 
	FINI_CHECK(stateSize_ > 0);
	return stateSize_; 
}

///////////////////////////////////////////////////////////////////////////////

#if !FINI_NO_RTTI
const char* StateDesc::getStateName() const 
{ 
	FINI_CHECK(stateName_);
	return stateName_; 
}
#endif

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getNumOrthoAreas() const 
{ 
	return orthoAreas_.size(); 
}

///////////////////////////////////////////////////////////////////////////////

StateOrthoArea& StateDesc::getOrthoArea(uint areaIndex)
{
	FINI_CHECK(areaIndex < orthoAreas_.size());
	return orthoAreas_[areaIndex];
}

///////////////////////////////////////////////////////////////////////////////

const StateOrthoArea& StateDesc::getOrthoArea(uint areaIndex) const
{
	FINI_CHECK(areaIndex < orthoAreas_.size());
	return orthoAreas_[areaIndex];
}

///////////////////////////////////////////////////////////////////////////////

StateOrthoArea& StateDesc::createOrthoArea(uint areaIndex)
{
	for (uint size = orthoAreas_.size(); size < areaIndex + 1; ++size)
		orthoAreas_.push_back(StateOrthoArea(size, *this));

	return getOrthoArea(areaIndex);
}

///////////////////////////////////////////////////////////////////////////////

StateDesc* StateDesc::getParentStateDesc() const
{ 
	return parent_ ? parent_->stateDesc_ : NULL; 
}

///////////////////////////////////////////////////////////////////////////////

bool StateDesc::hasParentStateDesc(const StateDesc& parentStateDesc) const
{
	if (parent_)
	{
		if (parent_->stateDesc_ == &parentStateDesc 
			|| parent_->stateDesc_->hasParentStateDesc(parentStateDesc))
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getParentOrthoArea() const
{
	FINI_CHECK(parent_);
	return parent_->orthoIndex_;
}

///////////////////////////////////////////////////////////////////////////////

void StateDesc::setParentStateDesc(StateDesc& parentStateDesc, uint parentOrthoIndex)
{
	FINI_CHECK(!parent_);
	parent_.reset(Parent(parentStateDesc, parentOrthoIndex));
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::computeStateTreeSize() const
{
	if (!stateTreeSize_)
	{
		uint size = getStateSize();
		uint areasSize = 0;

		for (uint areaIndex = 0; areaIndex < orthoAreas_.size(); ++areaIndex)
		{
			areasSize += orthoAreas_[areaIndex].computeSize();
		}

		stateTreeSize_.reset(size + areasSize);
	}
	return *stateTreeSize_;
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getStateDisplace() const
{
	if (!parent_)
		return 0;
	else
	{
		const uint displace = parent_->stateDesc_->getStateDisplace() +
			parent_->stateDesc_->getStateSize() +
			parent_->stateDesc_->getOrthoArea(parent_->orthoIndex_).getIndexDisplace();

		return displace;
	}
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getParametersSize() const
{
	//FINI_CHECK(parametersSize_ > 0);
	return parametersSize_;
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getDepth() const
{
	if (parent_)
		return parent_->stateDesc_->getDepth() + 1;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////

bool StateDesc::hasChildStateInsteadOf(const StateDesc& stateDesc, const StateDesc& insteadOf) const
{
	const StateOrthoArea& area = getOrthoArea(insteadOf.getParentOrthoArea());
	const uint numStates = area.getNumStates();
	for (uint stateIndex = 0; stateIndex < numStates; ++stateIndex)
	{
		const StateDesc& childStateDesc = area.getStateDesc(stateIndex);
		if (&childStateDesc == &insteadOf)
			continue;
		if (&childStateDesc == &stateDesc || childStateDesc.hasChildState(stateDesc))
			return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

boost::optional<uint> StateDesc::getChildStateArea(const StateDesc& stateDesc) const
{
	const uint numAreas = getNumOrthoAreas();
	for (uint areaIndex = 0; areaIndex < numAreas; ++areaIndex)
	{
		const StateOrthoArea& area = getOrthoArea(areaIndex);
		const uint numStates = area.getNumStates();
		for (uint stateIndex = 0; stateIndex < numStates; ++stateIndex)
		{
			const StateDesc& childStateDesc = area.getStateDesc(stateIndex);
			if (&childStateDesc == &stateDesc || childStateDesc.hasChildState(stateDesc))
				return areaIndex;
		}
	}
	return boost::none;
}

///////////////////////////////////////////////////////////////////////////////

StateBase& StateDesc::createState(uint8* poolPtr) const
{
	FINI_CHECK(createStateFunc_);
	return createStateFunc_(poolPtr);
}

///////////////////////////////////////////////////////////////////////////////

void StateDesc::registerReaction(ApplyEvent reaction)
{
	reactions_.push_back(reaction);
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getNumReactions() const
{
	return (uint)reactions_.size();
}

///////////////////////////////////////////////////////////////////////////////

ApplyEvent StateDesc::getReaction(uint reactIndex) const
{
	return reactions_[reactIndex];
}

///////////////////////////////////////////////////////////////////////////////

void StateDesc::registerRule(Rule& rule)
{
	rules_.push_back(&rule);
}

///////////////////////////////////////////////////////////////////////////////

uint StateDesc::getNumRules() const
{
	return rules_.size();
}

///////////////////////////////////////////////////////////////////////////////

const Rule& StateDesc::getRule(uint ruleIndex) const
{
	return rules_[ruleIndex];
}

///////////////////////////////////////////////////////////////////////////////

StateBase& StateDesc::getStateBaseFromStatePtr(uint8* statePtr)
{
	const uint displace = *reinterpret_cast<uint*>(statePtr);
	statePtr += displace;
	return *reinterpret_cast<StateBase*>(statePtr);
}

///////////////////////////////////////////////////////////////////////////////
}
