#pragma once
#include "Voxel.h"
#include "../Math/Vector.h"

namespace Volume {
	class FireVoxel : public VoxelGas
	{
	public:
		FireVoxel();
		FireVoxel(Vec2i position);
		~FireVoxel();

		// Functions
		bool Step(ChunkMatrix* matrix) override;
	};
}