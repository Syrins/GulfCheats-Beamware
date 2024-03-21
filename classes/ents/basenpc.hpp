#ifndef _BASENPC_HPP
#define _BASENPC_HPP

// includes for this header
#include <cstdint>
#include <string>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "../../utils/utils.hpp"
#include <future>
#include "../../utils/memory/memory.hpp"
#include "../../utils/utils.hpp"

// class to represent a player in rust
class BaseNpc
{
public:

	// default constructor
	BaseNpc() {}

	// constructor for this class
	BaseNpc(uint64_t _ent)
	{
		// store the constructor arguments
		this->ent = _ent;

		// get the bone transform addy
		this->bone_transforms = rust->mem->ReadChain<uint64_t>(this->ent, { (uint64_t)offsets->entityModel, (uint64_t)offsets->boneTransforms });
	}

	// used to calculate the distance to another player
	int Distance(Player* player)
	{
		return this->position.Distance(player->position);
	}

	// used to update the player object
	bool Update()
	{
		// read the player's position
		this->UpdatePosition();

		// read the player's health
		this->health = rust->mem->Read<float>(this->ent + offsets->health);

		// check player health
		if (this->health < 0.2) return false;

		return true;
	}

	Vector3 GetVelocity()
	{
		// get to the playermodel
		uint64_t player_model = rust->mem->Read<uint64_t>(this->ent + offsets->playerModel);

		// read and return the velocity vector
		return rust->mem->Read<Vector3>(player_model + offsets->playerVelocity);
	}

	// used to update the player's position
	void UpdatePosition()
	{
		// update the bones map
		this->UpdateBones();

		// read the player's position
		this->position = (this->bones[l_foot] + this->bones[r_foot]) / 2;
	}

	struct Matrix3x4
	{
		byte v0[16];
		byte v1[16];
		byte v2[16];
	};

