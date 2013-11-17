/*
 * Copyright (C) 2010 Yuriy Miroshnyk
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_dump_cout.h"

#include <iostream>
#include <string>

#include "fini_state_desc.h"

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

#if !FINI_NO_RTTI

static void shortDump(StateDesc& stateDesc, const std::string& tab = "")
{
	std::string defState = " ";
	if (StateDesc* parentStateDesc = stateDesc.getParentStateDesc())
	{
		if (parentStateDesc->getOrthoArea(stateDesc.getParentOrthoArea()).getDefaultState() == &stateDesc)
			defState = "*";
	}
	else
		defState = "^";

	std::cout << tab << 
		defState << stateDesc.getStateName() << " : " <<
		"(" << stateDesc.getStateDisplace() /*<< " + " << stateDesc.getInObjectDisp()*/ << ")[" << stateDesc.getStateSize() << "]" <<
		std::endl; 

	for (uint areaIndex = 0; areaIndex < stateDesc.getNumOrthoAreas(); ++areaIndex)
	{
		StateOrthoArea& area = stateDesc.getOrthoArea(areaIndex);

		std::cout << tab << " --" << areaIndex << std::endl;
		
		for (uint stateIndex = 0; stateIndex < area.getNumStates(); ++stateIndex)
			shortDump(area.getStateDesc(stateIndex), tab + "   ");
	}
}

///////////////////////////////////////////////////////////////////////////////

void dumpCout(StateDesc& stateDesc)
{
	std::cout << "Dumping state tree for '" << stateDesc.getStateName() << "':" << std::endl;
	std::cout << "  tree size = " <<stateDesc.computeStateTreeSize() << " bytes" << std::endl;

	shortDump(stateDesc, "  ");
}

#endif

///////////////////////////////////////////////////////////////////////////////
}