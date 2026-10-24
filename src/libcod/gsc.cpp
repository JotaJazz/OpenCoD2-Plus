#include "gsc.hpp"

#ifdef LIBCOD

const char *stackGetParamTypeAsString(int param)
{
	if (param >= Scr_GetNumParam())
		return "UNDEFINED";

	VariableValue *var;
	var = Scr_GetValue(param);

	switch (var->type)
	{
	case STACK_UNDEFINED:
		return "UNDEFINED";

	case STACK_OBJECT:
		return "OBJECT";

	case STACK_STRING:
		return "STRING";

	case STACK_LOCALIZED_STRING:
		return "LOCALIZED_STRING";

	case STACK_VECTOR:
		return "VECTOR";

	case STACK_FLOAT:
		return "FLOAT";

	case STACK_INT:
		return "INT";

	case STACK_INT64:
		return "INT64";

	case STACK_CODEPOS:
		return "CODEPOS";

	case STACK_PRECODEPOS:
		return "PRECODEPOS";

	case STACK_FUNCTION:
		return "FUNCTION";

	case STACK_STACK:
		return "STACK";

	case STACK_ANIMATION:
		return "ANIMATION";

	case STACK_DEVELOPER_CODEPOS:
		return "DEVELOPER_CODEPOS";

	case STACK_INCLUDE_CODEPOS:
		return "INCLUDE_CODEPOS";

	case STACK_THREAD_LIST:
		return "THREAD_LIST";

	case STACK_THREAD_1:
		return "THREAD_1";

	case STACK_THREAD_2:
		return "THREAD_2";

	case STACK_THREAD_3:
		return "THREAD_3";

	case STACK_THREAD_4:
		return "THREAD_4";

	case STACK_STRUCT:
		return "STRUCT";

	case STACK_REMOVED_ENTITY:
		return "REMOVED_ENTITY";

	case STACK_ENTITY:
		return "ENTITY";

	case STACK_ARRAY:
		return "ARRAY";

	case STACK_REMOVED_THREAD:
		return "REMOVED_THREAD";

	default:
		return "UNKNOWN TYPE";
	}
}

// sv_libcodUnsafe, read once at startup: a script's setCvar does not honour DVAR_INIT
bool libcod_unsafeBuiltins = false;

extern scr_function_t scriptFunctions[];

// Builtins that reach the shell, files by raw path, raw memory or any network
// address: without sv_libcodUnsafe a call is a script error
template <xfunction_t F>
static void gsc_unsafe()
{
	if (libcod_unsafeBuiltins)
	{
		F();
		return;
	}

	const char *name = "?";
	for (int i = 0; scriptFunctions[i].name; i++)
	{
		if (scriptFunctions[i].call == gsc_unsafe<F>)
		{
			name = scriptFunctions[i].name;
			break;
		}
	}

	stackError("%s() is disabled: start the server with +set sv_libcodUnsafe 1 to allow it", name);
	stackPushUndefined();
}

