#include "gsc_sqlite.hpp"

#if LIBCOD_COMPILE_SQLITE == 1

#include "sqlite/sqlite3.h"

#include <string>
#include <vector>

#define MAX_SQLITE_FIELDS 128
#define MAX_SQLITE_ROWS 128
#define MAX_SQLITE_ROW_LENGTH 256
#define MAX_SQLITE_TASKS 512
#define MAX_SQLITE_DB_STORES 128

#define SQLITE_TIMEOUT 4000

enum
{
	INT_VALUE,
	FLOAT_VALUE,
	STRING_VALUE,
	VECTOR_VALUE,
	OBJECT_VALUE
};

struct sqlite_db_store
{
	sqlite_db_store *prev;
	sqlite_db_store *next;
	sqlite3 *db;
	unsigned int levelId;
	bool closed;
};

// CRITSECT_SQLITE guards the task list links, the store list, the store
// fields the worker reads and the done/running/remove flags. The worker
// runs a query outside the lock with running set, and the main thread
// neither frees that task nor closes its store until it is done.
struct async_sqlite_task
{
	async_sqlite_task *prev;
	async_sqlite_task *next;
	sqlite_db_store *store;
	std::string query;
	std::vector<std::string> cells; // rows * columns
	int columns;
	int rows;
	int callback;
	bool done;
	bool running;
	bool save;
	bool error;
	bool remove;
	unsigned int levelId;
	char errorMessage[COD2_MAX_STRINGLENGTH];
	bool hasargument;
	int valueType;
	int intValue;
	float floatValue;
	char stringValue[COD2_MAX_STRINGLENGTH];
	vec3_t vectorValue;
	unsigned int objectValue;
	bool hasentity;
	unsigned int entityId;
};

async_sqlite_task *first_async_sqlite_task = NULL;
sqlite_db_store *first_sqlite_db_store = NULL;
bool sqlite_db_store_cleanup_pending = false;

static void async_sqlite_set_error(async_sqlite_task *task, sqlite3 *db)
{
	task->error = true;
	I_strncpyz(task->errorMessage, sqlite3_errmsg(db), sizeof(task->errorMessage));
}

static void async_sqlite_run_query(async_sqlite_task *task, sqlite3 *db)
{
	sqlite3_stmt *statement;

	if (sqlite3_prepare_v2(db, task->query.c_str(), -1, &statement, NULL) != SQLITE_OK)
	{
		async_sqlite_set_error(task, db);
		return;
	}

	int result;

	while ((result = sqlite3_step(statement)) == SQLITE_ROW)
	{
		if (!task->save || !task->callback)
			continue;

		if (task->rows >= MAX_SQLITE_FIELDS)
			break;

		if (task->rows == 0)
		{
			task->columns = sqlite3_column_count(statement);

			if (task->columns > MAX_SQLITE_ROWS)
				task->columns = MAX_SQLITE_ROWS;
		}

		for (int i = 0; i < task->columns; i++)
		{
			const char *text = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));

			if (text != NULL)
				task->cells.push_back(std::string(text, strnlen(text, MAX_SQLITE_ROW_LENGTH - 1)));
			else
				task->cells.push_back(std::string());
		}

		task->rows++;
	}

	if (result != SQLITE_ROW && result != SQLITE_DONE)
		async_sqlite_set_error(task, db);

	sqlite3_finalize(statement);
}

void *async_sqlite_query_handler(void*)
{
	while(1)
	{
		Sys_EnterCriticalSection(CRITSECT_SQLITE);
		async_sqlite_task *task;

		for (task = first_async_sqlite_task; task != NULL; task = task->next)
		{
			if (task->done || task->remove)
				continue;

			if (task->store == NULL || task->store->closed || libcodLevelGeneration != task->store->levelId)
			{
				task->remove = true;
				continue;
			}

			break;
		}

		if (task == NULL)
		{
			Sys_LeaveCriticalSection(CRITSECT_SQLITE);
			Sys_SleepMSec(50);
			continue;
		}

		task->running = true;
		sqlite3 *db = task->store->db;
		Sys_LeaveCriticalSection(CRITSECT_SQLITE);

		async_sqlite_run_query(task, db);

		Sys_EnterCriticalSection(CRITSECT_SQLITE);
		task->running = false;
		task->done = true;
		Sys_LeaveCriticalSection(CRITSECT_SQLITE);
	}

	return NULL;
}

