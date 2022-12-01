#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H
#include "core.h"
#include "types/platform.h"
#include "types/list.h"

typedef struct _PluginInterface {
	cs_str iname;
	void *iptr;
	cs_size isize;
} PluginInterface;

typedef struct _PluginInfo {
	cs_str name, home;
	cs_int32 ver;
} PluginInfo;

typedef cs_bool(*pluginInitFunc)(void);
typedef cs_bool(*pluginUnloadFunc)(cs_bool);
typedef void(*pluginReceiveIface)(cs_str name, void *ptr, cs_size size);
typedef cs_str(*pluginUrlFunc)(void);

typedef struct _Plugin {
	cs_int8 id;
	cs_str name;
	cs_int32 version;
	void *lib;
	PluginInterface *ifaces;
	pluginReceiveIface irecv;
	pluginUnloadFunc unload;
	pluginUrlFunc url;
	AListField *ireqHead;
	AListField *ireqHold;
	Mutex *mutex;
} Plugin;
#endif
