#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "core.h"
#include "client.h"

typedef cs_bool(*PacketHandler)(Client *client, cs_byte id, cs_bool iscpe, void *data);
typedef void *(*PacketReader)(cs_char *data, cs_int16 size, PacketHandler ph);

typedef enum _EPRState {
	PM_STATE_INITIAL,
	PM_STATE_WAITDATA,
	PM_STATE_DATAOK,
	PM_STATE_CLOSED
} EPMState;

typedef enum _EPRReturn {
	PM_RETURN_OK,
	PM_RETURN_FAIL,
	PM_RETURN_NOTREADY,
	PM_RETURN_SOCKETCLOSED,
	PM_RETURN_INVALIDPACKET,
	PM_RETURN_HANDLINGFAILED
} EPMReturn;

typedef struct _PacketManager {
	EPMState state;
	cs_int16 readed, need;
	cs_byte packetId;
	struct _Buffer {
		cs_byte *data;
		cs_size size;
	} buffer;
} PacketManager;

typedef struct _PacketInfo {
	cs_int16 size, cpesize;
	cs_uint32 cpeextension;
	cs_int32 cpeextversion;
	PacketReader preader;
	PacketHandler phandler;
} PacketInfo;
#endif
