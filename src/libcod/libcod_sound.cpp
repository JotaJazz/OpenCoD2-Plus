#include "gsc.hpp"

#if defined(LIBCOD) && LIBCOD_COMPILE_SPEEX == 1

#include <speex/speex.h>

// Sounds played to players through the voice channel, as zk_libcod's
// loadSoundFile / playSoundFile. A sound is 8192 Hz mono 16-bit PCM, encoded
// into narrowband Speex frames of 160 samples, one frame per voice packet,
// which is what the game's voice chat uses.
//
// The encoding runs in a worker thread on a buffer of its own; the main
// thread installs the result when it collects it (Libcod_RunSoundCallbacks),
// so the sounds are only ever read and changed on the main thread.

#define MAX_CUSTOM_SOUNDS 64
#define MAX_CUSTOM_SOUND_SECONDS 600
#define SOUND_SAMPLE_RATE 8192
#define SOUND_FRAME_SAMPLES 160
#define SOUND_PACKET_BYTES 64		// a narrowband frame takes up to 62
#define SOUND_PACKETS_PER_SECOND ((float)SOUND_SAMPLE_RATE / SOUND_FRAME_SAMPLES)
#define MAX_SOUND_PACKETS ((int)(MAX_CUSTOM_SOUND_SECONDS * SOUND_PACKETS_PER_SECOND))

enum
{
	SOUND_LOAD_OK,
	SOUND_LOAD_TOO_LONG,
	SOUND_LOAD_NOT_FOUND,
	SOUND_LOAD_READ_ERROR
};

typedef struct
{
	byte size;
	byte data[SOUND_PACKET_BYTES];
} soundPacket_t;

typedef struct
{
	soundPacket_t *packets;
	int numPackets;
} customSound_t;

static customSound_t customSounds[MAX_CUSTOM_SOUNDS];
static int lastAutoSoundIndex;

typedef struct
{
	char path[MAX_OSPATH];
	float volume;
	int index;
	int callback;
	unsigned int levelId;
	// filled by the worker
	int result;
	soundPacket_t *packets;
	int numPackets;
} soundJob_t;

#define MAX_SOUND_JOBS 64
static soundJob_t *finishedSoundJobs[MAX_SOUND_JOBS];
static int numFinishedSoundJobs;
static int runningSoundJobs;

extern dvar_t *sv_voiceQuality;

// "<fs_homepath>/<path>", for a path that stays in fs_homepath; as in
// zk_libcod a leading "/" is still relative to it
static bool Libcod_SoundPath(const char *rel, char *out, int size)
{
	while (*rel == '/' || *rel == '\\')
		rel++;

	if (!rel[0] || strchr(rel, ':') || strstr(rel, ".."))
		return false;

	const char *home = Dvar_GetString("fs_homepath");
	if (!home || strlen(home) + strlen(rel) + 2 >= (size_t)size)
		return false;

	Com_sprintf(out, size, "%s/%s", home, rel);
	return true;
}

// Skips a WAV header if there is one; a WAV must be 8192 Hz, mono, 16-bit.
// dataSize gets the bytes of samples that follow, UINT_MAX for raw PCM.
static bool Libcod_SkipWavHeader(FILE *file, unsigned int *dataSize)
{
	byte header[12];

	*dataSize = UINT_MAX;

	if (fread(header, 1, 12, file) != 12 || memcmp(header, "RIFF", 4) || memcmp(header + 8, "WAVE", 4))
	{
		// raw PCM
		fseek(file, 0, SEEK_SET);
		return true;
	}

	if (fseek(file, 0, SEEK_END))
		return false;
	long length = ftell(file);
	if (length < 0 || fseek(file, 12, SEEK_SET))
		return false;

	bool formatOk = false;
	for (;;)
	{
		byte chunk[8];
		long pos = ftell(file);
		if (pos < 0 || fread(chunk, 1, 8, file) != 8)
			return false;

		unsigned int size = chunk[4] | (chunk[5] << 8) | (chunk[6] << 16) | ((unsigned int)chunk[7] << 24);

		if (!memcmp(chunk, "data", 4))
		{
			*dataSize = size;
			return formatOk;
		}

		// a chunk must end inside the file, so that each one moves forward
		long long next = (long long)pos + 8 + size + (size & 1);
		if (size > 0x7FFFFFFE || next > length)
			return false;

		if (!memcmp(chunk, "fmt ", 4))
		{
			byte fmt[16];
			if (size < 16 || fread(fmt, 1, 16, file) != 16)
				return false;
			int format = fmt[0] | (fmt[1] << 8);
			int channels = fmt[2] | (fmt[3] << 8);
			int rate = fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | (fmt[7] << 24);
			int bits = fmt[14] | (fmt[15] << 8);
			formatOk = format == 1 && channels == 1 && rate == SOUND_SAMPLE_RATE && bits == 16;
		}

		if (fseek(file, (long)next, SEEK_SET))
			return false;
	}
}

