#pragma once
#include "Voxel.h"
#include "../Math/Vector.h"

namespace Volume {
	class FireVoxel : public VoxelGas
	{
	public:
		FireVoxel();
		FireVoxel(Vec2i position, Temperature temp);
		~FireVoxel();

		// Functions
		bool Step(ChunkMatrix* matrix) override;
	private:
		uint8_t forcedLifetimeTime = 20;
	};
}