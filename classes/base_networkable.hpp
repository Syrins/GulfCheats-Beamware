#ifndef _BASE_NETWORKABLE_HPP
#define _BASE_NETWORKABLE_HPP

// Includes for this header
#include <map>
#include <memory>
#include <vector>
#include <any>
#include "../utils/memory/memory.hpp"
#include "../utils/utils.hpp"
#include "ents/player.hpp"
#include "../skCrypt.h"

// Used to represent the BaseNetworkable class used in Rust.
class BaseNetworkable
{
public: // Public methods for this class

	// The constructor for BaseNetworkable, requires it's address.
	BaseNetworkable(uint64_t address)
	{
		LOG_G(skCrypt("Base networkable address: 0x%llx\n"), address);
		// Store the address to the class definition
		this->class_address = address;

		// Read into the list of children in the BaseNetworkable list
		this->entity_dict = rust->mem->ReadChain<uint64_t>(this->class_address, { 0xb8, 0x0, 0x10 });

		LOG_G(skCrypt("Entity Dictionary: 0x%llx\n"), this->entity_dict);
	}

	// used to discover entities in the BaseNetworkable list
	bool Discover()
	{
		// get the size of the entity list
		int32_t count = rust->mem->ReadChain<int32_t>(this->entity_dict, { 0x28, 0x10 });

		// clear cache if no players exist
		if (count < 2) {
			globals->players.clear();
			globals->ores.map.clear();

			return true;
		}

		// read into the keys/values list
		uint64_t value_list = rust->mem->ReadChain<uint64_t>(this->entity_dict, { 0x28, 0x18 });

		// will store the list of keys and values in the dict
		uint64_t* values = new uint64_t[count * sizeof(uint64_t)];

		// read the list of keys and values
		rust->mem->Read(value_list + 0x20, values, sizeof(uint64_t) * count);

		// copy into new map
		this->entities = std::vector<uint64_t>(values, values + count);

		// delete the allocated keys/values arrays
		delete[] values;

		// clear local players
		this->new_players.clear();
		this->new_ores.clear();

		std::map<uint64_t, Player> players_copy = globals->players;
		std::map<uint64_t, Ore> ores_copy = globals->ores.map;

		// iterate over entities in bn
		for (const auto& entity : this->entities)
		{
			uint64_t network_status = rust->mem->Read<uint64_t>(entity + 0x50);

			if (!network_status)
				continue;
			
			std::string ent_cls = rust->mem->ReadASCII(rust->mem->ReadChain<uint64_t>(entity, { 0x0, 0x10 }));
			if (ent_cls == std::string(skCrypt("BasePlayer")))
			{
				// if player was already found, skip and push already found ent
				if (players_copy.find(entity) != players_copy.end())
				{
					this->new_players[entity] = players_copy[entity];

					// skip discovering this entity
					continue;
				}

				// create new player obj
				Player player = Player(entity);

				// push into new players map
				this->new_players[entity] = player;
			}
			else if (ent_cls == std::string(skCrypt("OreResourceEntity")) || ent_cls == std::string(skCrypt("CollectibleEntity")) || ent_cls == std::string(skCrypt("StashContainer")))  {
				// if ore was already found, skip and push already found ent
				if (ores_copy.find(entity) != ores_copy.end())
				{
					this->new_ores[entity] = ores_copy[entity];

					// skip discovering this entity
					continue;
				}

				// create new ore obj
				Ore ore = Ore(entity);

				// make sure resource is valid 
				if (ore.name != std::string(skCrypt("Invalid"))) {
					// push into new players map
					this->new_ores[entity] = ore;
				}
			}
		}

		// update players map
		globals->players = this->new_players;
		globals->ores.map = this->new_ores;

		// return true ( success )
		return true;
	}

	// used to update entities in bn list
	bool Update()
	{
		std::map<uint64_t, Player> local_players;
		std::map<uint64_t, Player> players_copy = globals->players;

		std::map<uint64_t, Ore> local_ores;
		std::map<uint64_t, Ore> ores_copy = globals->ores.map;

		// update players
		for (auto& ent : players_copy)
		{
			Player player = (Player)ent.second;

			// ensure player is valid
			if (!player.Update())
				continue;

			// update local player
			if (player.local_player == true) {
				globals->local_player = player;
			}

			// push into local players map
			local_players[ent.first] = player;
		}

		// update ores
		for (auto& ent : ores_copy)
		{
			Ore ore = (Ore)ent.second;

			// ensure player is valid
			if (!ore.Update())
				continue;

			// push into local ores  map
			local_ores[ent.first] = ore;
		}

		// update players map
		globals->players = local_players;

		// update ores map
		globals->ores.map = local_ores;

		return true;
	}

	// thread that handles discovery / update of new entities
	void Thread()
	{
		// keeps track of iterations
		int iterations = 0;

		// loop indefinitely
		for (;;)
		{
			// update every iteration 
			Update();

			// every discovery latency ( every second ) discover new entities
			if (iterations % settings->config.latency_discovery * 10 == 0)
				Discover();

			// increment iterations every camera latency
			if (iterations == (int)(5000 / settings->config.latency_update)) {
				iterations = 0;

				// update camera
				//globals->camera = gom->GetCamera();
			}
			else
				iterations++;

			// sleep update latency
			SleepEx(settings->config.latency_update, false);
		}
	}

	// Used to get the entity count within BaseNetworkable
	int32_t Count()
	{
		// fetch and return the count of entries in the entities map
		return this->entities.size();
	}

	std::vector< uint64_t > entities; // will store the entity list for the game.

private: // Private members for this class

	std::map<uint64_t, Player> new_players;
	std::map<uint64_t, Ore> new_ores;

	// Addresses
	uint64_t class_address;
	uint64_t entity_dict;
};

#endif // _BASE_NETWORKABLE_HPP