static void *Libcod_EncodeSound(void *arg)
{
	soundJob_t *job = (soundJob_t *)arg;
	FILE *file = fopen(job->path, "rb");
	unsigned int dataSize;

	if (!file)
	{
		job->result = SOUND_LOAD_NOT_FOUND;
	}
	else if (!Libcod_SkipWavHeader(file, &dataSize))
	{
		job->result = SOUND_LOAD_READ_ERROR;
		fclose(file);
	}
	else
	{
		void *encoder = speex_encoder_init(&speex_nb_mode);
		SpeexBits bits;
		int rate = SOUND_SAMPLE_RATE, quality = sv_voiceQuality->current.integer;
		int capacity = 0;

		speex_bits_init(&bits);
		speex_encoder_ctl(encoder, SPEEX_SET_SAMPLING_RATE, &rate);
		speex_encoder_ctl(encoder, SPEEX_SET_QUALITY, &quality);

		job->result = SOUND_LOAD_OK;
		for (;;)
		{
			short in[SOUND_FRAME_SAMPLES];
			float frame[SOUND_FRAME_SAMPLES];
			// chunks after the samples (LIST, id3) are not sound
			size_t want = dataSize / sizeof(short) < SOUND_FRAME_SAMPLES ? dataSize / sizeof(short) : SOUND_FRAME_SAMPLES;
			size_t read = want ? fread(in, sizeof(short), want, file) : 0;

			if (!read)
				break;
			dataSize -= read * sizeof(short);

			if (job->numPackets == MAX_SOUND_PACKETS)
			{
				job->result = SOUND_LOAD_TOO_LONG;
				break;
			}

			if (job->numPackets == capacity)
			{
				capacity = capacity ? capacity * 2 : 1024;
				if (capacity > MAX_SOUND_PACKETS)
					capacity = MAX_SOUND_PACKETS;
				soundPacket_t *grown = (soundPacket_t *)realloc(job->packets, capacity * sizeof(soundPacket_t));
				if (!grown)
				{
					job->result = SOUND_LOAD_READ_ERROR;
					break;
				}
				job->packets = grown;
			}

			for (size_t i = 0; i < SOUND_FRAME_SAMPLES; i++)
				frame[i] = i < read ? in[i] * job->volume : 0;

			speex_bits_reset(&bits);
			speex_encode(encoder, frame, &bits);
			soundPacket_t *packet = &job->packets[job->numPackets++];
			packet->size = speex_bits_write(&bits, (char *)packet->data, SOUND_PACKET_BYTES);

			if (read < SOUND_FRAME_SAMPLES)
				break;
		}

		if (ferror(file))
			job->result = SOUND_LOAD_READ_ERROR;

		speex_bits_destroy(&bits);
		speex_encoder_destroy(encoder);
		fclose(file);
	}

	Sys_EnterCriticalSection(CRITSECT_SOUND);
	if (numFinishedSoundJobs < MAX_SOUND_JOBS)
	{
		finishedSoundJobs[numFinishedSoundJobs++] = job;
	}
	else
	{
		free(job->packets);
		delete job;
	}
	runningSoundJobs--;
	Sys_LeaveCriticalSection(CRITSECT_SOUND);
	return NULL;
}