bool async_sqlite_query_initialized = false;
bool async_sqlite_query_initialize()
{
	threadid_t tinfo;

	if (async_sqlite_query_initialized)
		return true;

	if (!Sys_CreateNewThread(async_sqlite_query_handler, &tinfo, NULL))
		return false;

	async_sqlite_query_initialized = true;
	Com_Printf("async_sqlite_query_initialize() thread initialized.\n");
	return true;
}

static sqlite_db_store *sqlite_find_store(sqlite3 *db)
{
	sqlite_db_store *store = first_sqlite_db_store;

	while (store != NULL)
	{
		if (store->db == db)
			break;

		store = store->next;
	}

	return store;
}

// The task holds references to its object argument and its entity until it
// is delivered or dropped, so neither id can be freed and reused meanwhile
static void async_sqlite_release_references(async_sqlite_task *task)
{
	if (task->hasargument && task->valueType == OBJECT_VALUE && task->objectValue)
	{
		RemoveRefToObject(task->objectValue);
		task->objectValue = 0;
	}

	if (task->entityId)
	{
		RemoveRefToObject(task->entityId);
		task->entityId = 0;
	}
}

static void async_sqlite_create_task(const char *function, scr_entref_t *entref, bool save)
{
	long int db;
	const char *query;

	if ( ! stackGetParams("ls", &db, &query))
	{
		stackError("%s() one or more arguments is undefined or has a wrong type", function);
		stackPushUndefined();
		return;
	}

	if (entref != NULL && entref->classnum != CLASS_NUM_ENTITY)
	{
		stackError("%s() must be called on an entity", function);
		stackPushUndefined();
		return;
	}

	sqlite_db_store *store = sqlite_find_store((sqlite3 *)db);

	if (store == NULL)
	{
		stackError("%s() failed to assign db store to task", function);
		stackPushUndefined();
		return;
	}

	if (store->closed)
	{
		stackError("%s() database is already closed", function);
		stackPushUndefined();
		return;
	}

	// without the worker the task would never run
	if (!async_sqlite_query_initialize())
	{
		stackError("%s() failed to start the query thread", function);
		stackPushUndefined();
		return;
	}

	async_sqlite_task *current = first_async_sqlite_task;
	int task_count = 0;

	// only the main thread changes the list, so it can walk it unlocked
	while (current != NULL && current->next != NULL)
	{
		if (task_count >= MAX_SQLITE_TASKS - 2)
		{
			stackError("%s() exceeded async task limit", function);
			stackPushUndefined();
			return;
		}

		current = current->next;
		task_count++;
	}

	async_sqlite_task *newtask = new async_sqlite_task();

	newtask->store = store;
	newtask->query = query;

	int callback;

	if (stackGetParamFunction(2, &callback))
		newtask->callback = callback;

	newtask->save = save;
	newtask->levelId = libcodLevelGeneration;
	newtask->hasargument = true;

	if (entref != NULL)
	{
		newtask->hasentity = true;
		newtask->entityId = Scr_GetEntityId(entref->entnum, entref->classnum);
		AddRefToObject(newtask->entityId);
	}

	int valueInt;
	float valueFloat;
	const char *valueString;
	vec3_t valueVector;
	unsigned int valueObject;

	if (stackGetParamInt(3, &valueInt))
	{
		newtask->valueType = INT_VALUE;
		newtask->intValue = valueInt;
	}
	else if (stackGetParamFloat(3, &valueFloat))
	{
		newtask->valueType = FLOAT_VALUE;
		newtask->floatValue = valueFloat;
	}
	else if (stackGetParamString(3, &valueString))
	{
		newtask->valueType = STRING_VALUE;
		I_strncpyz(newtask->stringValue, valueString, sizeof(newtask->stringValue));
	}
	else if (stackGetParamVector(3, valueVector))
	{
		newtask->valueType = VECTOR_VALUE;
		newtask->vectorValue[0] = valueVector[0];
		newtask->vectorValue[1] = valueVector[1];
		newtask->vectorValue[2] = valueVector[2];
	}
	else if (stackGetParamObject(3, &valueObject))
	{
		newtask->valueType = OBJECT_VALUE;
		newtask->objectValue = valueObject;
		AddRefToObject(valueObject);
	}
	else
		newtask->hasargument = false;

	Sys_EnterCriticalSection(CRITSECT_SQLITE);
	newtask->prev = current;

	if (current != NULL)
		current->next = newtask;
	else
		first_async_sqlite_task = newtask;

	Sys_LeaveCriticalSection(CRITSECT_SQLITE);

	stackPushBool(qtrue);
}

