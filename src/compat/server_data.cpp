// Globals of the engine's core that CoD2rev-Plus replaces in the dedicated
// server (test/plus_replaces.txt) and that the data blobs or code linked but
// not run there still name: the values of src/PC/qcommon.
extern "C" {

// common.c and msg_mp.c: strings the blobs' tables point to (data32.c,
// import_pointers_native.c)
extern const char str_002aee58[] = "mp/playeranim.script";
extern const char str_0021eb5c[] = "type";
extern const char str_002ace30[] = "text";
// surfaceflags.c: a surface name ui_shared_obj.c's tables point to
extern const char str_0021a570[] = "origin";

// net_chan_mp.c and common.c: the client's qport and CD key (client_mp, ui)
int g_qport = 0x0;
char cl_cdkey[52] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };
char cl_cdkeychecksum[12] = { 0x20, 0x20, 0x20, 0x20 };

}