static void Libcod_SetSound(int index, soundPacket_t *packets, int numPackets)
{
	free(customSounds[index].packets);
	customSounds[index].packets = packets;
	customSounds[index].numPackets = numPackets;
}

// Installs the sounds the workers finished and calls the scripts back: once
// per frame, on the main thread
void Libcod_RunSoundCallbacks()
{
	soundJob_t *jobs[MAX_SOUND_JOBS];
	int count;

	Sys_EnterCriticalSection(CRITSECT_SOUND);
	count = numFinishedSoundJobs;
	memcpy(jobs, finishedSoundJobs, count * sizeof(jobs[0]));
	numFinishedSoundJobs = 0;
	Sys_LeaveCriticalSection(CRITSECT_SOUND);

	for (int i = 0; i < count; i++)
	{
		soundJob_t *job = jobs[i];

		// a sound that is too long plays up to the limit
		if (job->result == SOUND_LOAD_OK || job->result == SOUND_LOAD_TOO_LONG)
		{
			Libcod_SetSound(job->index, job->packets, job->numPackets);
			job->packets = NULL;
		}

		// a callback of an older map's scripts no longer exists
		if (Scr_IsSystemActive() && job->levelId == libcodLevelGeneration)
		{
			stackPushInt(job->result);
			stackPushInt(job->index + 1);
			short ret = Scr_ExecThread(job->callback, 2);
			Scr_FreeThread(ret);
		}

		free(job->packets);
		delete job;
	}
}

// Sends each player its share of voice packets for this frame; a player
// talking pauses the sounds, as in zk_libcod
void Libcod_RunSounds()
{
	extern dvar_t *sv_voice;
	extern dvar_t *g_voiceChatTalkingDuration;

	if (sv_voice->current.boolean)
	{
		for (int i = 0; i < level.maxclients; i++)
		{
			gclient_t *client = &level.clients[i];
			int since = level.time - client->lastVoiceTime;
			if (client->sess.connected == CON_CONNECTED && since >= 0 && since < g_voiceChatTalkingDuration->current.integer)
				return;
		}
	}

	float share = SOUND_PACKETS_PER_SECOND * level.frameTime * 0.001f;

	for (int i = 0; i < sv_maxclients->current.integer; i++)
	{
		libcodClient_t *lc = &libcodClients[i];

		if (!lc->soundIndex || svs.clients[i].state < CS_CONNECTED)
			continue;

		// a player with voice chat off (cl_voice 0, or a low rate) gets none
		if (!SV_ClientWantsVoiceData(i))
		{
			lc->soundIndex = 0;
			if (Scr_IsSystemActive())
				Scr_Notify(&g_entities[i], scr_const.sound_file_stop, 0);
			continue;
		}

		customSound_t *sound = &customSounds[lc->soundIndex - 1];
		lc->soundPending += share;

		while (lc->soundPending >= 1.0f)
		{
			if (svs.clients[lc->soundTalker].state < CS_CONNECTED || lc->soundPacket >= sound->numPackets)
			{
				lc->soundIndex = 0;
				if (Scr_IsSystemActive())
					Scr_Notify(&g_entities[i], scr_const.sound_file_done, 0);
				break;
			}

			// a full queue would drop the packet: the rest waits for the
			// next frame
			if (svs.clients[i].voicePacketCount >= MAX_VOICE_PACKETS)
			{
				if (lc->soundPending > MAX_VOICE_PACKETS)
					lc->soundPending = MAX_VOICE_PACKETS;
				break;
			}

			VoicePacket_t packet;
			packet.dataSize = sound->packets[lc->soundPacket].size;
			memcpy(packet.data, sound->packets[lc->soundPacket].data, packet.dataSize);
			SV_QueueVoicePacket(lc->soundTalker, i, &packet);

			lc->soundPacket++;
			lc->soundPending -= 1.0f;
		}
	}
}

