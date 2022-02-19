#include "core.h"
#include "str.h"
#include "world.h"
#include "client.h"
#include "block.h"
#include "platform.h"
#include "types/list.h"

static cs_str defaultBlockNames[] = {
	"Air", "Stone", "Grass", "Dirt",
	"Cobblestone", "Planks", "Sapling",
	"Bedrock", "Water", "Still Water",
	"Lava", "Still Lava", "Sand",
	"Gravel", "Gold Ore", "Iron Ore",
	"Coal Ore", "Wood", "Leaves",
	"Sponge", "Glass", "Red Wool",
	"Orange Wool", "Yellow Wool",
	"Lime Wool", "Green Wool",
	"Teal Wool", "Aqua Wool",
	"Cyan Wool", "Blue Wool",
	"Indigo Wool", "Violen Wool",
	"Magenta Wool", "Pink Wool",
	"Black Wool", "Gray Wool",
	"White Wool", "Dandelion",
	"Rose", "Brown Mushroom",
	"Red Mushroom", "Gold Block",
	"Iron Block", "Double Slab",
	"Slab", "Brick", "TNT", "Bookshelf",
	"Mossy Stone", "Obsidian",
	"Cobblestone Slab", "Rope", "Sandstone",
	"Snow", "Fire", "Light-Pink Wool",
	"Forest-Green Wool", "Brown Wool",
	"Deep-Blue Wool", "Turquoise Wool", "Ice",
	"Ceramic Tile", "Magma", "Pillar", "Crate",
	"Stone Brick", "Cobblestone Slab", "Rope",
	"Sandstone", "Snow", "Fire", "Light Pink Wool",
	"Forest Green Wool", "Brown Wool", "Deep Blue",
	"Turquoise Wool", "Ice", "Ceramic Tile", "Magma",
	"Pillar", "Crate", "Stone Brick"
};

cs_bool Block_IsValid(World *world, BlockID id) {
	return id < 66 || world->info.bdefines[id] != NULL;
}

BlockID Block_GetFallbackFor(World *world, BlockID id) {
	if(world->info.bdefines[id])
		return world->info.bdefines[id]->fallback;

	switch(id) {
		case BLOCK_COBBLESLAB: return BLOCK_SLAB;
		case BLOCK_ROPE: return BLOCK_BROWN_SHROOM;
		case BLOCK_SANDSTONE: return BLOCK_SAND;
		case BLOCK_SNOW: return BLOCK_AIR;
		case BLOCK_FIRE: return BLOCK_LAVA;
		case BLOCK_LIGHTPINK: return BLOCK_PINK;
		case BLOCK_FORESTGREEN: return BLOCK_GREEN;
		case BLOCK_BROWN: return BLOCK_DIRT;
		case BLOCK_DEEPBLUE: return BLOCK_BLUE;
		case BLOCK_TURQUOISE: return BLOCK_CYAN;
		case BLOCK_ICE: return BLOCK_GLASS;
		case BLOCK_CERAMICTILE: return BLOCK_IRON;
		case BLOCK_MAGMA: return BLOCK_OBSIDIAN;
		case BLOCK_PILLAR: return BLOCK_WHITE;
		case BLOCK_CRATE: return BLOCK_WOOD;
		case BLOCK_STONEBRICK: return BLOCK_STONE;
		default: return id;
	}
}

cs_str Block_GetName(World *world, BlockID id) {
	if(!Block_IsValid(world, id))
		return "Unknown block";

	if(world->info.bdefines[id])
		return world->info.bdefines[id]->name;

	return defaultBlockNames[id];
}

BlockDef *Block_New(BlockID id, cs_str name, cs_byte flags) {
	BlockDef *bdef = Memory_Alloc(1, sizeof(BlockDef));
	String_Copy(bdef->name, 65, name);
	bdef->flags = BDF_DYNALLOCED | flags;
	bdef->id = id;
	return bdef;
}

void Block_Free(BlockDef *bdef) {
	if(bdef->flags & BDF_DYNALLOCED)
		Memory_Free(bdef);
}

cs_bool Block_Define(World *world, BlockDef *bdef) {
	if(world->info.bdefines[bdef->id]) return false;
	bdef->flags &= ~(BDF_UPDATED | BDF_UNDEFINED);
	world->info.bdefines[bdef->id] = bdef;
	return true;
}

BlockDef *Block_GetDefinition(World *world, BlockID id) {
	return world->info.bdefines[id];
}

cs_bool Block_Undefine(World *world, BlockDef *bdef) {
	if(!world->info.bdefines[bdef->id]) return false;
	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *client = Clients_List[id];
		if(client && Client_IsInWorld(client, world))
			Client_UndefineBlock(client, bdef->id);
	}
	world->info.bdefines[bdef->id] = NULL;
	return true;
}

void Block_UndefineGlobal(BlockDef *bdef) {
	bdef->flags |= BDF_UNDEFINED;
	bdef->flags &= ~BDF_UPDATED;
}

void Block_UpdateDefinition(BlockDef *bdef) {
	if(bdef->flags & BDF_UPDATED) return;
	bdef->flags |= BDF_UPDATED;

	AListField *tmp;
	List_Iter(tmp, World_Head) {
		World *world = (World *)tmp->value.ptr;
		if(world->info.bdefines[bdef->id] == bdef) {
			if(bdef->flags & BDF_UNDEFINED) {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_UndefineBlock(client, bdef->id);
				}
				world->info.bdefines[bdef->id] = NULL;
			} else {
				for(ClientID id = 0; id < MAX_CLIENTS; id++) {
					Client *client = Clients_List[id];
					if(client && Client_IsInWorld(client, world))
						Client_DefineBlock(client, bdef);
				}
			}
		}
	}
}

cs_bool Block_BulkUpdateAdd(BulkBlockUpdate *bbu, cs_uint32 offset, BlockID id) {
	if(bbu->data.count == 255) {
		if(bbu->autosend) {
			Block_BulkUpdateSend(bbu);
			Block_BulkUpdateClean(bbu);
		} else return false;
	}
	((cs_uint32 *)bbu->data.offsets)[bbu->data.count++] = htonl(offset);
	bbu->data.ids[bbu->data.count] = id;
	return true;
}

void Block_BulkUpdateSend(BulkBlockUpdate *bbu) {
	for(ClientID cid = 0; cid < MAX_CLIENTS; cid++) {
		Client *client = Clients_List[cid];
		if(client && Client_IsInWorld(client, bbu->world))
			Client_BulkBlockUpdate(client, bbu);
	}
}

void Block_BulkUpdateClean(BulkBlockUpdate *bbu) {
	Memory_Zero(&bbu->data, sizeof(struct _BBUData));
}
