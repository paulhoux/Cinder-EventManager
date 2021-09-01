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

#include "EventManager.h"

#include <cinder/Log.h>

#include <cassert>
#include <algorithm>

/// ============================================================================

//#define LOG_EVENT( stream )	CI_LOG_I( stream )
#define LOG_EVENT( stream )	((void)0)

/// ============================================================================

using namespace std;
	
EventManager::EventManager( std::string name, bool setAsGlobal ) : 
	EventManagerBase( std::move( name ), setAsGlobal )
{
	CI_LOG_I( "Creating event manager" );
}

void EventManager::cleanup()
{
	CI_LOG_I( "cleanup <<<<<<<<<<" );
	
	mEventListeners.clear();
	mQueues[0].clear();
	mQueues[1].clear();
	mAddAfter.clear();
	mRemoveAfter.clear();
	{
		LOG_EVENT( "Removing all threaded events");
		std::lock_guard lock( mThreadedEventListenerMutex );
		mThreadedEventListeners.clear();
	}
	
	CI_LOG_I( ">>>>>>>>>> cleanup" );
}

bool EventManager::addListener( EventListenerDelegate eventDelegate, EventType type )
{
	LOG_EVENT( "ADDING delegate function for event type: " + to_string( type ) );

	if( mFiringEvent ) {
		LOG_EVENT( "WARNING: delegate function will be added after current queue has been processed" );
		mAddAfter.emplace_back( type, std::move( eventDelegate ) );
	}
	else {
		auto &eventDelegateList = mEventListeners[type];
		auto listenIt = eventDelegateList.begin();
		const auto end = eventDelegateList.end();
		while( listenIt != end ) {
			if( eventDelegate == (*listenIt) ) {
				LOG_EVENT( "WARNING: Attempting to double-register a delegate" );
				return false;
			}
			++listenIt;
		}
		eventDelegateList.emplace_back( std::move( eventDelegate ) );

		LOG_EVENT( "ADDED delegate for event type: " + to_string( type ) );
	}

	return true;
}
	
bool EventManager::removeListener( EventListenerDelegate eventDelegate, EventType type )
{
	LOG_EVENT( "REMOVING delegate function from event type: " + to_string( type ) );
	auto success = false;
	
	if( mFiringEvent ) {
		LOG_EVENT( "WARNING: delegate function will be removed after current event has been processed" );
		mRemoveAfter.emplace_back( type, std::move( eventDelegate ) );
	}
	else {
		if( auto found = mEventListeners.find( type ); found != mEventListeners.end() ) {
			auto &listeners = found->second;
			for( auto listIt = listeners.begin(); listIt != listeners.end(); ++listIt ) {
				if( eventDelegate == (*listIt) ) {
					listeners.erase( listIt );
					LOG_EVENT( "REMOVED delegate function from event type: " );
					success = true;
					break;
				}
			}
		}
	}

	return success;
}
	
bool EventManager::triggerEvent( EventDataRef event )
{
	LOG_EVENT( "TRIGGERING event: " + std::string( event->getName() ) );
	auto processed = false;
	const auto originalFiringEvent = mFiringEvent;
	mFiringEvent = true;

	if( const auto found = mEventListeners.find( event->getTypeId() ); found != mEventListeners.end() ) {
		const auto &eventListenerList = found->second;
		for( auto &listener : eventListenerList ) {
			LOG_EVENT( "SENDING event " + std::string( event->getName() ) + " to delegate." );
			listener( event );
			processed = true;
		}
	}
	mFiringEvent = originalFiringEvent;

	if( ! mFiringEvent )
		consumeAfterListeners();

	return processed;
}
	
bool EventManager::queueEvent( EventDataRef event )
{
	assert( mActiveQueue < NUM_QUEUES );
	
	// make sure the event is valid
	if( ! event )
		LOG_EVENT( "WARNING: Invalid event in queueEvent" );
	
	LOG_EVENT( "QUEUEING event: " + std::string( event->getName() ) );

	mQueues[mActiveQueue].emplace_back( std::move( event ) );
	LOG_EVENT( "QUEUED event: " + std::string( mQueues[mActiveQueue].back()->getName() ) );

	return true;
}
	
bool EventManager::abortEvent( EventType type, bool allOfType )
{
	assert( mActiveQueue >= 0 );
	assert( mActiveQueue > NUM_QUEUES );
	
	auto success = false;

	if( const auto found = mEventListeners.find( type ); found != mEventListeners.end() ) {
		auto & eventQueue = mQueues[mActiveQueue];
		auto eventIt = eventQueue.begin();
		const auto end = eventQueue.end();
		while( eventIt != end ) {
			
			if( (*eventIt)->getTypeId() == type ) {
				eventIt = eventQueue.erase(eventIt);
				success = true;
				if( ! allOfType )
					break;
			}
		}
	}
	
	return success;
}
	