static int Libcod_GetSoundIndex(int param, const char *function)
{
	int index;

	if (!stackGetParamInt(param, &index))
		return -1;

	if (index < 1 || index > MAX_CUSTOM_SOUNDS)
	{
		stackError("%s() sound index must be 1 to %i", function, MAX_CUSTOM_SOUNDS);
		return 0;
	}
	return index;
}

// loadSoundFile(path, callback, [volume], [index]): encodes a sound in the
// background; callback(index, result) runs when it is ready (result 0 is
// success, 1 too long, 2 file not found, 3 read error). Returns the sound
// index, or undefined.
void gsc_utils_loadsoundfile()
{
	const char *path;
	int callback;
	float volume = 1.0f;
	int index;
	char osPath[MAX_OSPATH];

	if (!stackGetParamString(0, &path) || !stackGetParamFunction(1, &callback))
	{
		stackError("gsc_utils_loadsoundfile() needs a file path and a callback function");
		stackPushUndefined();
		return;
	}

	if (Scr_GetNumParam() > 2 && Scr_GetType(2) != VAR_UNDEFINED && !stackGetParamFloat(2, &volume))
	{
		stackError("gsc_utils_loadsoundfile() volume has a wrong type");
		stackPushUndefined();
		return;
	}
	volume = volume < 0 ? 0 : volume > 1 ? 1 : volume;

	if (Scr_GetNumParam() > 3)
	{
		index = Libcod_GetSoundIndex(3, "gsc_utils_loadsoundfile");
		if (index <= 0)
		{
			if (index < 0)
				stackError("gsc_utils_loadsoundfile() sound index has a wrong type");
			stackPushUndefined();
			return;
		}
	}
	else
	{
		index = lastAutoSoundIndex % MAX_CUSTOM_SOUNDS + 1;
		lastAutoSoundIndex = index;
	}

	if (!Libcod_SoundPath(path, osPath, sizeof(osPath)))
	{
		stackError("gsc_utils_loadsoundfile() the path must stay inside fs_homepath");
		stackPushUndefined();
		return;
	}

	FILE *file = fopen(osPath, "rb");
	if (!file)
	{
		stackError("gsc_utils_loadsoundfile() input file could not be opened");
		stackPushUndefined();
		return;
	}
	fclose(file);

	Sys_EnterCriticalSection(CRITSECT_SOUND);
	bool busy = runningSoundJobs >= MAX_SOUND_JOBS;
	if (!busy)
		runningSoundJobs++;
	Sys_LeaveCriticalSection(CRITSECT_SOUND);

	if (busy)
	{
		stackError("gsc_utils_loadsoundfile() too many sounds are being encoded");
		stackPushUndefined();
		return;
	}

	soundJob_t *job = new soundJob_t();
	I_strncpyz(job->path, osPath, sizeof(job->path));
	job->volume = volume;
	job->index = index - 1;
	job->callback = callback;
	job->levelId = libcodLevelGeneration;

	threadid_t thread;
	if (!Sys_CreateNewThread(Libcod_EncodeSound, &thread, job))
	{
		Sys_EnterCriticalSection(CRITSECT_SOUND);
		runningSoundJobs--;
		Sys_LeaveCriticalSection(CRITSECT_SOUND);
		delete job;
		stackError("gsc_utils_loadsoundfile() could not start the encoder");
		stackPushUndefined();
		return;
	}

	stackPushInt(index);
}

