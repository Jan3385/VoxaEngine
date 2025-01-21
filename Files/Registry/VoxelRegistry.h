#pragma once

#include <unordered_map>
#include <string>
#include "../Math/Color.h"
#include "../Math/Temperature.h"

namespace Volume{
    static constexpr float TEMP_TRANSITION_THRESHOLD = 1.5f;
    enum VoxelState {
		Gas,
		Liquid,
		ImmovableSolid,
		MovableSolid,
	};
    enum VoxelType {
		Dirt,
		Grass,
		Stone,
		Sand,
		Oxygen,
		Water,
		Fire,
		Plasma,
		CarbonDioxide,
		Lava,
		Steam
	};
    struct VoxelProperty {
		const std::string name;
		const RGB pColor;
		const float Density; // g/L or kg/m^3
		const float HeatCapacity; // J/kg*K
		const float HeatConductivity; // W/m*K
	};
}

class VoxelBuilder{
public:
	VoxelBuilder(Volume::VoxelType Type, Volume::VoxelState DefaultState, float tCapacity, float tConductivity);
	Volume::VoxelType Type;
	VoxelBuilder& SetName(std::string Name);
	VoxelBuilder& SetColor(RGB Color);
	VoxelBuilder& SetDensity(float Density);
	Volume::VoxelProperty Build();
private:
	std::string Name = "UNNAMED TYPE";
	Volume::VoxelState DefaultState;
	RGB Color;
	float Density;
	float HeatCapacity;
	float HeatConductivity;
};

class VoxelRegistry {
public:
	static const Volume::VoxelProperty& GetProperties(Volume::VoxelType type);
	static const bool CanGetMovedByExplosion(Volume::VoxelState state);
	static const bool CanGetDestroyedByExplosion(Volume::VoxelType type, float explosionPower);
	static const bool CanBeMovedBySolid(Volume::VoxelState state);
	static const bool CanBeMovedByLiquid(Volume::VoxelState state);
	static void RegisterVoxelType(VoxelBuilder build);
	static void RegisterVoxels();
private:
	static std::unordered_map<Volume::VoxelType, Volume::VoxelProperty> voxelProperties;
};