scr_function_t scriptFunctions[] =
{
#if LIBCOD_COMPILE_EXEC == 1
	{"exec", gsc_unsafe<gsc_exec>, 0},
	{"exec_async_create", gsc_unsafe<gsc_exec_async_create>, 0},
	{"exec_async_create_nosave", gsc_unsafe<gsc_exec_async_create_nosave>, 0},
	{"exec_async_checkdone", gsc_unsafe<gsc_exec_async_checkdone>, 0},
	{"execute", gsc_unsafe<gsc_exec>, 0},
	{"execute_async_create", gsc_unsafe<gsc_exec_async_create>, 0},
	{"execute_async_create_nosave", gsc_unsafe<gsc_exec_async_create_nosave>, 0},
	{"execute_async_checkdone", gsc_unsafe<gsc_exec_async_checkdone>, 0},
#endif

#if LIBCOD_COMPILE_LEVEL == 1
	{"getnumberofstaticmodels", gsc_level_getnumberofstaticmodels, 0},
	{"getstaticmodelname", gsc_level_getstaticmodelname, 0},
	{"getstaticmodelorigin", gsc_level_getstaticmodelorigin, 0},
	{"getentitycount", gsc_level_getentitycount, 0},
#endif

#if LIBCOD_COMPILE_MEMORY == 1
	{"memory_malloc", gsc_unsafe<gsc_memory_malloc>, 0},
	{"memory_free", gsc_unsafe<gsc_memory_free>, 0},
	{"memory_int_get", gsc_unsafe<gsc_memory_int_get>, 0},
	{"memory_int_set", gsc_unsafe<gsc_memory_int_set>, 0},
	{"memory_memset", gsc_unsafe<gsc_memory_memset>, 0},
	{"binarybuffer_new", gsc_unsafe<gsc_binarybuffer_new>, 0},
	{"binarybuffer_free", gsc_unsafe<gsc_binarybuffer_free>, 0},
	{"binarybuffer_seek", gsc_unsafe<gsc_binarybuffer_seek>, 0},
	{"binarybuffer_write", gsc_unsafe<gsc_binarybuffer_write>, 0},
	{"binarybuffer_read", gsc_unsafe<gsc_binarybuffer_read>, 0},
#endif

#if LIBCOD_COMPILE_MYSQL == 1
	{"mysql_init", gsc_mysql_init, 0},
	{"mysql_real_connect", gsc_mysql_real_connect, 0},
	{"mysql_close", gsc_mysql_close, 0},
	{"mysql_query", gsc_mysql_query, 0},
	{"mysql_errno", gsc_mysql_errno, 0},
	{"mysql_error", gsc_mysql_error, 0},
	{"mysql_affected_rows", gsc_mysql_affected_rows, 0},
	{"mysql_store_result", gsc_mysql_store_result, 0},
	{"mysql_num_rows", gsc_mysql_num_rows, 0},
	{"mysql_num_fields", gsc_mysql_num_fields, 0},
	{"mysql_field_seek", gsc_mysql_field_seek, 0},
	{"mysql_fetch_field", gsc_mysql_fetch_field, 0},
	{"mysql_fetch_row", gsc_mysql_fetch_row, 0},
	{"mysql_free_result", gsc_mysql_free_result, 0},
	{"mysql_real_escape_string", gsc_mysql_real_escape_string, 0},
	{"mysql_async_create_query", gsc_mysql_async_create_query, 0},
	{"mysql_async_create_query_nosave", gsc_mysql_async_create_query_nosave, 0},
	{"mysql_async_getdone_list", gsc_mysql_async_getdone_list, 0},
	{"mysql_async_getresult_and_free", gsc_mysql_async_getresult_and_free, 0},
	{"mysql_async_initializer", gsc_mysql_async_initializer, 0},
	{"mysql_reuse_connection", gsc_mysql_reuse_connection, 0},
#endif

#if LIBCOD_COMPILE_PLAYER == 1
	{"kick2", gsc_kick_slot, 0},
#endif

#if LIBCOD_COMPILE_SQLITE == 1
	{"sqlite_open", gsc_sqlite_open, 0},
	{"sqlite_query", gsc_sqlite_query, 0},
	{"sqlite_close", gsc_sqlite_close, 0},
	{"sqlite_escape_string", gsc_sqlite_escape_string, 0},
	{"sqlite_databases_count", gsc_sqlite_databases_count, 0},
	{"sqlite_tasks_count", gsc_sqlite_tasks_count, 0},
	{"async_sqlite_create_query", gsc_async_sqlite_create_query, 0},
	{"async_sqlite_create_query_nosave", gsc_async_sqlite_create_query_nosave, 0},
	{"async_sqlite_checkdone", gsc_async_sqlite_checkdone, 0},
#endif

#if LIBCOD_COMPILE_UTILS == 1
	{"printf", gsc_utils_printf, 0},
	{"error", gsc_utils_error, 0},
	{"printoutofband", gsc_unsafe<gsc_utils_outofbandprint>, 0},
	{"getarraykeys", gsc_utils_getarraykeys, 0},
	{"getascii", gsc_utils_getAscii, 0},
	{"toupper", gsc_utils_toupper, 0},
	{"system", gsc_unsafe<gsc_utils_system>, 0},
	{"exponent", gsc_utils_exponent, 0},
	{"round", gsc_utils_round, 0},
	{"file_link", gsc_unsafe<gsc_utils_file_link>, 0},
	{"file_unlink", gsc_unsafe<gsc_utils_file_unlink>, 0},
	{"file_exists", gsc_unsafe<gsc_utils_file_exists>, 0},
	{"fs_loaddir", gsc_unsafe<gsc_utils_FS_LoadDir>, 0},
	{"loaddir", gsc_unsafe<gsc_utils_FS_LoadDir>, 0},
	{"gettype", gsc_utils_getType, 0},
	{"float", gsc_utils_float, 0},
	{"cmd_executestring", gsc_utils_ExecuteString, 0},
	{"executecommand", gsc_utils_ExecuteString, 0},
	{"sendgameservercommand", gsc_utils_sendgameservercommand, 0},
	{"sendcommandtoclient", gsc_utils_sendgameservercommand, 0},
	{"sendpacket", gsc_unsafe<gsc_utils_sendpacket>, 0},
	{"scandir", gsc_unsafe<gsc_utils_scandir>, 0},
	{"fopen", gsc_unsafe<gsc_utils_fopen>, 0},
	{"fread", gsc_unsafe<gsc_utils_fread>, 0},
	{"fwrite", gsc_unsafe<gsc_utils_fwrite>, 0},
	{"fclose", gsc_unsafe<gsc_utils_fclose>, 0},
	{"fsize", gsc_unsafe<gsc_utils_fsize>, 0},
	{"ftime", gsc_unsafe<gsc_utils_ftime>, 0},
	{"sprintf", gsc_utils_sprintf, 0},
	{"getsystemtime", gsc_utils_getsystemtime, 0},
	{"getserverstarttime", gsc_utils_getserverstarttime, 0},
	{"getlocaltime", gsc_utils_getlocaltime, 0},
	{"g_findconfigstringindex", gsc_G_FindConfigstringIndex, 0},
	{"findconfigstringindex", gsc_G_FindConfigstringIndex, 0},
	{"g_findconfigstringindexoriginal", gsc_G_FindConfigstringIndexOriginal, 0},
	{"findconfigstringindexoriginal", gsc_G_FindConfigstringIndexOriginal, 0},
	{"getconfigstring", gsc_get_configstring, 0},
	{"setconfigstring", gsc_set_configstring, 0},
	{"makelocalizedstring", gsc_make_localized_string, 0},
	{"sqrt", gsc_utils_sqrt, 0},
	{"abs", gsc_utils_abs, 0},
	{"makestring", gsc_utils_makestring, 0},
	{"makeclientlocalizedstring", gsc_utils_makeclientlocalizedstring, 0},
	{"getcvarflags", gsc_utils_getcvarflags, 0},
	{"getsurfacename", gsc_utils_getsurfacename, 0},
	{"setnorthyaw", gsc_utils_setnorthyaw, 0},
	{"getmovers", gsc_utils_getmovers, 0},
	{"getsavepersist", gsc_utils_getsavepersist, 0},
	{"setsavepersist", gsc_utils_setsavepersist, 0},
	{"setconsoleprefix", gsc_utils_setconsoleprefix, 0},
	{"setnexttestclientname", gsc_utils_setnexttestclientname, 0},
	{"resettestclientnaming", gsc_utils_resettestclientnaming, 0},
	{"floor", gsc_utils_floor, 0},
	{"ceil", gsc_utils_ceil, 0},
	{"pow", gsc_utils_pow, 0},
	{"atan2", gsc_utils_atan2, 0},
	{"roundto", gsc_utils_roundto, 0},
	{"getmilliseconds", gsc_utils_getmilliseconds, 0},
	{"getmicroseconds", gsc_utils_getmicroseconds, 0},
	{"tohex", gsc_utils_tohex, 0},
	{"fromhex", gsc_utils_fromhex, 0},
	{"ord", gsc_utils_ord, 0},
	{"chr", gsc_utils_chr, 0},
	{"logprintconsole", gsc_utils_logprintconsole, 0},
	{"sqrtinv", gsc_utils_sqrtInv, 0},
	{"getlasttestclientnumber", gsc_utils_getlasttestclientnumber, 0},
	{"getmaxmsglen", gsc_utils_getmaxmsglen, 0},
	{"bullethiteffect", gsc_utils_bullethiteffect, 0},
	{"vectorscale", gsc_utils_vectorscale, 0},
	{"remove_file", gsc_unsafe<gsc_utils_remove_file>, 0},
	{"fremove", gsc_unsafe<gsc_utils_remove_file>, 0},
	{"putchar", gsc_utils_putchar, 0},
	{"processremotecommand", gsc_utils_processremotecommand, 0},
#endif

#if LIBCOD_COMPILE_WEAPONS == 1
	{"getweaponmaxammo", gsc_weapons_getweaponmaxammo, 0},
	{"setweaponmaxammo", gsc_weapons_setweaponmaxammo, 0},
	{"getweaponclipsize", gsc_weapons_getweaponclipsize, 0},
	{"setweaponclipsize", gsc_weapons_setweaponclipsize, 0},
	{"getweapondamage", gsc_weapons_getweapondamage, 0},
	{"setweapondamage", gsc_weapons_setweapondamage, 0},
	{"getweaponmeleedamage", gsc_weapons_getweaponmeleedamage, 0},
	{"setweaponmeleedamage", gsc_weapons_setweaponmeleedamage, 0},
	{"getweaponfiretime", gsc_weapons_getweaponfiretime, 0},
	{"setweaponfiretime", gsc_weapons_setweaponfiretime, 0},
	{"getweaponfusetime", gsc_weapons_getweaponfusetime, 0},
	{"getweaponraisetime", gsc_weapons_getweaponraisetime, 0},
	{"getweaponmovespeedscale", gsc_weapons_getweaponmovespeedscale, 0},
	{"getweapondisplayname", gsc_weapons_getweapondisplayname, 0},
	{"issemiautoweapon", gsc_weapons_issemiautoweapon, 0},
	{"ignoreweapon", gsc_weapons_ignoreweapon, 0},
	{"resetignoredweapons", gsc_weapons_resetignoredweapons, 0},
	{"getweaponmeleetime", gsc_weapons_getweaponmeleetime, 0},
	{"setweaponmeleetime", gsc_weapons_setweaponmeleetime, 0},
	{"getweaponreloadtime", gsc_weapons_getweaponreloadtime, 0},
	{"setweaponreloadtime", gsc_weapons_setweaponreloadtime, 0},
	{"getweaponreloademptytime", gsc_weapons_getweaponreloademptytime, 0},
	{"setweaponreloademptytime", gsc_weapons_setweaponreloademptytime, 0},
	{"getweaponcookable", gsc_weapons_getweaponcookable, 0},
	{"setweaponcookable", gsc_weapons_setweaponcookable, 0},
	{"getweaponhitlocmultiplier", gsc_weapons_getweaponhitlocmultiplier, 0},
	{"setweaponhitlocmultiplier", gsc_weapons_setweaponhitlocmultiplier, 0},
	{"getloadedweapons", gsc_weapons_getloadedweapons, 0},
#endif

#if LIBCOD_COMPILE_SPEEX == 1
	{"loadsoundfile", gsc_utils_loadsoundfile, 0},
	{"loadspeexfile", gsc_utils_loadspeexfile, 0},
	{"savespeexfile", gsc_utils_savespeexfile, 0},
	{"getsoundfileduration", gsc_utils_getsoundfileduration, 0},
#endif

#if LIBCOD_COMPILE_JSON == 1
	{"json_parse", gsc_json_parse, 0},
	{"json_stringify", gsc_json_stringify, 0},
	{"json_load", gsc_json_load, 0},
	{"json_save", gsc_json_save, 0},
	{"json_load_async", gsc_json_load_async, 0},
	{"json_save_async", gsc_json_save_async, 0},
	{"json_async_done", gsc_json_async_done, 0},
	{"json_async_result", gsc_json_async_result, 0},
#endif

#if LIBCOD_COMPILE_HTTP == 1
	{"httpfetch", gsc_http_fetch, 0},
	{"http_fetch", gsc_http_fetch, 0},
	{"websocketconnect", gsc_websocket_connect, 0},
	{"websocket_connect", gsc_websocket_connect, 0},
	{"websocketsendtext", gsc_websocket_sendtext, 0},
	{"websocket_sendtext", gsc_websocket_sendtext, 0},
	{"websocketclose", gsc_websocket_close, 0},
	{"websocket_close", gsc_websocket_close, 0},
#endif

#if LIBCOD_COMPILE_GRAPH == 1
	{"graphcreate", gsc_graph_create, 0},
	{"graphfindpath", gsc_graph_find_path, 0},
	{"graphfindclosestnode", gsc_graph_find_closest_node, 0},
	{"graphremove", gsc_graph_remove, 0},
	{"graphremoveall", gsc_graph_removeall, 0},
	{"graphaddnode", gsc_graph_add_node, 0},
	{"graphaddedge", gsc_graph_add_edge, 0},
	{"graphgetnodeorigin", gsc_graph_get_node_origin, 0},
	{"graphgetnodecount", gsc_graph_get_node_count, 0},
	{"graphgetedgecount", gsc_graph_get_edge_count, 0},
	{"graphgetstats", gsc_graph_get_stats, 0},
	{"graphfindclosestedge", gsc_graph_find_closest_edge, 0},
	{"graphgetallnodes", gsc_graph_get_all_nodes, 0},
	{"graphgetalledges", gsc_graph_get_all_edges, 0},
	{"graphgetedgeproperties", gsc_graph_get_edge_properties, 0},
	{"graphsetedgecost", gsc_graph_set_edge_cost, 0},
	{"graphsetedgetype", gsc_graph_set_edge_type, 0},
	{"graphremoveedge", gsc_graph_remove_edge, 0},
	{"graphgetnodeproperties", gsc_graph_get_node_properties, 0},
	{"graphsetnodetype", gsc_graph_set_node_type, 0},
	{"graphsetnodeorigin", gsc_graph_set_node_origin, 0},
	{"graphgetnodeidsaccessiblefrom", gsc_graph_get_node_ids_accessible_from, 0},
	{"graphgetnodeidsaccessibleto", gsc_graph_get_node_ids_accessible_to, 0},
	{"graphremovenode", gsc_graph_remove_node, 0},
	{"graphprecomputepathstonode", gsc_graph_precompute_paths_to_node, 0},
	{"graphsave", gsc_graph_save, 0},
	{"graphload", gsc_graph_load, 0},
	{"graphautodiscover", gsc_graph_autodiscover, 0},
	{"graphautodiscoverex", gsc_graph_autodiscover_ex, 0},
#endif

#ifdef EXTRA_FUNCTIONS_INC
#include "extra/functions.hpp"
#endif

	{NULL, NULL, 0} /* terminator */
};

