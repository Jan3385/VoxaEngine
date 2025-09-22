#pragma once
#include <World/Voxel.h>
#include <Math/Vector.h>

namespace Volume{
    class SolidSimpleVoxel : public VoxelElement
	{
	public:
        SolidSimpleVoxel() = default;
		SolidSimpleVoxel(std::string id, Vec2i position, Temperature temp, float amount);

        State GetState() const override { return State::Solid; };

        bool ShouldTriggerDirtyColliders() const { return true; };
		bool IsSolidCollider() const { return true; };
	private:
	};
}