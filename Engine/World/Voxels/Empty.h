#pragma once
#include "World/Voxel.h"
#include "Math/Vector.h"

namespace Volume{
    class EmptyVoxel : public VoxelElement
	{
	public:
		EmptyVoxel(Vec2i position);
		bool Step(ChunkMatrix* matrix) override;
	};
}

