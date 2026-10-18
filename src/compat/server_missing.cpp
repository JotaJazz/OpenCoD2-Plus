// CoD2rev-Plus functions that its code calls and this engine does not have.
// The bodies are CoD2rev-Plus's (the file named above each one); where it
// reaches into a module still not copied, the engine's public functions stand
// in.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "../server/server.h"
#include "../game/g_shared.h"
#include "server_decls.h"

// xanim/dobj.cpp
int DObjSetControlRotTransIndex( const DObj *obj, const int *partBits, int boneIndex )
{
	assert(obj);
	assert(&obj->skel->Mat);
	assert(boneIndex >= 0);
	assert(boneIndex < obj->numBones);

	int boneIndexHigh = boneIndex >> 5;
	int boneIndexLow = 1 << (boneIndex & 0x1F);

	if ( !( partBits[boneIndex >> 5] & boneIndexLow ) )
	{
		return 0;
	}

	DSkel_t *skel = obj->skel;

	if ( skel->partBits.skel[boneIndexHigh] & boneIndexLow )
	{
		return 0;
	}

	assert(!(skel->partBits.skel[boneIndexHigh] & boneIndexLow));
	skel->partBits.control[boneIndexHigh] |= boneIndexLow;
	skel->partBits.anim[boneIndexHigh] |= boneIndexLow;

	return 1;
}

// xanim/dobj.cpp
int DObjSetRotTransIndex( const DObj *obj, const int *partBits, int boneIndex )
{
	assert(obj);
	assert(&obj->skel->Mat);
	assert(boneIndex >= 0);
	assert(boneIndex < obj->numBones);

	int boneIndexHigh = boneIndex >> 5;
	int boneIndexLow = 1 << (boneIndex & 0x1F);

	if ( !( partBits[boneIndex >> 5] & boneIndexLow ) )
	{
		return 0;
	}

	DSkel_t *skel = obj->skel;

	if ( skel->partBits.skel[boneIndexHigh] & boneIndexLow )
	{
		return 0;
	}

	assert(!(skel->partBits.skel[boneIndexHigh] & boneIndexLow));
	skel->partBits.anim[boneIndexHigh] |= boneIndexLow;

	return 1;
}