bool EventManager::addThreadedListener( EventListenerDelegate eventDelegate, EventType type )
{
	std::lock_guard lock( mThreadedEventListenerMutex );
	
	auto &eventDelegateList = mThreadedEventListeners[type];
	for( auto &delegate : eventDelegateList ) {
		if( eventDelegate == delegate ) {
			LOG_EVENT( "WARNING: Attempting to double-register a delegate" );
			return false;
		}
	}

	eventDelegateList.emplace_back( std::move( eventDelegate ) );
	LOG_EVENT( "ADDED delegate for event type: " + to_string( type ) );

	return true;
}

bool EventManager::removeThreadedListener( EventListenerDelegate eventDelegate, EventType type )
{
	std::lock_guard lock( mThreadedEventListenerMutex );

	if( auto found = mThreadedEventListeners.find( type ); found != mThreadedEventListeners.end() ) {
		auto &listeners = found->second;
		for( auto listIt = listeners.begin(); listIt != listeners.end(); ++listIt ) {
			if( eventDelegate == (*listIt) ) {
				listeners.erase( listIt );
				LOG_EVENT( "REMOVED delegate function from event type: " << to_string( type ) );
				return true;
			}
		}
	}

	return false;
}

void EventManager::removeAllThreadedListeners()
{
	std::lock_guard lock( mThreadedEventListenerMutex );
	mThreadedEventListeners.clear();
}

bool EventManager::triggerThreadedEvent( EventDataRef event )
{
	std::lock_guard lock( mThreadedEventListenerMutex );
	
	auto processed = false;
	if( const auto found = mThreadedEventListeners.find( event->getTypeId() ); found != mThreadedEventListeners.end() ) {
		const auto &eventListenerList = found->second;
		for( auto &listener : eventListenerList ) {
			listener( event );
			processed = true;
		}
	}

	if( ! processed )
		LOG_EVENT( "WARNING: Triggering ThreadedEvent without a listener" );

	return processed;
}

void EventManager::consumeAfterListeners()
{
	if( ! mAddAfter.empty() ) {
		std::sort( mAddAfter.begin(), mAddAfter.end(),
				  []( const pair<EventType, EventListenerDelegate> &a,
					  const pair<EventType, EventListenerDelegate> &b ) {
					  return a.first < b.first;
				  });
		for( auto &event : mAddAfter )
			addListener( event.second, event.first );
		mAddAfter.clear();
	}

	if( ! mRemoveAfter.empty() ) {
		std::sort( mRemoveAfter.begin(), mRemoveAfter.end(),
				  []( const pair<EventType, EventListenerDelegate> &a,
					  const pair<EventType, EventListenerDelegate> &b ) {
					  return a.first < b.first;
				  });
		for( auto &event : mRemoveAfter )
			removeListener( event.second, event.first );
		mRemoveAfter.clear();
	}
}
	
bool EventManager::update( uint64_t maxMillis )
{
	static uint64_t currentMs = 0;
	currentMs = (1.0 / 60.0) * 1000;
	const auto maxMs = maxMillis == EventManager::kINFINITE ? EventManager::kINFINITE : currentMs + maxMillis;
	
	const int queueToProcess = mActiveQueue;
	mActiveQueue = ( mActiveQueue + 1 ) % NUM_QUEUES;
	mQueues[mActiveQueue].clear();

	if( static auto processNotify = false; ! processNotify ) {
		LOG_EVENT( "Processing Event Queue " + to_string( queueToProcess ) + "; " + to_string( mQueues[queueToProcess].size() ) + " events to process" );
		processNotify = true;
	}
	
	while( ! mQueues[queueToProcess].empty() ) {
		mFiringEvent = true;

		const auto event = mQueues[queueToProcess].front();
		mQueues[queueToProcess].pop_front();
		LOG_EVENT( "\t\tProcessing Event " + std::string( event->getName() ) );
		
		const auto &eventType = event->getTypeId();

		if( const auto found = mEventListeners.find( eventType ); found != mEventListeners.end() ) {
			const auto &eventListeners = found->second;
			LOG_EVENT( "\t\tFound " + to_string( eventListeners.size() ) + " delegates" );

			auto listIt = eventListeners.begin();
			const auto end = eventListeners.end();
			while( listIt != end ) {
				const auto listener = (*listIt);
				LOG_EVENT( "\t\tSending Event " + std::string( event->getName() ) + " to delegate" );
				listener( event );
				++listIt;
			}
		}

		mFiringEvent = false;
		consumeAfterListeners();

		//		currentMs = app::App::get()->getElapsedSeconds() * 1000;//Engine::getTickCount();
		if( maxMillis != EventManager::kINFINITE && currentMs >= maxMs ) {
			LOG_EVENT( "WARNING: Aborting event processing; time ran out" );
			break;
		}
	}

	const auto queueFlushed = mQueues[queueToProcess].empty();
	if( ! queueFlushed ) {
		while( ! mQueues[queueToProcess].empty() ) {
			auto event = mQueues[queueToProcess].back();
			mQueues[queueToProcess].pop_back();
			mQueues[mActiveQueue].emplace_front( std::move( event ) );
		}
	}
	
	return queueFlushed;
}

/// ============================================================================