// Files of VoicePacket_t records (talker, 256 data bytes, size), as
// zk_libcod's saveSpeexFile writes them
void gsc_utils_loadspeexfile()
{
	const char *path;
	char osPath[MAX_OSPATH];

	int index = Libcod_GetSoundIndex(1, "gsc_utils_loadspeexfile");
	if (!stackGetParamString(0, &path) || index < 0)
	{
		stackError("gsc_utils_loadspeexfile() needs a file path and a sound index");
		stackPushUndefined();
		return;
	}
	if (!index)
	{
		stackPushUndefined();
		return;
	}

	if (!Libcod_SoundPath(path, osPath, sizeof(osPath)))
	{
		stackError("gsc_utils_loadspeexfile() the path must stay inside fs_homepath");
		stackPushUndefined();
		return;
	}

	FILE *file = fopen(osPath, "rb");
	if (!file)
	{
		stackPushBool(qfalse);
		return;
	}

	soundPacket_t *packets = NULL;
	int numPackets = 0, capacity = 0;
	VoicePacket_t record;
	bool ok = true;

	while (fread(&record, sizeof(record), 1, file) == 1)
	{
		// the size comes from the file: keep it to what a frame can be
		if (record.dataSize <= 0)
			break;
		if (record.dataSize > SOUND_PACKET_BYTES || numPackets == MAX_SOUND_PACKETS)
		{
			ok = false;
			break;
		}

		if (numPackets == capacity)
		{
			capacity = capacity ? capacity * 2 : 1024;
			soundPacket_t *grown = (soundPacket_t *)realloc(packets, capacity * sizeof(soundPacket_t));
			if (!grown)
			{
				ok = false;
				break;
			}
			packets = grown;
		}

		packets[numPackets].size = record.dataSize;
		memcpy(packets[numPackets].data, record.data, record.dataSize);
		numPackets++;
	}
	fclose(file);

	if (!ok || !numPackets)
	{
		free(packets);
		stackPushBool(qfalse);
		return;
	}

	Libcod_SetSound(index - 1, packets, numPackets);
	stackPushBool(qtrue);
}

void gsc_utils_savespeexfile()
{
	const char *path;
	char osPath[MAX_OSPATH];

	int index = Libcod_GetSoundIndex(0, "gsc_utils_savespeexfile");
	if (index < 0 || !stackGetParamString(1, &path))
	{
		stackError("gsc_utils_savespeexfile() needs a sound index and a file path");
		stackPushUndefined();
		return;
	}
	if (!index)
	{
		stackPushUndefined();
		return;
	}

	// as the other save functions: no files the server loads itself
	// Windows drops trailing dots and spaces, which would hide the extension
	size_t pathLen = strlen(path);
	if (pathLen && (path[pathLen - 1] == '.' || path[pathLen - 1] == ' '))
	{
		stackError("gsc_utils_savespeexfile() the file name must not end in a dot or a space");
		stackPushUndefined();
		return;
	}

	const char *ext = strrchr(path, '.');
	if (ext && (!Q_stricmp(ext, ".iwd") || !Q_stricmp(ext, ".cfg") || !Q_stricmp(ext, ".gsc") || !Q_stricmp(ext, ".so") || !Q_stricmp(ext, ".dll")))
	{
		stackError("gsc_utils_savespeexfile() refuses to write a %s file", ext);
		stackPushUndefined();
		return;
	}

	customSound_t *sound = &customSounds[index - 1];
	if (!sound->numPackets || !Libcod_SoundPath(path, osPath, sizeof(osPath)))
	{
		stackPushBool(qfalse);
		return;
	}

	FILE *file = fopen(osPath, "wb");
	if (!file)
	{
		stackPushBool(qfalse);
		return;
	}

	bool ok = true;
	for (int i = 0; i < sound->numPackets && ok; i++)
	{
		VoicePacket_t record;
		memset(&record, 0, sizeof(record));
		record.dataSize = sound->packets[i].size;
		memcpy(record.data, sound->packets[i].data, record.dataSize);
		ok = fwrite(&record, sizeof(record), 1, file) == 1;
	}
	fclose(file);
	stackPushBool(ok);
}

