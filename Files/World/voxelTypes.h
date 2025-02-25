#pragma once
#include "Voxel.h"
#include "../Math/Vector.h"

namespace Volume {
	class FireVoxel : public VoxelGas
	{
	public:
		FireVoxel(Vec2i position, Temperature temp, float pressure);
		bool Step(ChunkMatrix* matrix) override;
	private:
		uint8_t forcedLifetimeTime = 20;
	};

	class IronVoxel : public VoxelSolid
	{
	public:
		IronVoxel(Vec2i position, Temperature temp);
		bool Step(ChunkMatrix* matrix) override;
	};
}