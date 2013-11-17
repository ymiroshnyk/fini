/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#ifndef _FINI_STATE_ORTHO_AREA_H
#define _FINI_STATE_ORTHO_AREA_H

#include <vector>
#include <boost/optional.hpp>

#include "fini_setup.h"
#include "fini_types.h"

namespace Fini
{
class StateDesc;
///////////////////////////////////////////////////////////////////////////////

class StateOrthoArea
{
	uint index_;
	StateDesc* stateDesc_;
	// todo: replace with boost::intrusive_list !
	std::vector<StateDesc*> states_;
	StateDesc* defaultState_;

	// optimization
	mutable boost::optional<uint> size_;

public:
	StateOrthoArea(uint index, StateDesc& stateDesc);

	uint getIndex() const;
	StateDesc& getContainingState() const;

	StateDesc* getDefaultState() const;
	void setDefaultState(StateDesc& stateDesc);

	uint getNumStates() const;
	StateDesc& getStateDesc(uint stateIndex);
	const StateDesc& getStateDesc(uint stateIndex) const;

	void registerState(StateDesc& stateDesc);
	uint computeSize() const;
	uint getIndexDisplace() const;

	// todo: instantiate ortho areas via singletons and connect them with StateDesc using intrusive_list
	/*
	template <typename TState, int index>
	static StateOrthoArea& instance()
	{
		static StateOrthoArea orthoArea;
		return orthoArea;
	}
	*/
};

///////////////////////////////////////////////////////////////////////////////
}

#endif