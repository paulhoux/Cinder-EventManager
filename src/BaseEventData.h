/// ============================================================================
//
//  EventDataBase.h
//
/// ============================================================================

#pragma once
#pragma warning( push )
#pragma warning( disable : 4068 )
#pragma GCC visibility push( default )
#pragma warning( pop )

#include <memory>

/// ============================================================================

namespace cinder {
	class Buffer;
}

/// ============================================================================

using EventDataRef = std::shared_ptr<class EventData>;
using EventType = uint64_t;
	
/// ============================================================================

class EventData {
public:
	explicit EventData( const float timestamp = 0.0f ) : mTimeStamp( timestamp ), mIsHandled( false ) {}
	virtual ~EventData() = default;

	EventData( const EventData &other ) = default;
	EventData &operator=( const EventData &other ) = delete;
	EventData( EventData &&other ) = default;
	EventData &operator=( EventData &&other ) = delete;
	
	[[nodiscard]] virtual const char* getName() const = 0;
	[[nodiscard]] virtual EventType getTypeId() const = 0;
	[[nodiscard]] float getTimeStamp() const { return mTimeStamp; }

	[[nodiscard]] bool isHandled() const { return mIsHandled; }
	void setIsHandled( bool handled = true ) { mIsHandled = handled; }
	
	virtual void serialize( cinder::Buffer &streamOut ) {}
	virtual void deSerialize( const cinder::Buffer &streamIn ) {}
	
	virtual EventDataRef copy() { return EventDataRef(); }
	
private:
	const float mTimeStamp;
	bool		mIsHandled;
};

/// ============================================================================

#pragma warning( push )
#pragma warning( disable : 4068 )
#pragma GCC visibility pop
#pragma warning( pop )

/// ============================================================================