void gsc_async_sqlite_create_query()
{
	async_sqlite_create_task("gsc_async_sqlite_create_query", NULL, true);
}

void gsc_async_sqlite_create_query_nosave()
{
	async_sqlite_create_task("gsc_async_sqlite_create_query_nosave", NULL, false);
}

void gsc_async_sqlite_create_entity_query(scr_entref_t entid)
{
	async_sqlite_create_task("gsc_async_sqlite_create_entity_query", &entid, true);
}

void gsc_async_sqlite_create_entity_query_nosave(scr_entref_t entid)
{
	async_sqlite_create_task("gsc_async_sqlite_create_entity_query_nosave", &entid, false);
}

static void async_sqlite_deliver(async_sqlite_task *task)
{
	if (task->error)
	{
		// not stackError: it would leave the task in the list to fail again
		if (task->errorMessage[0])
			Com_Printf("gsc_async_sqlite_checkdone() query error in '%s' - '%s'\n", task->query.c_str(), task->errorMessage);

		return;
	}

	if (!Scr_IsSystemActive() || !task->save || !task->callback || libcodLevelGeneration != task->levelId)
		return;

	gentity_t *ent = NULL;

	if (task->hasentity)
	{
		// a freed entity's object turns into a dead entity while referenced
		if (!task->entityId || GetObjectType(task->entityId) != VAR_ENTITY)
			return;

		scr_entref_t entref = Scr_GetEntityIdRef(task->entityId);

		if (entref.classnum != CLASS_NUM_ENTITY)
			return;

		ent = &g_entities[entref.entnum];

		if (!ent->r.inuse)
			return;
	}

	if (task->hasargument)
	{
		switch(task->valueType)
		{
		case INT_VALUE:
			stackPushInt(task->intValue);
			break;

		case FLOAT_VALUE:
			stackPushFloat(task->floatValue);
			break;

		case STRING_VALUE:
			stackPushString(task->stringValue);
			break;

		case VECTOR_VALUE:
			stackPushVector(task->vectorValue);
			break;

		case OBJECT_VALUE:
			if (task->objectValue)
				stackPushObject(task->objectValue);
			else
				stackPushUndefined();
			break;

		default:
			stackPushUndefined();
			break;
		}
	}

	stackPushArray();

	for (int i = 0; i < task->rows; i++)
	{
		stackPushArray();

		for (int x = 0; x < task->columns; x++)
		{
			stackPushString(task->cells[i * task->columns + x].c_str());
			stackPushArrayLast();
		}

		stackPushArrayLast();
	}

	short ret;

	if (ent != NULL)
		ret = Scr_ExecEntThread(ent, task->callback, task->save + task->hasargument);
	else
		ret = Scr_ExecThread(task->callback, task->save + task->hasargument);

	Scr_FreeThread(ret);
}

static async_sqlite_task *async_sqlite_take_finished_task()
{
	Sys_EnterCriticalSection(CRITSECT_SQLITE);
	async_sqlite_task *task = first_async_sqlite_task;

	while (task != NULL && !task->done && !task->remove)
		task = task->next;

	if (task != NULL)
	{
		if (task->next != NULL)
			task->next->prev = task->prev;

		if (task->prev != NULL)
			task->prev->next = task->next;
		else
			first_async_sqlite_task = task->next;
	}

	Sys_LeaveCriticalSection(CRITSECT_SQLITE);
	return task;
}

void sqlite_db_store_cleanup();

void gsc_async_sqlite_checkdone()
{
	async_sqlite_task *task;

	// one at a time: a callback may create tasks or call this again
	while ((task = async_sqlite_take_finished_task()) != NULL)
	{
		if (task->done && !task->remove)
			async_sqlite_deliver(task);

		async_sqlite_release_references(task);
		delete task;
	}

	if (sqlite_db_store_cleanup_pending)
		sqlite_db_store_cleanup();
}

