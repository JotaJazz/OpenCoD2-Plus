#include "../qcommon/qcommon.h"
#include "script_public.h"

scrMemTreeGlob_t scrMemTreeGlob;

/*
============
Scr_GetStringUsage
============
*/
int Scr_GetStringUsage()
{
	return scrMemTreeGlob.totalAllocBuckets;
}

/*
============
MT_DumpTree
============
*/
void MT_DumpTree()
{
	int totalBuckets, subTreeSize, nodeNum;

	Com_Printf("********************************\n");
	totalBuckets = scrMemTreeGlob.totalAllocBuckets;

	for ( nodeNum = 0; nodeNum <= MEMORY_NODE_BITS; nodeNum++ )
	{
		subTreeSize = MT_GetSubTreeSize(scrMemTreeGlob.head[nodeNum]);
		totalBuckets += subTreeSize << nodeNum;
		Com_Printf("%d subtree has %d * %d = %d free buckets\n", nodeNum, subTreeSize, 1 << nodeNum, subTreeSize << nodeNum);
	}

	Com_Printf("********************************\n");
	Com_Printf("********************************\n");
	Com_Printf("total memory alloc buckets: %d (%d instances)\n", scrMemTreeGlob.totalAllocBuckets, scrMemTreeGlob.totalAlloc);
	Com_Printf("total memory free buckets: %d\n", MEMORY_NODE_COUNT - 1 - scrMemTreeGlob.totalAllocBuckets);
	Com_Printf("********************************\n");
}

/*
============
MT_Init
============
*/
void MT_Init()
{
	int i;

	MT_InitBits();

	for ( i = 0; i <= MEMORY_NODE_BITS; i++ )
	{
		scrMemTreeGlob.head[i] = 0;
	}

	scrMemTreeGlob.nodes[0].prev = 0;
	scrMemTreeGlob.nodes[0].next = 0;

	for ( i = 0; i < MEMORY_NODE_BITS; i++ )
	{
		MT_AddMemoryNode(1 << i, i);
	}

	scrMemTreeGlob.totalAlloc = 0;
	scrMemTreeGlob.totalAllocBuckets = 0;
}

/*
============
MT_FinishForceAlloc
============
*/
void MT_FinishForceAlloc( byte *allocBits )
{
	for ( int nodeNum = 1; nodeNum < MEMORY_NODE_COUNT; nodeNum++ )
	{
		if ( !( allocBits[ nodeNum >> 3 ] >> ( nodeNum & 7 ) & 1 ) )
		{
			MT_SafeFreeIndex(nodeNum);
		}
	}

	Z_Free(allocBits);
}

/*
============
MT_Realloc
============
*/
int MT_Realloc( int oldNumBytes, int newNumbytes )
{
	return MT_GetSize(oldNumBytes) >= MT_GetSize(newNumbytes);
}

/*
============
MT_ForceAllocIndex
============
*/
void MT_ForceAllocIndex( byte *allocBits, unsigned int nodeNum, int numBytes )
{
	int size, newSize;

	size = MT_GetSize(numBytes);
	scrMemTreeGlob.totalAlloc++;

	newSize = 1 << size;
	scrMemTreeGlob.totalAllocBuckets += 1 << size;

	while ( newSize )
	{
		allocBits[ nodeNum >> 3 ] |= 1 << ( nodeNum & 7 );
		nodeNum++;
		newSize--;
	}
}

/*
============
MT_FreeIndex
============
*/
void MT_FreeIndex( unsigned int nodeNum, int numBytes )
{
	int size, lowBit;

	size = MT_GetSize(numBytes);
	assert(size >= 0 && size <= MEMORY_NODE_BITS);
	assert(nodeNum > 0 && nodeNum < MEMORY_NODE_COUNT);

	scrMemTreeGlob.totalAlloc--;
	scrMemTreeGlob.totalAllocBuckets -= 1 << size;

	while ( 1 )
	{
		assert(size <= MEMORY_NODE_BITS);
		lowBit = 1 << size;
		assert(nodeNum == (nodeNum & ~(lowBit - 1)));

		if ( size == MEMORY_NODE_BITS || !MT_RemoveMemoryNode(nodeNum ^ lowBit, size) )
		{
			break;
		}

		nodeNum &= ~lowBit;
		size++;
	}

	MT_AddMemoryNode(nodeNum, size);
}

