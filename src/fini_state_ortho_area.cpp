/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_state_ortho_area.h"

#include <algorithm>

#include "fini_check.h"
#include "fini_state_desc.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

StateOrthoArea::StateOrthoArea(uint index, StateDesc& stateDesc)
: index_(index)
, stateDesc_(&stateDesc)
, defaultState_(NULL)
{
	//std::cout << "+ ortho area to " << typeid(*stateDesc_).name() << std::endl;
}

///////////////////////////////////////////////////////////////////////////////

uint StateOrthoArea::getIndex() const
{
	return index_;
}

///////////////////////////////////////////////////////////////////////////////

StateDesc& StateOrthoArea::getContainingState() const
{
	return *stateDesc_;
}

///////////////////////////////////////////////////////////////////////////////

StateDesc* StateOrthoArea::getDefaultState() const
{
	return defaultState_;
}

///////////////////////////////////////////////////////////////////////////////

void StateOrthoArea::setDefaultState(StateDesc& stateDesc)
{
	// два стейта по умолчанию не может быть в одной области
	FINI_CHECK(!defaultState_);
	// стейт уже должен быть зарегистрирован в области
	FINI_CHECK(std::count(states_.begin(), states_.end(), &stateDesc) > 0);
	defaultState_ = &stateDesc;
}

///////////////////////////////////////////////////////////////////////////////

uint StateOrthoArea::getNumStates() const 
{ 
	return states_.size(); 
}

///////////////////////////////////////////////////////////////////////////////

StateDesc& StateOrthoArea::getStateDesc(uint stateIndex) 
{ 
	FINI_CHECK(stateIndex < states_.size());
	return  *states_[stateIndex];
}

///////////////////////////////////////////////////////////////////////////////

const StateDesc& StateOrthoArea::getStateDesc(uint stateIndex) const
{ 
	FINI_CHECK(stateIndex < states_.size());
	return  *states_[stateIndex];
}

///////////////////////////////////////////////////////////////////////////////

void StateOrthoArea::registerState(StateDesc& stateDesc)
{
	//std::cout << "+ state to area " << typeid(*stateDesc_).name() << " -> " << typeid(stateDesc).name() << std::endl;

	states_.push_back(&stateDesc);
	stateDesc.setParentStateDesc(*stateDesc_, index_);
}

///////////////////////////////////////////////////////////////////////////////

uint StateOrthoArea::computeSize() const
{
	if (!size_)
	{
		size_.reset(0);
		for (uint stateIndex = 0; stateIndex < states_.size(); ++stateIndex)
		{
			*size_ = std::max(*size_, states_[stateIndex]->computeStateTreeSize());
		}
	}
	return *size_;
}

///////////////////////////////////////////////////////////////////////////////

uint StateOrthoArea::getIndexDisplace() const
{
	uint disp = 0;

	for (uint areaIndex = 0; areaIndex < index_; ++areaIndex)
		disp += stateDesc_->getOrthoArea(areaIndex).computeSize();

	return disp;
}

///////////////////////////////////////////////////////////////////////////////
}