void gsc_async_sqlite_shutdown_level()
{
	for (async_sqlite_task *task = first_async_sqlite_task; task != NULL; task = task->next)
		async_sqlite_release_references(task);
}

void sqlite_db_store_cleanup()
{
	Sys_EnterCriticalSection(CRITSECT_SQLITE);
	sqlite_db_store *current_store = first_sqlite_db_store;
	sqlite_db_store_cleanup_pending = false;

	while (current_store != NULL)
	{
		sqlite_db_store *store = current_store;
		current_store = current_store->next;

		if (store->closed || libcodLevelGeneration != store->levelId)
		{
			async_sqlite_task *current_task;
			bool busy = false;

			for (current_task = first_async_sqlite_task; current_task != NULL; current_task = current_task->next)
			{
				if (current_task->store == store && current_task->running)
					busy = true;
			}

			// the worker is running a query on it, close it on a later call
			if (busy)
			{
				store->closed = true;
				sqlite_db_store_cleanup_pending = true;
				continue;
			}

			for (current_task = first_async_sqlite_task; current_task != NULL; current_task = current_task->next)
			{
				if (current_task->store == store)
					current_task->store = NULL;
			}

			if ( store->db != NULL )
			{
				sqlite3_close_v2(store->db);
				store->db = NULL;
			}

			if (store->next != NULL)
				store->next->prev = store->prev;

			if (store->prev != NULL)
				store->prev->next = store->next;
			else
				first_sqlite_db_store = store->next;

			delete store;
		}
	}

	Sys_LeaveCriticalSection(CRITSECT_SQLITE);
}

extern void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath);

// "<fs_homepath>/<fs_gamedir>/<rel>", as json_save; ":memory:" is kept. Refuses
// paths that leave that folder and the files the server loads from it.
static bool sqlite_resolve_path(const char *rel, char *osPath)
{
	if ( !strcmp(rel, ":memory:") )
	{
		I_strncpyz(osPath, rel, MAX_OSPATH);
		return true;
	}

	size_t len = strlen(rel);

	// ':' is a drive, a URI or a Windows stream; Windows drops trailing dots and spaces
	if ( !len || rel[0] == '/' || rel[0] == '\\' || strchr(rel, ':') || strstr(rel, "..")
		|| rel[len - 1] == '.' || rel[len - 1] == ' ' )
		return false;

	const char *ext = strrchr(rel, '.');
	if ( ext && ( !Q_stricmp(ext, ".iwd") || !Q_stricmp(ext, ".cfg") || !Q_stricmp(ext, ".gsc") || !Q_stricmp(ext, ".so") || !Q_stricmp(ext, ".dll") ) )
		return false;

	const char *home = Dvar_GetString("fs_homepath");

	// FS_BuildOSPath is fatal on overflow; the margin covers fs_gamedir and the separators
	if ( !home || !home[0] || strlen(home) + len + MAX_QPATH + 3 >= MAX_OSPATH )
		return false;

	FS_BuildOSPath(home, "", rel, osPath);
	return true;
}

