#include "gsc_utils.hpp"

#if LIBCOD_COMPILE_UTILS == 1

//thanks to riicchhaarrd/php
void gsc_utils_getarraykeys()
{
	unsigned int arrIndex;

	if (!stackGetParamObject(0, &arrIndex))
	{
		stackError("gsc_utils_getarraykeys() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int arraysize = GetArraySize(arrIndex);

	if (!arraysize)
	{
		stackError("gsc_utils_getarraykeys() got an empty or invalid array");
		stackPushUndefined();
		return;
	}

	unsigned int index = arrIndex;
	unsigned int name;

	stackPushArray();

	for (int i = 0; i < arraysize; i++)
	{
		index = FindNextSibling(index);
		name = GetVariableName(index);

		if (name < 0x10000)
		{
			stackPushString(SL_ConvertToString(name));
			stackPushArrayLast();
		}
	}
}

// Appends formatted text to a COD2_MAX_STRINGLENGTH buffer, truncating instead of overflowing it
static int gsc_utils_append(char *result, int num, const char *format, ...)
{
	va_list va;
	int room = COD2_MAX_STRINGLENGTH - num;
	int written;

	va_start(va, format);
	written = vsnprintf(result + num, room, format, va);
	va_end(va);

	if (written < 0)
		return num;

	if (written >= room)
		return COD2_MAX_STRINGLENGTH - 1;

	return num + written;
}

void gsc_utils_printf()
{
	char result[COD2_MAX_STRINGLENGTH];
	const char *str;

	if (!stackGetParams("s", &str))
	{
		stackError("gsc_utils_printf() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int param = 1; // maps to first %
	int len = strlen(str);
	int num = 0;

	for (int i = 0; i < len; i++)
	{
		if (str[i] == '%')
		{
			if (str[i + 1] == '%')
			{
				if (num < COD2_MAX_STRINGLENGTH - 1)
					result[num++] = '%';
				i++;
			}
			else
			{
				if(param >= Scr_GetNumParam())
					continue;

				switch (stackGetParamType(param))
				{
				case STACK_STRING:
					const char *tmp_str;
					stackGetParamString(param, &tmp_str); // no error checking, since we know it's a string
					num = gsc_utils_append(result, num, "%s", tmp_str);
					break;

				case STACK_VECTOR:
					float vec[3];
					stackGetParamVector(param, vec);
					num = gsc_utils_append(result, num, "(%.2f, %.2f, %.2f)", vec[0], vec[1], vec[2]);
					break;

				case STACK_FLOAT:
					float tmp_float;
					stackGetParamFloat(param, &tmp_float);
					num = gsc_utils_append(result, num, "%.3f", tmp_float); // need a way to define precision
					break;

				case STACK_INT:
					int tmp_int;
					stackGetParamInt(param, &tmp_int);
					num = gsc_utils_append(result, num, "%d", tmp_int);
					break;

				case STACK_INT64:
					long int tmp_int64;
					stackGetParamInt64(param, &tmp_int64);
					num = gsc_utils_append(result, num, "%ld", tmp_int64);
					break;
				}

				param++;
			}
		}
		else
			if (num < COD2_MAX_STRINGLENGTH - 1)
				result[num++] = str[i];
	}

	result[num] = '\0';
	Com_Printf("%s", result);
	stackPushBool(qtrue);
}

void gsc_utils_sprintf()
{
	char result[COD2_MAX_STRINGLENGTH];
	const char *str;

	if (!stackGetParams("s", &str))
	{
		stackError("gsc_utils_sprintf() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int param = 1; // maps to first %
	int len = strlen(str);
	int num = 0;

	for (int i = 0; i < len; i++)
	{
		if (str[i] == '%')
		{
			if (str[i + 1] == '%')
			{
				if (num < COD2_MAX_STRINGLENGTH - 1)
					result[num++] = '%';
				i++;
			}
			else
			{
				if(param >= Scr_GetNumParam())
					continue;

				switch (stackGetParamType(param))
				{
				case STACK_STRING:
					const char *tmp_str;
					stackGetParamString(param, &tmp_str); // no error checking, since we know it's a string
					num = gsc_utils_append(result, num, "%s", tmp_str);
					break;

				case STACK_VECTOR:
					float vec[3];
					stackGetParamVector(param, vec);
					num = gsc_utils_append(result, num, "(%.2f, %.2f, %.2f)", vec[0], vec[1], vec[2]);
					break;

				case STACK_FLOAT:
					float tmp_float;
					stackGetParamFloat(param, &tmp_float);
					num = gsc_utils_append(result, num, "%.3f", tmp_float); // need a way to define precision
					break;

				case STACK_INT:
					int tmp_int;
					stackGetParamInt(param, &tmp_int);
					num = gsc_utils_append(result, num, "%d", tmp_int);
					break;

				case STACK_INT64:
					long int tmp_int64;
					stackGetParamInt64(param, &tmp_int64);
					num = gsc_utils_append(result, num, "%ld", tmp_int64);
					break;
				}

				param++;
			}
		}
		else
			if (num < COD2_MAX_STRINGLENGTH - 1)
				result[num++] = str[i];
	}

	result[num] = '\0';
	stackPushString(result);
}

void gsc_utils_outofbandprint()
{
	const char *address;
	const char *msg;

	if (!stackGetParams("ss", &address, &msg))
	{
		stackError("gsc_utils_outofbandprint() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	netadr_t from;
	NET_StringToAdr(address, &from);
	NET_OutOfBandPrint(NS_SERVER, from, msg);
}

void gsc_utils_getAscii()
{
	const char *str;

	if ( ! stackGetParams("s", &str))
	{
		stackError("gsc_utils_getAscii() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!strlen(str))
	{
		stackError("gsc_utils_getAscii() string length is 0");
		stackPushUndefined();
		return;
	}

	stackPushInt(str[0]);
}

void gsc_utils_putchar()
{
	int val;

	if ( ! stackGetParams("i",  &val))
	{
		stackError("gsc_utils_putchar() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (val < -127 || val > 127)
	{
		stackError("gsc_utils_putchar() character index is out of range");
		stackPushUndefined();
		return;
	}

	char s[2];

	s[0] = val;
	s[1] = '\0';

	stackPushString( s );
}

void gsc_utils_toupper()
{
	const char *string;
	int i;
	char c;
	char tempString[1024];

	if ( ! stackGetParams("s", &string))
	{
		stackError("gsc_utils_toupper() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!strlen(string))
	{
		stackError("gsc_utils_toupper() string length is 0");
		stackPushUndefined();
		return;
	}

	// a longer string is cut to fit
	for ( i = 0; i < (int)sizeof(tempString) - 1 && string[i]; ++i )
	{
		c = toupper(string[i]);
		tempString[i] = c;
	}
	tempString[i] = 0;

	stackPushString( tempString );
}

void gsc_utils_system()
{
	const char *cmd;

	if ( ! stackGetParams("s",  &cmd))
	{
		stackError("gsc_utils_system() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushInt( system(cmd) );
}

static int starttime = time(NULL);
void gsc_utils_getserverstarttime()
{
	stackPushInt( starttime );
}

void gsc_utils_getsystemtime()
{
	time_t timer;
	stackPushInt( time(&timer) );
}

void gsc_utils_getlocaltime()
{
	time_t timer;
	struct tm *timeinfo;

	time(&timer);
	timeinfo = localtime(&timer);

	const char *timestring = asctime(timeinfo);
	char stripped_time[128];

	strncpy(stripped_time, timestring, sizeof(stripped_time));
	stripped_time[strlen(timestring) - 1] = '\0';

	stackPushString( stripped_time );
}

void gsc_utils_exponent()
{
	float basis;
	float exponent;

	if ( ! stackGetParams("ff", &basis, &exponent))
	{
		stackError("gsc_utils_exponent() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat( pow(basis, exponent) );
}

void gsc_utils_round()
{
	float val;

	if ( ! stackGetParams("f",  &val))
	{
		stackError("gsc_utils_round() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat( roundf(val * 100) / 100 );
}

void gsc_utils_file_link()
{
	const char *source, *dest;

	if ( ! stackGetParams("ss",  &source, &dest))
	{
		stackError("gsc_utils_file_link() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

#ifdef _WIN32
	stackPushInt( CreateHardLinkA(dest, source, NULL) != 0 );
#else
	stackPushInt( symlink(source, dest) == 0 );
#endif
}

void gsc_utils_file_unlink()
{
	const char *file;

	if ( ! stackGetParams("s",  &file))
	{
		stackError("gsc_utils_file_unlink() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int unlink_success = unlink(file) == 0;
	stackPushInt( unlink_success );
}

void gsc_utils_file_exists()
{
	const char *filename;

	if ( ! stackGetParams("s", &filename))
	{
		stackError("gsc_utils_file_exists() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int file_exists = access(filename, F_OK) != -1;
	stackPushInt( file_exists );
}

void gsc_utils_FS_LoadDir()
{
	const char *path, *dir;

	if ( ! stackGetParams("ss", &path, &dir))
	{
		stackError("gsc_utils_FS_LoadDir() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	FS_AddIwdFilesForGameDirectory(path, dir);
	stackPushBool(qtrue);
}

void gsc_utils_getType()
{
	if (Scr_GetNumParam() == 0)
	{
		stackError("gsc_utils_getType() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushString( stackGetParamTypeAsString(0) );
}

void gsc_utils_float()
{
	if (Scr_GetNumParam() == 0)
	{
		stackError("gsc_utils_float() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	switch (stackGetParamType(0))
	{
	case STACK_STRING:
		const char *asstring;
		stackGetParamString(0, &asstring);
		stackPushFloat( atof(asstring) );
		return;

	case STACK_FLOAT:
		float asfloat;
		stackGetParamFloat(0, &asfloat);
		stackPushFloat( asfloat );
		return;

	case STACK_INT:
		int asinteger;
		stackGetParamInt(0, &asinteger);
		stackPushFloat( float(asinteger) );
		return;

	case STACK_INT64:
		long int asinteger64;
		stackGetParamInt64(0, &asinteger64);
		stackPushFloat( float(asinteger64) );
		return;

	default:
		stackError("gsc_utils_float() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}
}

// True when one of the ;-separated commands in text changes or restarts the
// map, or shuts the server down
bool ContainsMapCommand(const char *text)
{
	static const char *commands[] = { "map", "devmap", "map_restart", "fast_restart", "map_rotate", "killserver", "quit" };
	const char *p = text;

	while (*p)
	{
		while (*p == ' ' || *p == '\t' || *p == ';' || *p == '\r' || *p == '\n')
			p++;

		for (size_t i = 0; i < ARRAY_COUNT(commands); i++)
		{
			int len = strlen(commands[i]);
			if (!I_strnicmp(p, commands[i], len) && (p[len] == 0 || p[len] == ' ' || p[len] == '\t' || p[len] == ';' || p[len] == '\r' || p[len] == '\n'))
				return true;
		}

		while (*p && *p != ';' && *p != '\r' && *p != '\n')
			p++;
	}

	return false;
}

void gsc_utils_ExecuteString()
{
	const char *str;

	if ( ! stackGetParams("s", &str))
	{
		stackError("gsc_utils_ExecuteString() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	// a map change frees the scripts that are running, this one included, so
	// it waits in the command buffer, which runs outside of any script
	if (ContainsMapCommand(str))
	{
		Cbuf_AddText(str);
		Cbuf_AddText("\n");
	}
	else
		Cmd_ExecuteString(str);

	stackPushBool(qtrue);
}

void gsc_utils_sendgameservercommand()
{
	int clientNum;
	const char *message;

	if ( ! stackGetParams("is", &clientNum, &message))
	{
		stackError("gsc_utils_sendgameservercommand() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	SV_GameSendServerCommand(clientNum, SV_CMD_CAN_IGNORE, message);
	stackPushBool(qtrue);
}

void gsc_utils_scandir()
{
	const char *dirname;
	char** files;
	int numfiles;
	int i;

	if ( ! stackGetParams("s", &dirname))
	{
		stackError("gsc_utils_scandir() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	files = Sys_ListFiles(dirname, "", 0, &numfiles, 0);

	if (!numfiles)
	{
		stackPushUndefined();
		return;
	}

	stackPushArray();

	for(i = 0; i < numfiles; i++)
	{
		stackPushString(files[i]);
		stackPushArrayLast();
	}

	Sys_FreeFileList(files);
}

void gsc_utils_fopen()
{
	FILE *file;
	const char *filename, *mode;

	if ( ! stackGetParams("ss", &filename, &mode))
	{
		stackError("gsc_utils_fopen() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	file = fopen(filename, mode);

	if (!file)
	{
		stackError("gsc_utils_fopen() returned a error");
		stackPushUndefined();
		return;
	}

	stackPushInt64((intptr_t)file);
}

void gsc_utils_fread()
{
	FILE *file;

	if ( ! stackGetParams("l", &file))
	{
		stackError("gsc_utils_fread() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!file)
	{
		stackError("gsc_utils_fread() returned a error");
		stackPushUndefined();
		return;
	}

	char buffer[256];
	int ret = fread(buffer, 1, 255, file);

	if ( ! ret)
	{
		stackPushUndefined();
		return;
	}

	buffer[ret] = '\0';
	stackPushString(buffer);
}

void gsc_utils_fwrite()
{
	FILE *file;
	const char *buffer;

	if ( ! stackGetParams("ls", &file, &buffer))
	{
		stackError("gsc_utils_fwrite() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!file)
	{
		stackError("gsc_utils_fwrite() returned a error");
		stackPushUndefined();
		return;
	}

	stackPushInt(fwrite(buffer, 1, strlen(buffer), file));
}

void gsc_utils_fclose()
{
	FILE *file;

	if ( ! stackGetParams("l", &file))
	{
		stackError("gsc_utils_fclose() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!file)
	{
		stackError("gsc_utils_fclose() returned a error");
		stackPushUndefined();
		return;
	}

	stackPushInt( fclose(file) );
}

void gsc_utils_fsize()
{
	FILE *file;

	if ( ! stackGetParams("l", &file))
	{
		stackError("gsc_utils_fsize() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!file)
	{
		stackError("gsc_utils_fsize() returned a error");
		stackPushUndefined();
		return;
	}

	struct stat buf;
	fstat(fileno(file), &buf);

	stackPushInt( buf.st_size );
}

void gsc_utils_ftime()
{
	FILE *file;

	if ( ! stackGetParams("l", &file))
	{
		stackError("gsc_utils_ftime() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!file)
	{
		stackError("gsc_utils_ftime() returned a error");
		stackPushUndefined();
		return;
	}

	struct stat buf;
	fstat(fileno(file), &buf);

	const char *timestring = ctime(&buf.st_mtime);
	char stripped_time[128];

	if (!timestring)
	{
		stackPushUndefined();
		return;
	}

	strncpy(stripped_time, timestring, sizeof(stripped_time));
	stripped_time[strlen(timestring) - 1] = '\0';

	stackPushString( stripped_time );
}

// http://code.metager.de/source/xref/RavenSoftware/jediacademy/code/game/g_utils.cpp#36
void gsc_G_FindConfigstringIndexOriginal()
{
	const char *name;
	int min, max, create;

	if ( ! stackGetParams("siii", &name, &min, &max, &create))
	{
		stackError("gsc_G_FindConfigstringIndexOriginal() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	// the search covers min + 1 to min + max - 1
	if (min < 0 || max < 1 || max > MAX_CONFIGSTRINGS - min)
	{
		stackError("gsc_G_FindConfigstringIndexOriginal() configstring index is out of range");
		stackPushUndefined();
		return;
	}

	stackPushInt( G_FindConfigstringIndex(name, min, max, create, "G_FindConfigstringIndex() from GSC") );
}

// simple version, without crash
void gsc_G_FindConfigstringIndex()
{
	const char *name;
	int min, max;

	if ( ! stackGetParams("sii", &name, &min, &max))
	{
		stackError("gsc_G_FindConfigstringIndex() one or more arguments is undefined or has a wrong type");
		return;
	}

	// the search covers min + 1 to min + max - 1
	if (min < 0 || max < 1 || max > MAX_CONFIGSTRINGS - min)
	{
		stackError("gsc_G_FindConfigstringIndex() configstring index is out of range");
		stackPushUndefined();
		return;
	}

	for (int i = 1; i < max; i++)
	{
		const char *curitem = SV_GetConfigstringConst(min + i);

		if ( ! *curitem)
			break;

		if ( ! strcasecmp(name, curitem))
		{
			stackPushInt(i + min);
			return;
		}
	}

	stackPushBool(qtrue);
}

void gsc_get_configstring()
{
	int index;

	if ( ! stackGetParams("i", &index))
	{
		stackError("gsc_get_configstring() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		stackError("gsc_get_configstring() configstring index is out of range");
		stackPushUndefined();
		return;
	}

	const char *string = SV_GetConfigstringConst(index);

	if ( ! *string )
		stackPushUndefined();
	else
		stackPushString(string);
}

void gsc_set_configstring()
{
	int index;
	const char *string;

	if ( ! stackGetParams("is", &index, &string))
	{
		stackError("gsc_set_configstring() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		stackError("gsc_set_configstring() configstring index is out of range");
		stackPushUndefined();
		return;
	}

	SV_SetConfigstring(index, string);
	stackPushBool(qtrue);
}

void gsc_utils_sqrt()
{
	float x;

	if ( ! stackGetParams("f", &x))
	{
		stackError("gsc_utils_sqrt() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(sqrt(x));
}

void gsc_utils_sqrtInv()
{
	float x;

	if ( ! stackGetParams("f", &x))
	{
		stackError("gsc_utils_sqrtInv() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(Q_rsqrt(x));
}

void gsc_make_localized_string()
{
	const char *str;

	if ( ! stackGetParams("s", &str))
	{
		stackError("gsc_make_localized_string() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushString(str);

	VariableValue *var;
	int param = 0;

	var = Scr_GetValue(param);
	var->type = STACK_LOCALIZED_STRING;
}

void gsc_utils_getlasttestclientnumber()
{
	extern int botport;
	stackPushInt(botport);
}

void gsc_utils_getmaxmsglen()
{
	stackPushInt(MAX_MSGLEN);
}

void gsc_utils_bullethiteffect()
{
	vec3_t origin;
	vec3_t normal;

	if ( ! stackGetParams("vv", &origin, &normal))
	{
		stackError("gsc_utils_bullethiteffect() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = G_TempEntity(origin, EV_SHOTGUN_HIT);
	entity->s.eventParm = DirToByte(normal);

	trace_t trace;

	vec3_t end_origin = { origin[0] - (normal[0] * 10), origin[1] - (normal[1] * 10), origin[2] - (normal[2] * 10) };
	G_TraceCapsule(&trace, origin, vec3_origin, vec3_origin, end_origin, 1023, 2065);

	entity->s.surfType = (trace.surfaceFlags >> 20) & 0x1F;

	stackPushBool(qtrue);
}

void gsc_utils_vectorscale()
{
	vec3_t vector;
	float scale;

	if ( ! stackGetParams("vf", &vector, &scale))
	{
		stackError("gsc_utils_vectorscale() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	vec3_t out;

	VectorScale(vector, scale, out);
	stackPushVector(out);
}

void gsc_utils_remove_file()
{
	const char *filename;

	if (!stackGetParams("s", &filename))
	{
		stackError("gsc_utils_remove_file() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushInt(remove( filename ));
}

// Runs the rcon command CodeCallback_RemoteCommand was called for
void gsc_utils_processremotecommand()
{
	if (!Scr_ProcessRemoteCommand())
	{
		stackError("gsc_utils_processremotecommand() must be called from CodeCallback_RemoteCommand, without delay, and at most once per rcon command");
		return;
	}
}

// Functions below are named and behave as in zk_libcod

// The text of a localized string reference (&"...")
void gsc_utils_makestring()
{
	if (Scr_GetNumParam() < 1 || Scr_GetType(0) != VAR_ISTRING)
	{
		stackError("gsc_utils_makestring() argument is not a localized string");
		stackPushUndefined();
		return;
	}

	stackPushString(Scr_GetIString(0));
}

// A string the client looks up in its localization files when it shows it
void gsc_utils_makeclientlocalizedstring()
{
	const char *str;
	char buf[MAX_STRING_CHARS];

	if (!stackGetParams("s", &str))
	{
		stackError("gsc_utils_makeclientlocalizedstring() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (strlen(str) > sizeof(buf) - 3)
	{
		stackError("gsc_utils_makeclientlocalizedstring() string is too long");
		stackPushUndefined();
		return;
	}

	Com_sprintf(buf, sizeof(buf), "\x14%s\x15", str);
	stackPushString(buf);
}

void gsc_utils_getcvarflags()
{
	const char *name;

	if (!stackGetParams("s", &name))
	{
		stackError("gsc_utils_getcvarflags() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	dvar_t *dvar = Dvar_FindVar(name);
	if (!dvar)
	{
		stackPushUndefined();
		return;
	}

	stackPushInt(dvar->flags);
}

// The name of a surface type number, as bulletTrace's "surfacetype" gives
void gsc_utils_getsurfacename()
{
	int type;

	if (!stackGetParams("i", &type))
	{
		stackError("gsc_utils_getsurfacename() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushString(Com_SurfaceTypeToName(type));
}

// Where north is on the compass, in degrees; the stock getNorthYaw reads it
void gsc_utils_setnorthyaw()
{
	float yaw;

	if (!stackGetParams("f", &yaw))
	{
		stackError("gsc_utils_setnorthyaw() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	SV_SetConfigstring(CS_NORTHYAW, va("%g", yaw));
	stackPushBool(qtrue);
}

// The map's brush models moved by script (doors, elevators and the like)
void gsc_utils_getmovers()
{
	Scr_MakeArray();
	for (int i = 0; i < level.num_entities; i++)
	{
		gentity_t *ent = &g_entities[i];
		if (ent->r.inuse && ent->s.eType == ET_SCRIPTMOVER && ent->r.bmodel)
		{
			stackPushEntity(ent);
			Scr_AddArray();
		}
	}
}

// Whether the next map change or map_restart keeps the players' pers data,
// as map_restart(true) does
void gsc_utils_getsavepersist()
{
	stackPushBool(G_GetSavePersist() != 0);
}

void gsc_utils_setsavepersist()
{
	int persist;

	if (!stackGetParams("i", &persist))
	{
		stackError("gsc_utils_setsavepersist() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	G_SetSavePersist(persist != 0);
	stackPushBool(qtrue);
}

// As printoutofband(address, message), but false for an address that does
// not resolve instead of sending nowhere
void gsc_utils_sendpacket()
{
	const char *address;
	const char *msg;
	netadr_t to;

	if (!stackGetParams("ss", &address, &msg))
	{
		stackError("gsc_utils_sendpacket() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!NET_StringToAdr(address, &to) || to.type == NA_BAD)
	{
		stackPushBool(qfalse);
		return;
	}

	NET_OutOfBandPrint(NS_SERVER, to, msg);
	stackPushBool(qtrue);
}

// Replaces "console: " in front of the say and tell console commands
void gsc_utils_setconsoleprefix()
{
	const char *prefix;

	if (!stackGetParams("s", &prefix))
	{
		stackError("gsc_utils_setconsoleprefix() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	I_strncpyz(consolePrefix, prefix, sizeof(consolePrefix));
	stackPushString(consolePrefix);
}

// The name for the test clients (bots) added from now on, instead of
// bot<number>
void gsc_utils_setnexttestclientname()
{
	const char *name;

	if (!stackGetParams("s", &name))
	{
		stackError("gsc_utils_setnexttestclientname() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int len = strlen(name);
	if (len < 1 || len >= MAX_NAME_LENGTH)
	{
		stackError("gsc_utils_setnexttestclientname() requires a name of 1-%i characters", MAX_NAME_LENGTH - 1);
		stackPushUndefined();
		return;
	}

	// the name goes into the bot's userinfo string
	for (int i = 0; i < len; i++)
	{
		if ((unsigned char)name[i] < ' ' || name[i] == '\\' || name[i] == '"' || name[i] == ';')
		{
			stackError("gsc_utils_setnexttestclientname() name contains a control character, \\, \" or ;");
			stackPushUndefined();
			return;
		}
	}

	I_strncpyz(testClientName, name, MAX_NAME_LENGTH);
	stackPushBool(qtrue);
}

void gsc_utils_resettestclientnaming()
{
	testClientName[0] = 0;
	stackPushBool(qtrue);
}

void gsc_utils_abs()
{
	float val;

	if (!stackGetParams("f", &val))
	{
		stackError("gsc_utils_abs() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(fabsf(val));
}

void gsc_utils_floor()
{
	float val;

	if (!stackGetParams("f", &val))
	{
		stackError("gsc_utils_floor() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(floorf(val));
}

void gsc_utils_ceil()
{
	float val;

	if (!stackGetParams("f", &val))
	{
		stackError("gsc_utils_ceil() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(ceilf(val));
}

void gsc_utils_pow()
{
	float base, exponent;

	if (!stackGetParams("ff", &base, &exponent))
	{
		stackError("gsc_utils_pow() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(powf(base, exponent));
}

void gsc_utils_atan2()
{
	float y, x;

	if (!stackGetParams("ff", &y, &x))
	{
		stackError("gsc_utils_atan2() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	stackPushFloat(atan2f(y, x));
}

// roundTo(value, precision): roundTo(1.234, 100) is 1.23
void gsc_utils_roundto()
{
	float val, precision;

	if (!stackGetParams("ff", &val, &precision) || precision == 0)
	{
		stackError("gsc_utils_roundto() arguments are undefined, of a wrong type or the precision is 0");
		stackPushUndefined();
		return;
	}

	stackPushFloat(roundf(val * precision) / precision);
}

void gsc_utils_getmilliseconds()
{
	stackPushInt(Sys_Milliseconds() & 0x7FFFFFFF);
}

// Wraps around every 35 minutes, so only differences are meaningful
void gsc_utils_getmicroseconds()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	stackPushInt((int)((uint64_t)tv.tv_sec * 1000000 + tv.tv_usec) & 0x7FFFFFFF);
}

void gsc_utils_tohex()
{
	int input;
	char output[16];

	if (!stackGetParams("i", &input))
	{
		stackError("gsc_utils_tohex() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	Com_sprintf(output, sizeof(output), "0x%X", input);
	stackPushString(output);
}

void gsc_utils_fromhex()
{
	const char *input;
	unsigned int output;

	if (!stackGetParams("s", &input) || sscanf(input, "%X", &output) != 1)
	{
		stackError("gsc_utils_fromhex() argument is undefined, of a wrong type or not a hex number");
		stackPushUndefined();
		return;
	}

	stackPushInt((int)output);
}

// ord("A") is 65
void gsc_utils_ord()
{
	const char *input;

	if (!stackGetParams("s", &input) || strlen(input) != 1)
	{
		stackError("gsc_utils_ord() argument must be a string of one character");
		stackPushUndefined();
		return;
	}

	stackPushInt((unsigned char)input[0]);
}

// chr(65) is "A"; "%" becomes "." so the result is safe in format strings
void gsc_utils_chr()
{
	int input;
	char output[2];

	if (!stackGetParams("i", &input))
	{
		stackError("gsc_utils_chr() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	output[0] = (input & 0xFF) == '%' ? '.' : (char)(input & 0xFF);
	output[1] = '\0';
	stackPushString(output);
}

// Prints to the server console and console log, not to games_mp.log
void gsc_utils_logprintconsole()
{
	const char *str;

	if (!stackGetParams("s", &str))
	{
		stackError("gsc_utils_logprintconsole() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	Com_Printf("%s", str);
	stackPushBool(qtrue);
}

#endif