xfunction_t Scr_GetCustomFunction(const char **fname, qboolean *fdev)
{
	for (int i = 0; scriptFunctions[i].name; i++)
	{
		if (strcasecmp(*fname, scriptFunctions[i].name))
			continue;

		scr_function_t func = scriptFunctions[i];

		*fname = func.name;
		*fdev = func.developer;

		return func.call;
	}

	return NULL;
}

scr_method_t scriptMethods[] =
{
#if LIBCOD_COMPILE_BOTS == 1
	{"setwalkdir", gsc_bots_set_walkdir, 0},
	{"setlean", gsc_bots_set_lean, 0},
	{"setbotstance", gsc_bots_set_stance, 0},
	{"thrownade", gsc_bots_thrownade, 0},
	{"throwgrenade", gsc_bots_thrownade, 0},
	{"fireweapon", gsc_bots_fireweapon, 0},
	{"meleeweapon", gsc_bots_meleeweapon, 0},
	{"reloadweapon", gsc_bots_reloadweapon, 0},
	{"adsaim", gsc_bots_adsaim, 0},
	{"setaim", gsc_bots_adsaim, 0},
	{"switchtoweaponid", gsc_bots_switchtoweaponid, 0},
	{"usebutton", gsc_bots_usebutton, 0},
	{"throwsmoke", gsc_bots_throwsmoke, 0},
	{"throwsmokegrenade", gsc_bots_throwsmoke, 0},
	{"holdbreath", gsc_bots_holdbreath, 0},
	{"setwalkvalues", gsc_bots_setwalkvalues, 0},
#endif

#if LIBCOD_COMPILE_ENTITY == 1
	{"setalive", gsc_entity_setalive, 0},
	{"setbounds", gsc_entity_setbounds, 0},
	{"spawngrenade", gsc_entity_spawngrenade, 0},
	{"getgrenadefusetime", gsc_entity_getgrenadefusetime, 0},
	{"addgrenadefusetime", gsc_entity_addgrenadefusetime, 0},
	{"isturret", gsc_entity_isturret, 0},
	{"getturretowner", gsc_entity_getturretowner, 0},
	{"getweaponitemammo", gsc_entity_getweaponitemammo, 0},
	{"setweaponitemammo", gsc_entity_setweaponitemammo, 0},
	{"getweaponitemclipammo", gsc_entity_getweaponitemclipammo, 0},
	{"setweaponitemclipammo", gsc_entity_setweaponitemclipammo, 0},
	{"getitemquantity", gsc_entity_getitemquantity, 0},
	{"setitemquantity", gsc_entity_setitemquantity, 0},
	{"gettagorigin", gsc_entity_gettagorigin, 0},
	{"gettagangles", gsc_entity_gettagangles, 0},
	{"hastag", gsc_entity_hastag, 0},
	{"islinkedto", gsc_entity_islinkedto, 0},
	{"getvmin", gsc_entity_getvmin, 0},
	{"getvmax", gsc_entity_getvmax, 0},
	{"getclipmask", gsc_entity_getclipmask, 0},
	{"setclipmask", gsc_entity_setclipmask, 0},
	{"getcontents", gsc_entity_getcontents, 0},
	{"setlight", gsc_entity_setlight, 0},
	{"hidefromplayer", gsc_entity_hidefromplayer, 0},
	{"notsolidforplayer", gsc_entity_notsolidforplayer, 0},
	{"solidforplayer", gsc_entity_solidforplayer, 0},
	{"enablegravity", gsc_entity_enablegravity, 0},
	{"disablegravity", gsc_entity_disablegravity, 0},
	{"isgravityenabled", gsc_entity_isgravityenabled, 0},
	{"enablebounce", gsc_entity_enablebounce, 0},
	{"disablebounce", gsc_entity_disablebounce, 0},
	{"getentityvelocity", gsc_entity_getentityvelocity, 0},
	{"setentityvelocity", gsc_entity_setentityvelocity, 0},
	{"addentityvelocity", gsc_entity_addentityvelocity, 0},
	{"setmaxentityvelocity", gsc_entity_setmaxentityvelocity, 0},
	{"getmaxentityvelocity", gsc_entity_getmaxentityvelocity, 0},
#endif

#if LIBCOD_COMPILE_PLAYER == 1
	{"getstance", gsc_player_stance_get, 0},
	{"setstance", gsc_player_stance_set, 0},
	{"setvelocity", gsc_player_velocity_set, 0},
	{"addvelocity", gsc_player_velocity_add, 0},
	{"getvelocity", gsc_player_velocity_get, 0},
	{"aimbuttonpressed", gsc_player_button_ads, 0},
	{"leftbuttonpressed", gsc_player_button_left, 0},
	{"rightbuttonpressed", gsc_player_button_right, 0},
	{"forwardbuttonpressed", gsc_player_button_forward, 0},
	{"backbuttonpressed", gsc_player_button_back, 0},
	{"leanleftbuttonpressed", gsc_player_button_leanleft, 0},
	{"leanrightbuttonpressed", gsc_player_button_leanright, 0},
	{"jumpbuttonpressed", gsc_player_button_jump, 0},
	{"reloadbuttonpressed", gsc_player_button_reload, 0},
	{"fragbuttonpressed", gsc_player_button_frag, 0},
	{"smokebuttonpressed", gsc_player_button_smoke, 0},
	{"getip", gsc_player_getip, 0},
	{"getping", gsc_player_getping, 0},
	{"getspectatorclient", gsc_player_spectatorclient_get, 0},
	{"clientcommand", gsc_player_clientcommand, 0},
	{"processclientcommand", gsc_player_clientcommand, 0},
	{"getlastconnecttime", gsc_player_getlastconnecttime, 0},
	{"getlastmsg", gsc_player_getlastmsg, 0},
	{"getaddresstype", gsc_player_addresstype, 0},
	{"getclientstate", gsc_player_getclientstate, 0},
	{"getclientconnectstate", gsc_player_getclientstate, 0},
	{"renameclient", gsc_player_renameclient, 0},
	{"get_userinfo", gsc_player_get_userinfo, 0},
	{"set_userinfo", gsc_player_set_userinfo, 0},
	{"printoutofband", gsc_player_outofbandprint, 0},
	{"connectionlesspackettoclient", gsc_player_outofbandprint, 0},
	{"connectionlesspacket", gsc_player_connectionlesspacket, 0},
	{"connectionlesspackettoserver", gsc_player_connectionlesspacket, 0},
	{"clientuserinfochanged", gsc_player_clientuserinfochanged, 0},
	{"processclientuserinfochange", gsc_player_clientuserinfochanged, 0},
	{"resetnextreliabletime", gsc_player_resetnextreliabletime, 0},
	{"setg_speed", gsc_player_setg_speed, 0},
	{"setg_gravity", gsc_player_setg_gravity, 0},
	{"setweaponfiremeleedelay", gsc_player_setweaponfiremeleedelay, 0},
	{"setanim", gsc_player_set_anim, 0},
	{"getjumpslowdowntimer", gsc_player_getjumpslowdowntimer, 0},
	{"clearjumpstate", gsc_player_clearjumpstate, 0},
	{"getcooktime", gsc_player_getcooktime, 0},
	{"setguid", gsc_player_setguid, 0},
	{"clienthasclientmuted", gsc_player_clienthasclientmuted, 0},
	{"getlastgamestatesize", gsc_player_getlastgamestatesize, 0},
	{"getfps", gsc_player_getfps, 0},
	{"ismantling", gsc_player_ismantling, 0},
	{"isonladder", gsc_player_isonladder, 0},
	{"isusingturret", gsc_player_isusingturret, 0},
	{"isbot", gsc_player_isbot, 0},
	{"disableitempickup", gsc_player_disableitempickup, 0},
	{"enableitempickup", gsc_player_enableitempickup, 0},
	{"getcurrentoffhandslotammo", gsc_player_getcurrentoffhandslotammo, 0},
	{"getprotocol", gsc_player_getprotocol, 0},
	{"islocalclient", gsc_player_islocalclient, 0},
	{"isfiring", gsc_player_isfiring, 0},
	{"ismeleeing", gsc_player_ismeleeing, 0},
	{"isreloading", gsc_player_isreloading, 0},
	{"isthrowinggrenade", gsc_player_isthrowinggrenade, 0},
	{"isusingbinoculars", gsc_player_isusingbinoculars, 0},
	{"isrechambering", gsc_player_isrechambering, 0},
	{"isshellshocked", gsc_player_isshellshocked, 0},
	{"ischatting", gsc_player_ischatting, 0},
	{"holdbreathbuttonpressed", gsc_player_button_holdbreath, 0},
	{"getcurrentweaponammo", gsc_player_getcurrentweaponammo, 0},
	{"getcurrentweaponclipammo", gsc_player_getcurrentweaponclipammo, 0},
	{"setcurrentweaponammo", gsc_player_setcurrentweaponammo, 0},
	{"setcurrentweaponclipammo", gsc_player_setcurrentweaponclipammo, 0},
	{"getcurrentweaponslot", gsc_player_getcurrentweaponslot, 0},
	{"getspeed", gsc_player_getspeed, 0},
	{"setspeed", gsc_player_setg_speed, 0},
	{"getgravity", gsc_player_getgravity, 0},
	{"setgravity", gsc_player_setg_gravity, 0},
	{"getgroundentity", gsc_player_getgroundentity, 0},
	{"getvieworigin", gsc_player_getvieworigin, 0},
	{"getplayerstateflags", gsc_player_getplayerstateflags, 0},
	{"getinactivitytime", gsc_player_getinactivitytime, 0},
	{"getentertime", gsc_player_getentertime, 0},
	{"processsuicide", gsc_player_processsuicide, 0},
	{"getuserinfo", gsc_player_getuserinfo, 0},
	{"setuserinfo", gsc_player_set_userinfo, 0},
	{"muteclient", gsc_player_muteclient, 0},
	{"unmuteclient", gsc_player_unmuteclient, 0},
	{"sethiddenfromscoreboard", gsc_player_sethiddenfromscoreboard, 0},
	{"ishiddenfromscoreboard", gsc_player_ishiddenfromscoreboard, 0},
	{"sethiddenfromserverstatus", gsc_player_sethiddenfromserverstatus, 0},
	{"ishiddenfromserverstatus", gsc_player_ishiddenfromserverstatus, 0},
	{"setallowspectators", gsc_player_setallowspectators, 0},
	{"getauthorizationstatus", gsc_player_getauthorizationstatus, 0},
	{"setactivateonusebuttonrelease", gsc_player_setactivateonusebuttonrelease, 0},
	{"lookatkiller", gsc_player_lookatkiller, 0},
	{"playscriptanimation", gsc_player_playscriptanimation, 0},
	{"isallowingspectators", gsc_player_isallowingspectators, 0},
	{"setconfigstringforplayer", gsc_player_setconfigstringforplayer, 0},
	{"getservercommandqueuesize", gsc_player_getservercommandqueuesize, 0},
	{"getcdkeyhash", gsc_player_getcdkeyhash, 0},
	{"getclienthudelemcount", gsc_player_getclienthudelemcount, 0},
	{"setweaponspreadscale", gsc_player_setweaponspreadscale, 0},
	{"setturretspreadscale", gsc_player_setturretspreadscale, 0},
	{"setmeleerangescale", gsc_player_setmeleerangescale, 0},
	{"setmeleewidthscale", gsc_player_setmeleewidthscale, 0},
	{"setmeleeheightscale", gsc_player_setmeleeheightscale, 0},
	{"setfirerangescale", gsc_player_setfirerangescale, 0},
	{"setbulletmask", gsc_player_setbulletmask, 0},
	{"getbulletmask", gsc_player_getbulletmask, 0},
	{"setfirethroughwalls", gsc_player_setfirethroughwalls, 0},
	{"enablebulletimpacts", gsc_player_enablebulletimpacts, 0},
	{"disablebulletimpacts", gsc_player_disablebulletimpacts, 0},
	{"canuseturret", gsc_player_canuseturret, 0},
	{"useturret", gsc_player_useturret, 0},
	{"stopuseturret", gsc_player_stopuseturret, 0},
	{"canmantle", gsc_player_canmantle, 0},
	{"getweaponanimation", gsc_player_getweaponanimation, 0},
	{"forceshot", gsc_player_forceshot, 0},
	{"noclip", gsc_player_noclip, 0},
	{"isusetouching", gsc_player_isusetouching, 0},
	{"getprotocolstring", gsc_player_getprotocolstring, 0},
	{"setoriginandangles", gsc_player_setoriginandangles, 0},
	{"overridecontents", gsc_player_overridecontents, 0},
	{"playfxforplayer", gsc_player_playfxforplayer, 0},
	{"earthquakeforplayer", gsc_player_earthquakeforplayer, 0},
	{"disableearthquakes", gsc_player_disableearthquakes, 0},
	{"enableearthquakes", gsc_player_enableearthquakes, 0},
	{"setnorthyawforplayer", gsc_player_setnorthyawforplayer, 0},
	{"setcullfogforplayer", gsc_player_setcullfogforplayer, 0},
	{"setexpfogforplayer", gsc_player_setexpfogforplayer, 0},
	{"objective_player_add", gsc_player_objective_player_add, 0},
	{"objective_player_delete", gsc_player_objective_player_delete, 0},
	{"objective_player_icon", gsc_player_objective_player_icon, 0},
	{"objective_player_position", gsc_player_objective_player_position, 0},
	{"objective_player_state", gsc_player_objective_player_state, 0},
	{"enablesilent", gsc_player_enablesilent, 0},
	{"disablesilent", gsc_player_disablesilent, 0},
	{"setcollisionteam", gsc_player_setcollisionteam, 0},
	{"getcollisionteam", gsc_player_getcollisionteam, 0},
	{"playfxontagforplayer", gsc_player_playfxontagforplayer, 0},
	{"enablebulletdrop", gsc_player_enablebulletdrop, 0},
	{"disablebulletdrop", gsc_player_disablebulletdrop, 0},
	{"setbulletdrag", gsc_player_setbulletdrag, 0},
	{"setbulletvelocity", gsc_player_setbulletvelocity, 0},
#if LIBCOD_COMPILE_SPEEX == 1
	{"playsoundfile", gsc_player_playsoundfile, 0},
	{"stopsoundfile", gsc_player_stopsoundfile, 0},
	{"isplayingsoundfile", gsc_player_isplayingsoundfile, 0},
	{"getremainingsoundfileduration", gsc_player_getremainingsoundfileduration, 0},
#endif
	{"setanimation", gsc_player_set_anim, 0},
#endif

#if LIBCOD_COMPILE_SQLITE == 1
	{"async_sqlite_create_entity_query", gsc_async_sqlite_create_entity_query, 0},
	{"async_sqlite_create_entity_query_nosave", gsc_async_sqlite_create_entity_query_nosave, 0},
#endif

#ifdef EXTRA_METHODS_INC
#include "extra/methods.hpp"
#endif

	{NULL, NULL, 0} /* terminator */
};

