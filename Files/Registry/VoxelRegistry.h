#pragma once

#include <unordered_map>
#include <string>
#include <optional>
#include "../Math/Color.h"
#include "../Math/Temperature.h"

struct PhaseChange{
		Volume::Temperature TemperatureAt;
		std::string To;
};

namespace Volume{
    static constexpr float TEMP_TRANSITION_THRESHOLD = 1.5f;
    enum class VoxelState {
		Gas,
		Liquid,
		MovableSolid,
		ImmovableSolid,
	};
	enum class State {
		Gas,
		Liquid,
		Solid,
	};
	struct VoxelProperty {
		std::string name;
		State state;
		RGB pColor;
		float Density; // g/L or kg/m^3
		float HeatCapacity; // J/kg*K
		float HeatConductivity; // W/m*K
		std::optional<PhaseChange> CooledChange;
		std::optional<PhaseChange> HeatedChange;
	};
}

class VoxelBuilder{
public:
	VoxelBuilder(Volume::State State, float tCapacity, float tConductivity, float Density);
	VoxelBuilder& SetName(std::string Name);
	VoxelBuilder& SetColor(RGB Color);
	VoxelBuilder& PhaseUp(std::string To, float Temperature);
	VoxelBuilder& PhaseDown(std::string To, float Temperature);
	Volume::VoxelProperty Build();
private:
	Volume::State State;
	std::string Name = "NONAME";
	RGB Color;
	float Density;
	float HeatCapacity;
	float HeatConductivity;
	std::optional<PhaseChange> CooledChange;
	std::optional<PhaseChange> HeatedChange;
};

class VoxelRegistry {
public:
	static Volume::VoxelProperty* GetProperties(std::string id);
	static const bool CanGetMovedByExplosion(Volume::VoxelState state);
	static const bool CanGetDestroyedByExplosion(std::string id, float explosionPower);
	static const bool CanBeMovedBySolid(Volume::VoxelState state);
	static const bool CanBeMovedByLiquid(Volume::VoxelState state);

	static void RegisterVoxel(const std::string& name, const Volume::VoxelProperty property);
	static void RegisterVoxels();
	static std::unordered_map<std::string, Volume::VoxelProperty> registry;
private:
	//static std::unordered_map<Volume::VoxelType, Volume::VoxelProperty> voxelProperties;
};