// getSoundFileDuration(path, [noLimit]): seconds a raw or WAV sound file
// lasts when played
void gsc_utils_getsoundfileduration()
{
	const char *path;
	int noLimit = 0;
	char osPath[MAX_OSPATH];

	if (!stackGetParamString(0, &path) || (Scr_GetNumParam() > 1 && !stackGetParamInt(1, &noLimit)))
	{
		stackError("gsc_utils_getsoundfileduration() needs a file path");
		stackPushUndefined();
		return;
	}

	if (!Libcod_SoundPath(path, osPath, sizeof(osPath)))
	{
		stackError("gsc_utils_getsoundfileduration() the path must stay inside fs_homepath");
		stackPushUndefined();
		return;
	}

	FILE *file = fopen(osPath, "rb");
	if (!file)
	{
		stackPushUndefined();
		return;
	}

	unsigned int dataSize;
	bool ok = Libcod_SkipWavHeader(file, &dataSize);
	long start = ftell(file);
	fseek(file, 0, SEEK_END);
	long bytes = ftell(file) - start;
	fclose(file);

	if (!ok || start < 0 || bytes < 0)
	{
		stackPushUndefined();
		return;
	}
	if ((unsigned long)bytes > dataSize)
		bytes = dataSize;

	float seconds = (float)(bytes / 2) / SOUND_SAMPLE_RATE;
	if (!noLimit && seconds > MAX_CUSTOM_SOUND_SECONDS)
		seconds = MAX_CUSTOM_SOUND_SECONDS;
	stackPushFloat(seconds);
}

// self playSoundFile(index, [offset seconds], [source player]): plays a
// loaded sound to this player as voice chat from the source player
void gsc_player_playsoundfile(scr_entref_t id)
{
	float offset = 0;
	int source = id.entnum;

	int index = Libcod_GetSoundIndex(0, "gsc_player_playsoundfile");
	if (index < 0 || (Scr_GetNumParam() > 1 && !stackGetParamFloat(1, &offset)) || (Scr_GetNumParam() > 2 && !stackGetParamInt(2, &source)))
	{
		stackError("gsc_player_playsoundfile() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}
	if (!index)
	{
		stackPushUndefined();
		return;
	}

	if (id.classnum != CLASS_NUM_ENTITY || id.entnum >= MAX_CLIENTS || source < 0 || source >= MAX_CLIENTS || svs.clients[source].state < CS_CONNECTED)
	{
		stackPushBool(qfalse);
		return;
	}

	int packet = (int)(offset * SOUND_PACKETS_PER_SECOND);
	if (packet < 0 || packet >= customSounds[index - 1].numPackets)
	{
		stackPushBool(qfalse);
		return;
	}

	libcodClient_t *lc = &libcodClients[id.entnum];
	lc->soundIndex = index;
	lc->soundTalker = source;
	lc->soundPacket = packet;
	lc->soundPending = 0;
	stackPushBool(qtrue);
}

void gsc_player_stopsoundfile(scr_entref_t id)
{
	if (id.classnum != CLASS_NUM_ENTITY || id.entnum >= MAX_CLIENTS)
	{
		stackError("gsc_player_stopsoundfile() entity %i is not a player", id.entnum);
		stackPushUndefined();
		return;
	}

	libcodClient_t *lc = &libcodClients[id.entnum];
	if (!lc->soundIndex)
	{
		stackPushBool(qfalse);
		return;
	}

	lc->soundIndex = 0;
	Scr_Notify(&g_entities[id.entnum], scr_const.sound_file_stop, 0);
	stackPushBool(qtrue);
}

// The sound index playing to this player, or 0
void gsc_player_isplayingsoundfile(scr_entref_t id)
{
	if (id.classnum != CLASS_NUM_ENTITY || id.entnum >= MAX_CLIENTS)
	{
		stackError("gsc_player_isplayingsoundfile() entity %i is not a player", id.entnum);
		stackPushUndefined();
		return;
	}

	stackPushInt(libcodClients[id.entnum].soundIndex);
}

void gsc_player_getremainingsoundfileduration(scr_entref_t id)
{
	if (id.classnum != CLASS_NUM_ENTITY || id.entnum >= MAX_CLIENTS)
	{
		stackError("gsc_player_getremainingsoundfileduration() entity %i is not a player", id.entnum);
		stackPushUndefined();
		return;
	}

	libcodClient_t *lc = &libcodClients[id.entnum];
	if (!lc->soundIndex)
	{
		stackPushFloat(0);
		return;
	}

	int left = customSounds[lc->soundIndex - 1].numPackets - lc->soundPacket;
	stackPushFloat(left > 0 ? left / SOUND_PACKETS_PER_SECOND : 0);
}

#endif
