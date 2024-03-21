#ifndef _STASH_HPP
#define _STASH_HPP

// includes for this header file
#include <cstdint>
#include "../../utils/memory/memory.hpp"
#include "../../utils/utils.hpp"
#include "player.hpp"

// stash class
class Stash
{
public:
	// default constructor
	Stash() {}

	// used to initialize an stash object
	Stash(uint64_t _ent)
	{
		// store constructor arguments
		this->ent = _ent;
		this->obj = rust->mem->ReadChain<uint64_t>(this->ent, { 0x10, 0x30 });

		// get the stash's position
		this->position = rust->mem->ReadChain<Vector3>(this->obj, { 0x30, 0x8, 0x38, 0x90 });

		// check if the stash is hidden or not
		this->hidden = (rust->mem->Read<int>(this->ent + 0x120) == true);
	}

	// updates stash state
	bool Update()
	{
		// update stash state
		this->hidden = (rust->mem->Read<int>(this->ent + 0x120) == 2048);

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
	bool hidden = false;
};


#endif // _STASH_HPP