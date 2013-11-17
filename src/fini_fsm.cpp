/*
 * Copyright (C) 2010-2012 Yuriy Miroshnyk, Andrey Antsut
 * For conditions of distribution and use, see copyright notice in fini.h
 */

#include "fini_stdafx.h"

#include "fini_fsm.h"
#include "fini_state_desc.h"
#include "fini_check.h"
#include "fini_state_base.h"
#include "fini_event.h"

// Almost all of the following #define'd calculations are only needed to be done in FSM constructor,
// so there should be no noticeable overhead if the compiler does them few times more than necessary.
// DISP_STATE_TREE is also needed elsewhere, but it is constant and should be optimized by the compiler.

#define DISP_FSM_PTR			0
#define SIZEOF_FSM_PTR			FINI_ALIGNED_SIZEOF(Fini::Fsm*)
#define DISP_STATE_TREE			(DISP_FSM_PTR + SIZEOF_FSM_PTR)
#define SIZEOF_STATE_TREE		topStateDesc_->computeStateTreeSize()
#define DISP_PARAM_POOL			(DISP_STATE_TREE + SIZEOF_STATE_TREE)
#define SIZEOF_PARAM_POOL		computeSpaceForParameters(*topStateDesc_)
#define DISP_POST_EVENT_POOL	(DISP_PARAM_POOL + SIZEOF_PARAM_POOL)
#define POOL_SIZE				(SIZEOF_FSM_PTR + SIZEOF_STATE_TREE + SIZEOF_PARAM_POOL + (postEventPoolSize ? *postEventPoolSize * 2 : 0))

