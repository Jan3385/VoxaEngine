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
	VoxelRegistry::RegisterVoxel(
		"Oxygen",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 919, 0.026, 1.429)
			.SetName("Oxygen")
			.SetColor(RGBA(15, 15, 15, 50))
			.SetFluidDispersionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Water",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 4186, 0.6f, 2)
			.SetName("Water")
			.SetColor(RGBA(3, 169, 244, 200))
			.SetFluidDispersionRate(10)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Dirt",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 1000, 0.3, 200)
			.SetName("Dirt")
			.SetColor(RGBA(121, 85, 72, 255))
			.SetSolidInertiaResistance(0.7)
			.Build()
	);
}
}