xmethod_t Scr_GetCustomMethod(const char **fname, qboolean *fdev)
{
	for (int i = 0; scriptMethods[i].name; i++)
	{
		if (strcasecmp(*fname, scriptMethods[i].name))
			continue;

		scr_method_t func = scriptMethods[i];

		*fname = func.name;
		*fdev = func.developer;

		return func.call;
	}

	return NULL;
}

int stackGetParamType(int param)
{
	if (param >= Scr_GetNumParam())
		return STACK_UNDEFINED;

	VariableValue *var;
	var = Scr_GetValue(param);

	return var->type;
}

void stackError(const char *format, ...)
{
	static char errorMessage[COD2_MAX_STRINGLENGTH];
	memset(errorMessage, 0, sizeof(errorMessage));
	va_list va;

	va_start(va, format);
	Q_vsnprintf(errorMessage, sizeof(errorMessage), format, va);
	va_end(va);

	errorMessage[sizeof(errorMessage) - 1] = '\0';
	Scr_Error(errorMessage);
}

int stackGetParams(const char *params, ...)
{
	va_list args;
	va_start(args, params);

	int errors = 0;

	for (size_t i = 0; i < strlen(params); i++)
	{
		switch (params[i])
		{
		case ' ': // ignore param
			break;

		case 'i':
		{
			int *tmp = va_arg(args, int *);
			if ( ! stackGetParamInt(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not an int\n", i);
				errors++;
			}
			break;
		}

		case 'l':
		{
			long int *tmp = va_arg(args, long int *);
			if ( ! stackGetParamInt64(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not an int64\n", i);
				errors++;
			}
			break;
		}

		case 'v':
		{
			float *tmp = va_arg(args, float *);
			if ( ! stackGetParamVector(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not a vector\n", i);
				errors++;
			}
			break;
		}

		case 'f':
		{
			float *tmp = va_arg(args, float *);
			if ( ! stackGetParamFloat(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not a float\n", i);
				errors++;
			}
			break;
		}

		case 's':
		{
			const char **tmp = va_arg(args, const char **);
			if ( ! stackGetParamString(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not a string\n", i);
				errors++;
			}
			break;
		}

		case 'c':
		{
			unsigned int *tmp = va_arg(args, unsigned int *);
			if ( ! stackGetParamConstString(i, tmp))
			{
				Com_DPrintf("\nstackGetParams() Param %i is not a const string\n", i);
				errors++;
			}
			break;
		}

		default:
			errors++;
			Com_DPrintf("\nUnknown identifier [%c] passed to stackGetParams()\n", params[i]);
			break;
		}
	}

	va_end(args);
	return errors == 0; // success if no errors
}

int stackGetParamInt(int param, int *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type == STACK_FLOAT)
	{
		*value = var->u.floatValue;
		return 1;
	}

	if (var->type == STACK_INT64)
	{
		*value = var->u.int64Value;
		return 1;
	}

	if (var->type != STACK_INT)
		return 0;

	*value = var->u.intValue;

	return 1;
}

int stackGetParamInt64(int param, long int *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type == STACK_FLOAT)
	{
		*value = var->u.floatValue;
		return 1;
	}

	if (var->type == STACK_INT)
	{
		*value = var->u.intValue;
		return 1;
	}

	if (var->type != STACK_INT64)
		return 0;

	*value = var->u.int64Value;

	return 1;
}

int stackGetParamFunction(int param, int *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type != STACK_FUNCTION)
		return 0;

	*value = var->u.codePosValue - scrVarPub.programBuffer;

	return 1;
}

int stackGetParamString(int param, const char **value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type != STACK_STRING)
		return 0;

	*value = SL_ConvertToString(var->u.stringValue);

	return 1;
}

int stackGetParamConstString(int param, unsigned int *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type != STACK_STRING)
		return 0;

	*value = var->u.stringValue;

	return 1;
}

int stackGetParamVector(int param, vec3_t value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type != STACK_VECTOR)
		return 0;

	VectorCopy(var->u.vectorValue, value);

	return 1;
}

int stackGetParamFloat(int param, float *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type == STACK_INT)
	{
		*value = var->u.intValue;
		return 1;
	}

	if (var->type != STACK_FLOAT)
		return 0;

	*value = var->u.floatValue;

	return 1;
}

int stackGetParamObject(int param, unsigned int *value)
{
	if (param >= Scr_GetNumParam())
		return 0;

	VariableValue *var;
	var = Scr_GetValue(param);

	if (var->type != STACK_OBJECT)
		return 0;

	*value = var->u.pointerValue;

	return 1;
}

/**
 * @brief Base time in seconds
 */
time_t sys_timeBase64 = 0;

/**
 * @brief Current time in ms, using sys_timeBase64 as origin
 */
uint64_t Sys_Milliseconds64(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase64)
	{
		sys_timeBase64 = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	return (tp.tv_sec - sys_timeBase64) * 1000 + tp.tv_usec / 1000;
}

VariableValue* Scr_GetValue(unsigned int param)
{
	return &scrVmPub.top[int(-param)];
}

#endif
