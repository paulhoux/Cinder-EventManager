//========================================================================
// EventManager.h : implements a multi-listener multi-sender event system
//
// Part of the GameCode4 Application
//
// GameCode4 is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 4th Edition" by Mike McShaffry and David
// "Rez" Graham, published by Charles River Media.
// ISBN-10: 1133776574 | ISBN-13: 978-1133776574
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the authors a favor and buy a new copy of the book - there are
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here:
//    http://www.amazon.com/gp/product/1133776574/ref=olp_product_details?ie=UTF8&me=&seller=
//
// There's a companion web site at http://www.mcshaffry.com/GameCode/
//
// The source code is managed and maintained through Google Code:
//    http://code.google.com/p/gamecode4/
//
// (c) Copyright 2012 Michael L. McShaffry and David Graham
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser GPL v3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
// http://www.gnu.org/licenses/lgpl-3.0.txt for more details.
//
// You should have received a copy of the GNU Lesser GPL v3
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//========================================================================

#pragma once
#pragma warning(push)
#pragma warning( disable : 4068 )
/* The classes below are exported */
#pragma GCC visibility push(default)
#pragma warning( pop )

/// ============================================================================

#include "EventManagerBase.h"

#include <vector>
#include <deque>
#include <map>
#include <array>
#include <list>
#include <atomic>
#include <mutex>

/// ============================================================================

const uint32_t NUM_QUEUES = 2u;
using EventManagerRef = std::shared_ptr<class EventManager>;
	
/// ============================================================================

class EventManager : public EventManagerBase {
	using EventListenerList = std::list<EventListenerDelegate>;
	using EventListenerMap	= std::map<EventType, EventListenerList>;
	using EventQueue		= std::deque<EventDataRef>;
	
public:
	static auto create( std::string name, bool setAsGlobal )
	{
		return EventManagerRef( new EventManager( std::move( name ), setAsGlobal ) );
	}
	
	~EventManager() override = default;;

	EventManager( const EventManager &other ) = delete;
	EventManager &operator=( const EventManager &other ) = delete;
	EventManager( EventManager &&other ) = delete;
	EventManager &operator=( EventManager &&other ) = delete;

	void cleanup();
	
	bool addListener( EventListenerDelegate eventDelegate, EventType type ) override;
	bool removeListener( EventListenerDelegate eventDelegate, EventType type ) override;
	
	bool triggerEvent( EventDataRef event ) override;
	bool queueEvent( EventDataRef event ) override;
	bool abortEvent( EventType type, bool allOfType ) override;
	
	bool addThreadedListener( EventListenerDelegate eventDelegate, EventType type ) override;
	bool removeThreadedListener( EventListenerDelegate eventDelegate, EventType type ) override;
	void removeAllThreadedListeners() override;
	bool triggerThreadedEvent( EventDataRef event ) override;
	
	bool update( uint64_t maxMillis = kINFINITE ) override;

private:
	explicit EventManager( std::string name, bool setAsGlobal );
	
	void consumeAfterListeners();
	
	std::mutex							mThreadedEventListenerMutex;

	EventListenerMap					mThreadedEventListeners,
										mEventListeners;
	
	std::array<EventQueue, NUM_QUEUES>  mQueues;
	uint32_t							mActiveQueue {};
	
	using ListenerQueue = std::vector<std::pair<EventType, EventListenerDelegate>>;
	ListenerQueue	mAddAfter,
					mRemoveAfter;
	
	bool			mFiringEvent {};
};

/// ============================================================================

/* The classes below are exported */
#pragma warning( push )
#pragma warning( disable : 4068 )
#pragma GCC visibility pop
#pragma warning( pop )

/// ============================================================================