/*
============
MT_AllocIndex
============
*/
unsigned short MT_AllocIndex( int numBytes )
{
	int size, newSize, nodeNum;

	size = MT_GetSize(numBytes);
	assert(size >= 0 && size <= MEMORY_NODE_BITS);

	for ( newSize = size; ; newSize++ )
	{
		if ( newSize > MEMORY_NODE_BITS )
		{
			MT_Error("MT_AllocIndex", numBytes);
			return 0;
		}

		nodeNum = scrMemTreeGlob.head[newSize];

		if ( scrMemTreeGlob.head[newSize] )
		{
			break;
		}
	}

	MT_RemoveHeadMemoryNode(newSize);

	while ( newSize != size )
	{
		newSize--;
		MT_AddMemoryNode(nodeNum + (1 << newSize), newSize);
	}

	scrMemTreeGlob.totalAlloc++;
	scrMemTreeGlob.totalAllocBuckets += 1 << size;

	return nodeNum;
}

/*
============
MT_Free
============
*/
void MT_Free( void *p, int numBytes )
{
	assert( ( (MemoryNode *)p - scrMemTreeGlob.nodes >= 0 && (MemoryNode *)p - scrMemTreeGlob.nodes < MEMORY_NODE_COUNT ) );
	MT_FreeIndex( (MemoryNode *)p - scrMemTreeGlob.nodes, numBytes );
}

/*
============
MT_Alloc
============
*/
void* MT_Alloc( int numBytes )
{
	return scrMemTreeGlob.nodes + MT_AllocIndex( numBytes );
}

/*
============
MT_InitForceAlloc
============
*/
byte *MT_InitForceAlloc()
{
	scrMemTreeGlob.totalAlloc = 0;
	scrMemTreeGlob.totalAllocBuckets = 0;

	return (byte *)Z_Malloc( 8192 );
}

/*
============
MT_GetScore
============
*/
int MT_GetScore( int num )
{
	char bits;

	assert(num != 0);

	union MTnum_t
	{
		int i;
		uint8_t b[4];
	};

	MTnum_t mtnum;

	mtnum.i = MEMORY_NODE_COUNT - num;
	assert(mtnum.i != 0);

	bits = scrMemTreeGlob.leftBits[mtnum.b[0]];

	if ( !mtnum.b[0] )
	{
		bits += scrMemTreeGlob.leftBits[mtnum.b[1]];
	}

	return mtnum.i - (scrMemTreeGlob.numBits[mtnum.b[1]] + scrMemTreeGlob.numBits[mtnum.b[0]]) + (1 << bits);
}

/*
==============
MT_InitBits
==============
*/
void MT_InitBits()
{
	char bits;
	int temp;
	int i;

	for (i = 0; i < 256; i++)
	{
		bits = 0;
		for (temp = i; temp; temp >>= 1)
		{
			if (temp & 1)
			{
				bits++;
			}
		}

		scrMemTreeGlob.numBits[i] = bits;
		for (bits = 8; i & ((1 << bits) - 1); bits--)
		{
		}

		scrMemTreeGlob.leftBits[i] = bits;
		bits = 0;
		for (temp = i; temp; temp >>= 1)
		{
			bits++;
		}
		scrMemTreeGlob.logBits[i] = bits;
	}
}

/*
==============
MT_GetSubTreeSize
==============
*/
int MT_GetSubTreeSize( int nodeNum )
{
	if ( !nodeNum )
	{
		return 0;
	}

	return MT_GetSubTreeSize(scrMemTreeGlob.nodes[nodeNum].next) + MT_GetSubTreeSize(scrMemTreeGlob.nodes[nodeNum].prev) + 1;
}