namespace Fini
{
///////////////////////////////////////////////////////////////////////////////

Fsm::Fsm(const StateDesc& topStateDesc, boost::optional<uint> postEventPoolSize)
: topStateDesc_(&topStateDesc)
, pool_(new uint8[POOL_SIZE])
, initiated_(false)
, unstableCount_(1)
, lastStableArea_(NULL)
, eventProcessing_(0)
, params_(ObjectsQueuePoolDesc(&pool_[DISP_PARAM_POOL], SIZEOF_PARAM_POOL))
, paramsNextClearSize_(0)
, postEventsQueue0_(
		postEventPoolSize ? 
			boost::optional<ObjectsQueuePoolDesc>(ObjectsQueuePoolDesc(&pool_[DISP_POST_EVENT_POOL], *postEventPoolSize)) 
			: boost::optional<ObjectsQueuePoolDesc>())
, postEventsQueue1_(
		postEventPoolSize ? 
			boost::optional<ObjectsQueuePoolDesc>(ObjectsQueuePoolDesc(&pool_[DISP_POST_EVENT_POOL + *postEventPoolSize], *postEventPoolSize)) 
			: boost::optional<ObjectsQueuePoolDesc>())
, postEventsQueueIndex_(0)
{
	// fsm can be initialized only with top states (without parent)
	FINI_CHECK(!topStateDesc.getParentStateDesc());

	*reinterpret_cast<Fsm**>(&pool_[DISP_FSM_PTR]) = this;
}

///////////////////////////////////////////////////////////////////////////////

Fsm::~Fsm()
{
	if (initiated_)
		terminate();
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::initiate()
{
	FINI_CHECK(!initiated_);
	//std::cout << std::endl << "initiate" << std::endl;
	constructState(NULL, *topStateDesc_, boost::none);

	FINI_CHECK(unstableCount_ == 1);
	--unstableCount_;

	initiated_ = true;

	postConstructAll(getState(*topStateDesc_), boost::none);
	
	params_.clear(0);

	lastStableArea_ = NULL;
	
	processPostedEvents();
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::terminate()
{
	FINI_CHECK(initiated_);
	//std::cout << std::endl << "terminate" << std::endl;

	StateBase& topState = getState(*topStateDesc_);
	predestructChildren(topState, boost::none);

	++unstableCount_;

	destructChildren(topState, boost::none);
	topState.~StateBase();
	lastStableArea_ = NULL;
	initiated_ = false;

	unstableCount_ = 1; // на всякий случай
}

///////////////////////////////////////////////////////////////////////////////

EventResult Fsm::processEvent(const Event& evt)
{
	// state machine is not fully created. you can't processEvent(). use postEvent()
	FINI_CHECK(initiated_ && !unstableCount_);

	const EventResult result = processEventPrivate(evt);

	if (eventProcessing_ == 0)
		processPostedEvents();

	return result;
}

///////////////////////////////////////////////////////////////////////////////

StateBase& Fsm::getState(const StateDesc& stateDesc)
{
	return StateDesc::getStateBaseFromStatePtr(&pool_[DISP_STATE_TREE + stateDesc.getStateDisplace()]);
}

///////////////////////////////////////////////////////////////////////////////

const StateBase& Fsm::getState(const StateDesc& stateDesc) const
{
	return StateDesc::getStateBaseFromStatePtr(&pool_[DISP_STATE_TREE + stateDesc.getStateDisplace()]);
}

///////////////////////////////////////////////////////////////////////////////

ObjectsQueue<Event>& Fsm::getPostEventQueue()
{
	FINI_CHECK(postEventsQueueIndex_ >=0 && postEventsQueueIndex_ <= 1);

	switch (postEventsQueueIndex_)
	{
	case 0: return postEventsQueue0_;
	default: return postEventsQueue1_;
	};
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::processPostedEvents()
{
	FINI_CHECK(initiated_);
	FINI_CHECK(!unstableCount_);

	// todo?: здесь при неправильном бросании postEvent может возникнуть вечный цикл
	while (!getPostEventQueue().empty())
	{
		ObjectsQueue<Event>& postEventsQueue = getPostEventQueue();
		postEventsQueueIndex_ = (postEventsQueueIndex_ + 1) % 2;

		for (ObjectsQueue<Event>::Iterator it(postEventsQueue); it; ++it)
		{
			processEventPrivate(*it);
		}

		postEventsQueue.clear(0);
	}
}

///////////////////////////////////////////////////////////////////////////////

EventResult Fsm::processEventPrivate(const Event& evt)
{
	eventProcessing_ += 1;
	const EventResult result = getState(*topStateDesc_).processTreeEvent(evt);

	FINI_CHECK(eventProcessing_ > 0);
	eventProcessing_ -= 1;

	return result;
}

///////////////////////////////////////////////////////////////////////////////

const StateDesc& Fsm::transit(StateBase& sourceState, const StateDesc& targetStateDesc)
{
	FINI_CHECK(!unstableCount_);

/*  This check would require storing of the pool size, which doesn't seem efficient for debugging only.
	Besides, if the states are in different FSM's, mutual parent search would fail anyway.

	FINI_CHECK(reinterpret_cast<uint8*>(&sourceState) > &pool_[0] &&
		reinterpret_cast<uint8*>(&sourceState) < &pool_[poolSize_]);
*/
	const StateDesc& sourceStateDesc = sourceState.getStateDesc();

	const StateDesc* mutualParent = getFirstMutualParentStateDesc(sourceStateDesc, targetStateDesc);

	// попытка перейти из главного стейта, в другую ортогональную область, либо в другую машину
	FINI_CHECK(mutualParent);

	StateBase* mutualParentState = sourceState.getContextState(*mutualParent);
	FINI_CHECK(mutualParentState);

	boost::optional<uint> areaOnly;
	uint sourceArea = *mutualParent->getChildStateArea(sourceStateDesc);
	uint targetArea = *mutualParent->getChildStateArea(targetStateDesc);
	if (sourceArea == targetArea)
		areaOnly.reset(sourceArea);

	// защита параметров от вложенных транзитов (из preDestruct / postConstruct)

	// TODO: никакая защита не спасёт психическое здоровье нашей FSM,
	// если вложенный транзит затронет ту же область, с которой мы сейчас работаем;
	// в этом конкретном случае нужно бросать ошибку, но как его отследить?

	const uint tmp = paramsNextClearSize_;
	paramsNextClearSize_ = params_.size();

	predestructChildren(*mutualParentState, areaOnly);

	++unstableCount_;

	destructChildren(*mutualParentState, areaOnly);
	constructState(mutualParentState, targetStateDesc, areaOnly);

	--unstableCount_;
	lastStableArea_ = NULL;

	postConstructAll(*mutualParentState, areaOnly);

	paramsNextClearSize_ = tmp;	
	params_.clear(paramsNextClearSize_);

	return *mutualParent;
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::constructState(StateBase* parentState, const StateDesc& targetStateDesc, boost::optional<uint> areaOnly)
{
	if (!parentState)
		recursiveConstruction(NULL, targetStateDesc, areaOnly);
	else
		recursiveConstruction(&parentState->getStateDesc(), targetStateDesc, areaOnly);
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::predestructChildren(StateBase& parentState, boost::optional<uint> areaOnly)
{
	const StateDesc& parentStateDesc = parentState.getStateDesc();
	const uint numAreas = parentStateDesc.getNumOrthoAreas();

	const uint startAreaIndex = areaOnly ? *areaOnly : numAreas - 1;
	const uint endAreaIndex = areaOnly ? *areaOnly : 0;
	
	for (int areaIndex = (int)startAreaIndex; areaIndex >= (int)endAreaIndex; --areaIndex)
	{
		const StateOrthoArea& area = parentStateDesc.getOrthoArea((uint)areaIndex);
		
		StateBase& childAreaState = StateDesc::getStateBaseFromStatePtr(
			parentState.getStatePtr() + parentStateDesc.getStateSize() + area.getIndexDisplace());

		predestructChildren(childAreaState, boost::none);

		childAreaState.preDestruct();
	}
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::destructChildren(StateBase& parentState, boost::optional<uint> areaOnly)
{
	const StateDesc& parentStateDesc = parentState.getStateDesc();
	const uint numAreas = parentStateDesc.getNumOrthoAreas();

	const uint startAreaIndex = areaOnly ? *areaOnly : numAreas - 1;
	const uint endAreaIndex = areaOnly ? *areaOnly : 0;
	
	for (int areaIndex = (int)startAreaIndex; areaIndex >= (int)endAreaIndex; --areaIndex)
	{
		const StateOrthoArea& area = parentStateDesc.getOrthoArea((uint)areaIndex);
		
		StateBase& childAreaState = StateDesc::getStateBaseFromStatePtr(
			parentState.getStatePtr() + parentStateDesc.getStateSize() + area.getIndexDisplace());

		destructChildren(childAreaState, boost::none);

		if (areaIndex > 0)
			lastStableArea_ = &parentStateDesc.getOrthoArea(areaIndex - 1);
		else
		{
			if (const StateDesc* parentParentStateDesc = parentStateDesc.getParentStateDesc())
			{
				lastStableArea_ = &parentParentStateDesc->getOrthoArea(parentStateDesc.getParentOrthoArea());
			}
			else
			{
				lastStableArea_ = NULL;
			}
		}

		childAreaState.~StateBase();
	}
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::recursiveConstruction(const StateDesc* parentStateDesc, const StateDesc& targetStateDesc, boost::optional<uint> areaOnly)
{
	// если есть родительский стейт, то он уже должен быть создан. просто берем указатель на его хук
	if (parentStateDesc)
	{
		const uint numAreas = parentStateDesc->getNumOrthoAreas();
		const uint startAreaIndex = areaOnly ? *areaOnly : 0;
		const uint endAreaIndex = areaOnly ? *areaOnly + 1 : numAreas;
	
		for (uint areaIndex = startAreaIndex; areaIndex < endAreaIndex; ++areaIndex)		
		{
			const StateOrthoArea& area = parentStateDesc->getOrthoArea(areaIndex);
			// у области должен быть стейт по умолчанию
			FINI_CHECK(area.getDefaultState());

			const StateDesc* stateToCreate = area.getDefaultState();

			const uint numStates = area.getNumStates();
			for (uint stateIndex = 0; stateIndex < numStates; ++stateIndex)
			{
				const StateDesc& stateDesc = area.getStateDesc(stateIndex);
				if (&stateDesc == &targetStateDesc || stateDesc.hasChildState(targetStateDesc))
				{
					stateToCreate = &area.getStateDesc(stateIndex);
					break;
				}
			}

			stateToCreate->createState(&pool_[DISP_STATE_TREE + stateToCreate->getStateDisplace()]);
			lastStableArea_ = &area;

			recursiveConstruction(stateToCreate, targetStateDesc, boost::none);
		}
	}
	else
	{
		topStateDesc_->createState(&pool_[DISP_STATE_TREE + topStateDesc_->getStateDisplace()]);
		recursiveConstruction(topStateDesc_, targetStateDesc, boost::none);
	}	
}

///////////////////////////////////////////////////////////////////////////////

void Fsm::postConstructAll(StateBase& parentState, boost::optional<uint> areaOnly)
{
	const StateDesc& parentStateDesc = parentState.getStateDesc();
	const uint numAreas = parentStateDesc.getNumOrthoAreas();

	const uint startAreaIndex = areaOnly ? *areaOnly : numAreas - 1;
	const uint endAreaIndex = areaOnly ? *areaOnly : 0;
	
	for (int areaIndex = (int)startAreaIndex; areaIndex >= (int)endAreaIndex; --areaIndex)
	{
		const StateOrthoArea& area = parentStateDesc.getOrthoArea((uint)areaIndex);
		
		StateBase& childAreaState = StateDesc::getStateBaseFromStatePtr(
			parentState.getStatePtr() + parentStateDesc.getStateSize() + area.getIndexDisplace());

		childAreaState.postConstructStable();

		postConstructAll(childAreaState, boost::none);
	}
}

///////////////////////////////////////////////////////////////////////////////

const StateDesc* Fsm::getFirstMutualParentStateDesc(const StateDesc& sourceStateDesc, const StateDesc& targetStateDesc)
{
	if (&sourceStateDesc == &targetStateDesc)
		return sourceStateDesc.getParentStateDesc();

	if (StateDesc* parentStateDesc = sourceStateDesc.getParentStateDesc())
	{
		if (parentStateDesc->hasChildStateInsteadOf(targetStateDesc, sourceStateDesc))
			return parentStateDesc;
		else
			return getFirstMutualParentStateDesc(*parentStateDesc, targetStateDesc);
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

uint Fsm::computeSpaceForParameters(const StateDesc& stateDesc) const
{
	uint orthoParametersSize = 0;

	const uint numOrthoAreas = stateDesc.getNumOrthoAreas();
	for (uint orthoIndex = 0; orthoIndex < numOrthoAreas; ++orthoIndex)
	{
		const StateOrthoArea& orthoArea = stateDesc.getOrthoArea(orthoIndex);

		uint childrenParametersSize = 0;
		const uint numStates = orthoArea.getNumStates();
		for (uint stateIndex = 0; stateIndex < numStates; ++stateIndex)
		{
			childrenParametersSize = std::max(childrenParametersSize, 
				computeSpaceForParameters(orthoArea.getStateDesc(stateIndex)));
		}

		orthoParametersSize += childrenParametersSize;
	}

	if (uint parametersSize = stateDesc.getParametersSize())
		// todo: here will better to use real ParameterKeeperTyped<> in computations
		return FINI_ALIGNED_SIZEOF(ParameterKeeperTyped<StateBase::Parameters>) + parametersSize + orthoParametersSize;
	else
		return orthoParametersSize;
}

///////////////////////////////////////////////////////////////////////////////

const StateBase* Fsm::recursiveStateCast(const StateDesc& stateDesc) const
{
	if (const StateDesc* parentStateDesc = stateDesc.getParentStateDesc())
	{
		if (const StateBase* parentState = recursiveStateCast(*parentStateDesc))
		{
			if (const StateDesc* parentParentStateDesc = parentStateDesc->getParentStateDesc())
			{
				if (lastStableArea_ == &parentParentStateDesc->getOrthoArea(parentStateDesc->getParentOrthoArea())) 
					return NULL;
			}

			const StateBase& state = getState(stateDesc);

			if (&state.getStateDesc() == &stateDesc)
				return &state;
			else
				return NULL;
		}
		else
			return NULL;
	}
	else
	{
		FINI_CHECK(&stateDesc == topStateDesc_);
		
		if (unstableCount_ && !lastStableArea_)
			return NULL;

		return &getState(stateDesc);
	}
}

///////////////////////////////////////////////////////////////////////////////
}
