#ifndef _DROPPEDITEM_HPP
#define _DROPPEDITEM_HPP

// includes
#include <cstdint>
#include "../../utils/memory/memory.hpp"
#include "../../utils/utils.hpp"
#include "player.hpp"

// dropped item class
class DroppedItem
{
public:
	// default constructor
	DroppedItem() {}

	// used to initialize an ore object
	DroppedItem(uint64_t _ent)
	{
		// store constructor arguments
		this->ent = _ent;
		this->obj = rust->mem->ReadChain<uint64_t>(this->ent, { 0x10, 0x30 });

		// get the ore's position
		this->pos_address = rust->mem->ReadChain<uint64_t>(this->obj, { 0x30, 0x8, 0x38 });
		this->position = rust->mem->Read<Vector3>(this->pos_address + 0x90);

		// read the native name of the object
		std::wstring wname(rust->mem->ReadUnicode(rust->mem->ReadChain<uint64_t>(this->ent, { 0x168, 0x20 }) + 0x14));

		this->name = rust->mem->ReadNative(this->obj + 0x60);
	}

	// updates the modafucking ore
	bool Update()
	{
		this->iterations++;

		// update every 100 iterations
		if (iterations % 100 == 0)
			this->position = rust->mem->Read<Vector3>(this->pos_address + 0x90);

		return true;
	}

	// used to calculate the distance to another player
	int Distance(Player* player)
	{
		return this->position.Distance(player->position);
	}

public:
	uint64_t	ent;			// The BaseEntity address
	uint64_t	obj;			// The GameObject address
	Vector3		position;
	uint64_t	pos_address;
	std::string name;

	int iterations = 0;
};

#endif // _DROPPEDITEM_HPP