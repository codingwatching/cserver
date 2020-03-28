#include "core.h"
#include "str.h"
#include "client.h"
#include "block.h"

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
	"Stone Brick"
};

cs_bool Block_IsValid(BlockID id) {
	return id < 50 || Block_DefinitionsList[id] != NULL;
}

cs_str Block_GetName(BlockID id) {
	if(!Block_IsValid(id)) return "Unknown block";
	BlockDef *bdef = Block_DefinitionsList[id];
	if(bdef)
		return bdef->name;
	else
		return defaultBlockNames[id];
}

BlockDef *Block_New(BlockID id, cs_str name, cs_uint8 flags) {
	BlockDef *bdef = Memory_Alloc(1, sizeof(BlockDef));
	bdef->name = String_AllocCopy(name);
	bdef->flags = BDF_DYNALLOCED | flags;
	bdef->id = id;
	return bdef;
}

void Block_Free(BlockDef *bdef) {
	if(bdef->flags & BDF_DYNALLOCED) {
		Memory_Free((void *)bdef->name);
		Memory_Free(bdef);
	}
}

cs_bool Block_Define(BlockDef *bdef) {
	if(Block_DefinitionsList[bdef->id]) return false;
	bdef->flags &= ~(BDF_UPDATED | BDF_UNDEFINED);
	Block_DefinitionsList[bdef->id] = bdef;
	return true;
}

cs_bool Block_Undefine(BlockID id) {
	BlockDef *bdef = Block_DefinitionsList[id];
	if(bdef) {
		bdef->flags |= BDF_UNDEFINED;
		bdef->flags &= ~BDF_UPDATED;
		return true;
	}
	return false;
}

void Block_UpdateDefinitions() {
	for(BlockID id = 0; id < 255; id++) {
		BlockDef *bdef = Block_DefinitionsList[id];
		if(bdef && (bdef->flags & BDF_UPDATED) != BDF_UPDATED) {
			bdef->flags |= BDF_UPDATED;
			if(bdef->flags & BDF_UNDEFINED) {
				for(ClientID cid = 0; cid < MAX_CLIENTS; cid++) {
					Client *client = Clients_List[cid];
					if(client) Client_UndefineBlock(client, bdef->id);
				}
				Block_DefinitionsList[id] = NULL;
				Block_Free(bdef);
			} else {
				for(ClientID cid = 0; cid < MAX_CLIENTS; cid++) {
					Client *client = Clients_List[cid];
					if(client) Client_DefineBlock(client, bdef);
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
	cs_uint8 bcount = bbu->data.count++;
	((cs_uint32 *)bbu->data.offsets)[bcount] = htonl(offset - 4);
	bbu->data.ids[bcount] = id;
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
	Memory_Fill(&bbu->data, sizeof(struct _BBUData), 0);
}