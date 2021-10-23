#ifndef NETHANDLERS_H
#define NETHANDLERS_H
#include "core.h"
#include "vector.h"

/*
	Vanilla packets
	Client -> Server
*/

struct _IdentPacket {
	cs_byte protocol;
	cs_char name[65];
	cs_char key[65];
	cs_byte unused;
};

struct _SetBlockPacket {
	SVec pos;
	cs_byte mode;
	BlockID block;
};

struct _TeleportPacket {
	ClientID id;
	SVec pos;
	Ang ang;
};

struct _MessagePacket {
	cs_char message[65];
	cs_byte unused;
};

static void NetHandlers_RegisterAll(void);
#endif
