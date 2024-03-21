#ifndef _ANIMAL_HPP
#define _ANIMAL_HPP

// includes for this header
#include <cstdint>
#include <string>
#include "../../utils/memory/memory.hpp"
#include "../../utils/utils.hpp"
#include "player.hpp"

// animal class
class Animal
{
public:
	// default constructor
	Animal() {}

	// used to initialize an Animal object 
	Animal(uint64_t _ent)
	{
		// store the constructor arguments
		this->ent = _ent;

		// get the player's transform address
		this->transform = rust->mem->ReadChain<uint64_t>(this->ent, { 0x10, 0x30, 0x30, 0x8, 0x38 });

		// get the position
		this->position = rust->mem->Read<Vector3>(this->transform + 0x90);

		// read the name of the object
		name = rust->mem->ReadASCII(rust->mem->ReadChain<uint64_t>(this->ent, { 0x0, 0x10 }));
	}

	// used to update the animal's information
	bool Update()
	{
		// get the position
		this->position = rust->mem->Read<Vector3>(this->transform + 0x90);

		return true;
	}

	// used to calculate the distance to another player
	int Distance(Player* player)
	{
		return this->position.Distance(player->position);
	}

public:
	uint64_t	ent;			// The BaseItem address
	uint64_t transform;
	Vector3 position;
	std::string name;
};

#endif // _ANIMAL_HPP