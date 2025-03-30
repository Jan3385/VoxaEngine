#pragma once
#include "Voxel.h"
#include "../Math/Vector.h"

namespace Volume {
	class FireVoxel : public VoxelGas
	{
	public:
		FireVoxel(Vec2i position, Temperature temp, float pressure);
		bool Step(ChunkMatrix* matrix) override;

		static bool Spread(ChunkMatrix *matrix, const VoxelElement *FireVoxel);

		constexpr static uint8_t fireColorCount = 8;
		static const RGBA fireColors[8];
	private:
		// Lifetime time in frames
		uint8_t forcedLifetimeTime = 20;
	};

	class FireLiquidVoxel : public VoxelLiquid
	{
	public:
		FireLiquidVoxel(Vec2i position, Temperature temp, float pressure);
		bool Step(ChunkMatrix* matrix) override;
	};
	class FireSolidVoxel : public VoxelSolid
	{
	public:
		FireSolidVoxel(Vec2i position, Temperature temp, float amount, bool isStatic);
		bool Step(ChunkMatrix* matrix) override;
	};
}