/*
==============
MT_RemoveHeadMemoryNode
==============
*/
void MT_RemoveHeadMemoryNode( int size )
{
	MemoryNode tempNodeValue;
	int oldNode;
	MemoryNode oldNodeValue;
	uint16_t* parentNode;
	int prevScore;
	int nextScore;

	assert(size >= 0 && size <= MEMORY_NODE_BITS);

	parentNode = &scrMemTreeGlob.head[size];
	oldNodeValue = scrMemTreeGlob.nodes[*parentNode];

	while (1)
	{
		if (!oldNodeValue.prev)
		{
			oldNode = oldNodeValue.next;
			*parentNode = oldNodeValue.next;
			if (!oldNode)
			{
				break;
			}
			parentNode = &scrMemTreeGlob.nodes[oldNode].next;
		}
		else
		{
			if (oldNodeValue.next)
			{
				prevScore = MT_GetScore(oldNodeValue.prev);
				nextScore = MT_GetScore(oldNodeValue.next);

				assert(prevScore != nextScore);

				if (prevScore >= nextScore)
				{
					oldNode = oldNodeValue.prev;
					*parentNode = oldNode;
					parentNode = &scrMemTreeGlob.nodes[oldNode].prev;
				}
				else
				{
					oldNode = oldNodeValue.next;
					*parentNode = oldNode;
					parentNode = &scrMemTreeGlob.nodes[oldNode].next;
				}
			}
			else
			{
				oldNode = oldNodeValue.prev;
				*parentNode = oldNode;
				parentNode = &scrMemTreeGlob.nodes[oldNode].prev;
			}
		}

		assert(oldNode != 0);

		tempNodeValue = oldNodeValue;
		oldNodeValue = scrMemTreeGlob.nodes[oldNode];
		scrMemTreeGlob.nodes[oldNode] = tempNodeValue;
	}
}

/*
==============
MT_RemoveMemoryNode
==============
*/
bool MT_RemoveMemoryNode( int oldNode, int size )
{
	MemoryNode tempNodeValue;
	int node;
	MemoryNode oldNodeValue;
	int nodeNum;
	uint16_t* parentNode;
	int prevScore;
	int nextScore;
	int level;

	assert(size >= 0 && size <= MEMORY_NODE_BITS);

	nodeNum = 0;
	level = MEMORY_NODE_COUNT;
	parentNode = &scrMemTreeGlob.head[size];
	for (node = *parentNode; node; node = *parentNode)
	{
		if (oldNode == node)
		{
			oldNodeValue = scrMemTreeGlob.nodes[oldNode];

			while (1)
			{
				if (oldNodeValue.prev)
				{
					if (oldNodeValue.next)
					{
						prevScore = MT_GetScore(oldNodeValue.prev);
						nextScore = MT_GetScore(oldNodeValue.next);

						assert(prevScore != nextScore);

						if (prevScore >= nextScore)
						{
							oldNode = oldNodeValue.prev;
							*parentNode = oldNodeValue.prev;
							parentNode = &scrMemTreeGlob.nodes[oldNodeValue.prev].prev;
						}
						else
						{
							oldNode = oldNodeValue.next;
							*parentNode = oldNodeValue.next;
							parentNode = &scrMemTreeGlob.nodes[oldNodeValue.next].next;
						}
					}
					else
					{
						oldNode = oldNodeValue.prev;
						*parentNode = oldNodeValue.prev;
						parentNode = &scrMemTreeGlob.nodes[oldNodeValue.prev].prev;
					}
				}
				else
				{
					oldNode = oldNodeValue.next;
					*parentNode = oldNodeValue.next;
					if (!oldNodeValue.next)
					{
						return true;
					}
					parentNode = &scrMemTreeGlob.nodes[oldNodeValue.next].next;
				}

				assert(oldNode != 0);

				tempNodeValue = oldNodeValue;
				oldNodeValue = scrMemTreeGlob.nodes[oldNode];
				scrMemTreeGlob.nodes[oldNode] = tempNodeValue;
			}
		}

		if (oldNode == nodeNum)
		{
			return false;
		}

		level >>= 1;
		if (oldNode >= nodeNum)
		{
			parentNode = &scrMemTreeGlob.nodes[node].next;
			nodeNum += level;
		}
		else
		{
			parentNode = &scrMemTreeGlob.nodes[node].prev;
			nodeNum -= level;
		}
	}

	return false;
}