void gsc_sqlite_open()
{
	const char *database;

	if ( ! stackGetParams("s", &database))
	{
		stackError("gsc_sqlite_open() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	char osPath[MAX_OSPATH];
	if ( !sqlite_resolve_path(database, osPath) )
	{
		stackError("gsc_sqlite_open() the path '%s' must stay inside fs_homepath/<mod>", database);
		stackPushUndefined();
		return;
	}

	sqlite_db_store_cleanup();
	sqlite3 *db;
	char error[COD2_MAX_STRINGLENGTH];
	int rc = sqlite3_open(osPath, &db);

	// a failed open still allocates a handle that must be closed
	if (rc != SQLITE_OK)
	{
		I_strncpyz(error, sqlite3_errmsg(db), sizeof(error));
		sqlite3_close(db);
		stackError("gsc_sqlite_open() cannot open database: %s", error);
		stackPushUndefined();
		return;
	}

	rc = sqlite3_busy_timeout(db, SQLITE_TIMEOUT);

	if (rc != SQLITE_OK)
	{
		I_strncpyz(error, sqlite3_errmsg(db), sizeof(error));
		sqlite3_close(db);
		stackError("gsc_sqlite_open() cannot set database busy timeout: %s", error);
		stackPushUndefined();
		return;
	}

	sqlite_db_store *current = first_sqlite_db_store;
	int store_count = 0;

	while (current != NULL && current->next != NULL)
	{
		if (store_count >= MAX_SQLITE_DB_STORES - 2)
		{
			sqlite3_close(db);
			stackError("gsc_sqlite_open() exceeded db store limit");
			stackPushUndefined();
			return;
		}

		current = current->next;
		store_count++;
	}

	sqlite_db_store *newstore = new sqlite_db_store;

	newstore->prev = current;
	newstore->next = NULL;
	newstore->levelId = libcodLevelGeneration;
	newstore->db = db;
	newstore->closed = false;

	Sys_EnterCriticalSection(CRITSECT_SQLITE);

	if (current != NULL)
		current->next = newstore;
	else
		first_sqlite_db_store = newstore;

	Sys_LeaveCriticalSection(CRITSECT_SQLITE);

	stackPushInt64((intptr_t)db);
}

void gsc_sqlite_query()
{
	long int db;
	const char *query;

	if ( ! stackGetParams("ls", &db, &query))
	{
		stackError("gsc_sqlite_query() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	sqlite_db_store *store = sqlite_find_store((sqlite3 *)db);

	if (store == NULL)
	{
		stackError("gsc_sqlite_query() failed to assign db store");
		stackPushUndefined();
		return;
	}

	if (store->closed)
	{
		stackError("gsc_sqlite_query() database is already closed");
		stackPushUndefined();
		return;
	}

	sqlite3_stmt *statement;
	int result;

	result = sqlite3_prepare_v2((sqlite3 *)db, query, -1, &statement, 0);

	if (result != SQLITE_OK)
	{
		stackError("gsc_sqlite_query() failed to fetch query data: %s", sqlite3_errmsg((sqlite3 *)db));
		stackPushUndefined();
		return;
	}

	stackPushArray();

	result = sqlite3_step(statement);

	while (result != SQLITE_DONE)
	{
		if (result == SQLITE_ROW)
		{
			stackPushArray();

			for (int i = 0; i < sqlite3_column_count(statement); i++)
			{
				const unsigned char *text = sqlite3_column_text(statement, i);

				// NULL as "", like the async queries, so later columns keep their index
				if (text != NULL)
					stackPushString(reinterpret_cast<const char*>(text));
				else
					stackPushString("");

				stackPushArrayLast();
			}

			stackPushArrayLast();
		}
		else
		{
			char error[COD2_MAX_STRINGLENGTH];

			I_strncpyz(error, sqlite3_errmsg((sqlite3 *)db), sizeof(error));
			sqlite3_finalize(statement);
			stackError("gsc_sqlite_query() failed to execute query: %s", error);
			stackPushUndefined();
			return;
		}

		result = sqlite3_step(statement);
	}

	sqlite3_finalize(statement);
}

void gsc_sqlite_close()
{
	long int db;

	if ( ! stackGetParams("l", &db))
	{
		stackError("gsc_sqlite_close() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	Sys_EnterCriticalSection(CRITSECT_SQLITE);
	sqlite_db_store *store = first_sqlite_db_store;

	while (store != NULL)
	{
		if (store->db == (sqlite3 *)db)
			store->closed = true;

		store = store->next;
	}

	Sys_LeaveCriticalSection(CRITSECT_SQLITE);

	sqlite_db_store_cleanup();
	stackPushBool(qtrue);
}

void gsc_sqlite_escape_string()
{
	const char *string;

	if ( ! stackGetParams("s", &string))
	{
		stackError("gsc_sqlite_escape_string() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	char *result = sqlite3_mprintf("%q", string);

	stackPushString(result);
	sqlite3_free(result);
}

void gsc_sqlite_databases_count()
{
	sqlite_db_store *store = first_sqlite_db_store;
	int store_count = 0;

	while (store != NULL)
	{
		store = store->next;
		store_count++;
	}

	stackPushInt(store_count);
}

void gsc_sqlite_tasks_count()
{
	async_sqlite_task *current = first_async_sqlite_task;
	int task_count = 0;

	while (current != NULL)
	{
		current = current->next;
		task_count++;
	}

	stackPushInt(task_count);
}

#endif