	// used to update the player's bone position
	void UpdateBones()
	{
		// an bone array we can iterate over
		int bone_array[] = { 1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 18, 20, 21, 22, 24, 25, 26, 46, 47, 48, 55, 56, 57 };

		// iterate through the bone array
		for (int i : bone_array)
		{
			// get the address of the bone
			uint64_t bone = rust->mem->Read<uint64_t>(this->bone_transforms + (0x20 + (i * 0x8)));

			// check that we got the bone
			if (!bone)
			{
				// add an empty transform to the list
				this->bones[(Bones)i] = Vector3();

				// skip this bone
				continue;
			}

			// get the transform address
			uint64_t transform = rust->mem->Read<uint64_t>(bone + 0x10);

			// _m128 -> Vec4
			const __m128 mul_vec0 = { -2.000, 2.000, -2.000, 0.000 };
			const __m128 mul_vec1 = { 2.000, -2.000, -2.000, 0.000 };
			const __m128 mul_vec2 = { -2.000, -2.000, 2.000, 0.000 };

			// read the transform index for this bone
			int index = rust->mem->Read<int>(transform + 0x40);

			// read the transform data ptr
			uint64_t p_transform_data = rust->mem->Read<uint64_t>(transform + 0x38);

			// designate the transform data array
			uint64_t transform_data[2];

			// read the player transform data into the transform data array
			rust->mem->Read((p_transform_data + 0x18), &transform_data, 16);

			// set the size of the matricies buffer
			size_t size_matricies_buffer = 48 * index + 48;

			// set the indicies buffer size
			size_t size_indices_buffer = 4 * index + 4;

			// allocate memory to the matricies buffer
			PVOID p_matricies_buffer = malloc(size_matricies_buffer);

			// allocate memory to the indices buffer
			PVOID p_indices_buffer = malloc(size_indices_buffer);

			if (p_matricies_buffer && p_indices_buffer)
			{
				// transform position if matrices buffer and indices buffer are both correct
				uint64_t max_indicies_addy = (uint64_t)p_indices_buffer + size_indices_buffer;
				uint64_t min_indicies_addy = (uint64_t)p_indices_buffer;

				uint64_t max_matricies_addy = (uint64_t)p_matricies_buffer + size_matricies_buffer;
				uint64_t min_matricies_addy = (uint64_t)p_matricies_buffer;

				// read the matricies buffer into an array
				rust->mem->Read(transform_data[0], p_matricies_buffer, static_cast<uint32_t>(size_matricies_buffer));

				// read the indices buffer into an array
				rust->mem->Read(transform_data[1], p_indices_buffer, static_cast<uint32_t>(size_indices_buffer));

				// get result addy
				uint64_t a_result = (uint64_t)p_matricies_buffer + 0x30 * index;

				// sanity check
				if (a_result < min_matricies_addy || a_result > max_matricies_addy)
				{
					// prevent mem leak
					free(p_matricies_buffer);
					free(p_indices_buffer);

					// add an empty transform to the list
					this->bones[(Bones)i] = Vector3();

					continue;
				}

				// uint64_t -> __m128
				__m128 result = *(__m128*)(a_result);

				// get transform index addy
				uint64_t transformindex_a = (uint64_t)p_indices_buffer + 0x4 * index;

				// sanity check
				if (transformindex_a < min_indicies_addy || transformindex_a > max_indicies_addy)
				{
					// prevent mem leak
					free(p_matricies_buffer);
					free(p_indices_buffer);

					// add an empty transform to the list
					this->bones[(Bones)i] = Vector3();

					continue;
				}

				// uint64_t -> int
				int transform_index = *(int*)(transformindex_a);

				// used to track loop iterations
				int iterations = 0;

				while (transform_index >= 0 && iterations < 10)
				{
					// get matrix 3x4 address
					uint64_t matrix32_a = (uint64_t)p_matricies_buffer + 0x30 * transform_index;

					// sanity check
					if (matrix32_a < min_matricies_addy || matrix32_a > max_matricies_addy)
					{
						// prevent mem leak
						free(p_matricies_buffer);
						free(p_indices_buffer);

						// add an empty transform to the list
						this->bones[(Bones)i] = Vector3();

						continue;
					}

					Matrix3x4 matrix34 = *(Matrix3x4*)(matrix32_a);

					__m128 xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0x00));
					__m128 yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0x55));
					__m128 zwxy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0x8E));
					__m128 wzyw = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0xDB));
					__m128 zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0xAA));
					__m128 yxwy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.v1), 0x71));
					__m128 tmp7 = _mm_mul_ps(*(__m128*)(&matrix34.v2), result);

					result = _mm_add_ps(
						_mm_add_ps(
							_mm_add_ps(
								_mm_mul_ps(
									_mm_sub_ps(
										_mm_mul_ps(_mm_mul_ps(xxxx, mul_vec1), zwxy),
										_mm_mul_ps(_mm_mul_ps(yyyy, mul_vec2), wzyw)),
									_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0xAA))),
								_mm_mul_ps(
									_mm_sub_ps(
										_mm_mul_ps(_mm_mul_ps(zzzz, mul_vec2), wzyw),
										_mm_mul_ps(_mm_mul_ps(xxxx, mul_vec0), yxwy)),
									_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x55)))),
							_mm_add_ps(
								_mm_mul_ps(
									_mm_sub_ps(
										_mm_mul_ps(_mm_mul_ps(yyyy, mul_vec0), yxwy),
										_mm_mul_ps(_mm_mul_ps(zzzz, mul_vec1), zwxy)),
									_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x00))),
								tmp7)), *(__m128*)(&matrix34.v0));

					uint64_t new_transform_index_a = (uint64_t)p_indices_buffer + 0x4 * transform_index;

					// sanity check
					if (new_transform_index_a < min_indicies_addy || new_transform_index_a > max_indicies_addy)
					{
						// prevent mem leak
						free(p_matricies_buffer);
						free(p_indices_buffer);

						// add an empty transform to the list
						this->bones[(Bones)i] = Vector3();

						continue;
					}

					transform_index = *(int*)(new_transform_index_a);

					// increment iterations for cpu management
					iterations++;
				}

				// get the world position
				Vector3 world = Vector3(result.m128_f32[0], result.m128_f32[1], result.m128_f32[2]);

				// update bone
				bones[(Bones)i] = world;
			}

			// free the allocated memory
			free(p_matricies_buffer);
			free(p_indices_buffer);
		}
	}

public:
	uint64_t	ent = 0;				// the BaseEntity address
	uint64_t	bone_transforms;		// the address of the scientists's bone transforms 

	float health = 0.f;					// the player's current health
	Vector3	position = {};				// the player's 3d position#
	std::map<Bones, Vector3> bones = {};// the player's bones map
};

#endif // _BASENPC_HPP