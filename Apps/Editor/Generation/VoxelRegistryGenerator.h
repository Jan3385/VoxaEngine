#pragma once

#include <Registry/VoxelRegistry.h>
#include <Math/Temperature.h>

namespace Registry{
void RegisterEditorVoxels(){
    using namespace Registry;
    using namespace Volume;

    VoxelRegistry::RegisterVoxel(
		"Solid",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 1, 1, 1)
			.SetName("Solid")
			.SetColor(RGBA(255, 255, 255, 255))
			.Build()
	);
}
}
