// CoD2 Plus format, shared with OpenCoD2-Plus: docs/architecture/plus-protocol.md
#ifndef PLUS_FORMAT_H
#define PLUS_FORMAT_H

#define PLUS_TIME_OFFSET_BITS 11
#define PLUS_TIME_OFFSET_LIMIT ( 1u << PLUS_TIME_OFFSET_BITS )

// How far a time lies behind its snapshot. Unsigned, so that a time ahead of
// the snapshot comes out too large to send as an offset.
static inline unsigned Plus_TimeOffset( int snapshotTime, int time )
{
	return (unsigned)snapshotTime - (unsigned)time;
}

static inline int Plus_TimeFromOffset( int snapshotTime, unsigned offset )
{
	return (int)( (unsigned)snapshotTime - offset );
}

#endif