/*
==============
MT_AddMemoryNode
==============
*/
void MT_AddMemoryNode( int newNode, int size )
{
	int node;
	int nodeNum;
	int newScore;
	uint16_t* parentNode;
	int level;
	int score;

	assert(size >= 0 && size <= MEMORY_NODE_BITS);

	parentNode = &scrMemTreeGlob.head[size];
	node = scrMemTreeGlob.head[size];
	if (scrMemTreeGlob.head[size])
	{
		newScore = MT_GetScore(newNode);
		nodeNum = 0;
		level = MEMORY_NODE_COUNT;
		do
		{
			assert(newNode != node);
			score = MT_GetScore(node);

			assert(score != newScore);

			if (score < newScore)
			{
				while (1)
				{
					assert(node == *parentNode);
					assert(node != newNode);

					*parentNode = newNode;
					scrMemTreeGlob.nodes[newNode] = scrMemTreeGlob.nodes[node];
					if (!node)
					{
						break;
					}
					level >>= 1;

					assert(node != nodeNum);

					if (node >= nodeNum)
					{
						parentNode = &scrMemTreeGlob.nodes[newNode].next;
						nodeNum += level;
					}
					else
					{
						parentNode = &scrMemTreeGlob.nodes[newNode].prev;
						nodeNum -= level;
					}
					newNode = node;
					node = *parentNode;
				}
				return;
			}
			level >>= 1;

			assert(newNode != nodeNum);

			if (newNode >= nodeNum)
			{
				parentNode = &scrMemTreeGlob.nodes[node].next;
				nodeNum += level;
			}
			else
			{
				parentNode = &scrMemTreeGlob.nodes[node].prev;
				nodeNum -= level;
			}

			node = *parentNode;
		}
		while (node);
	}

	*parentNode = newNode;
	scrMemTreeGlob.nodes[newNode].prev = 0;
	scrMemTreeGlob.nodes[newNode].next = 0;
}

/*
==============
MT_AddMemoryNode
==============
*/
void MT_SafeFreeIndex( unsigned int nodeNum )
{
	int oldNode, size, lowBit;

	for ( size = 0, oldNode = nodeNum; !MT_RemoveMemoryNode(oldNode, size); oldNode &= ~(1 << size), size++ )
	{
		if ( size == MEMORY_NODE_BITS )
		{
			size = 0;
			oldNode = nodeNum;

			while ( 1 )
			{
				lowBit = 1 << size;

				if ( size == MEMORY_NODE_BITS || !MT_RemoveMemoryNode(oldNode ^ lowBit, size) )
				{
					break;
				}

				oldNode &= ~lowBit;
				size++;
			}

			break;
		}
	}

	MT_AddMemoryNode(oldNode, size);
}

/*
==============
MT_Error
==============
*/
void MT_Error( const char *funcName, int numBytes )
{
	MT_DumpTree();
	Com_Printf("%s: failed memory allocation of %d bytes for script usage\n", funcName, numBytes);
	Scr_TerminalError("failed memory allocation for script usage");
}

/*
==============
MT_GetSize
==============
*/
int MT_GetSize( int numBytes )
{
	int numBuckets;

	assert(numBytes > 0);

	if ( numBytes > MEMORY_NODE_COUNT )
	{
		MT_Error("MT_GetSize: max allocation exceeded", numBytes);
		return 0;
	}

	numBuckets = (numBytes + 7) / 8 - 1;

	if ( numBuckets > 256 - 1 )
	{
		return scrMemTreeGlob.logBits[numBuckets >> 8] + 8;
	}

	return scrMemTreeGlob.logBits[numBuckets];
}

/*
==============
MT_GetRefByIndex
==============
*/
byte* MT_GetRefByIndex( int index )
{
	if ( index > MEMORY_NODE_COUNT )
	{
		MT_Error("MT_GetRefByIndex: out of bounds index", index);
		return NULL;
	}

	return (byte *)&scrMemTreeGlob.nodes[index];
}

/*
==============
MT_GetIndexByRef
==============
*/
int MT_GetIndexByRef( byte *p )
{
	int index = (MemoryNode *)p - scrMemTreeGlob.nodes;

	if ( index < 0 || index >= MEMORY_NODE_COUNT )
	{
		MT_Error("MT_GetIndexByRef: out of bounds ref", index);
		return 0;
	}

	return index;
}
