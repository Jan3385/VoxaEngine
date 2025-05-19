#pragma once

#include <unordered_map>
#include <string>
#include <optional>
#include "Math/Color.h"
#include "Math/Temperature.h"
#include "Math/Vector.h"

struct PhaseChange{
		Volume::Temperature TemperatureAt;
		std::string To;
};

namespace Volume{
	class VoxelElement;
	
    static constexpr float TEMP_TRANSITION_THRESHOLD = 1.5f;
	enum class State {
		Gas,
		Liquid,
		Solid,
	};
	struct VoxelProperty {
		std::string name;
		State state;
		RGBA pColor;
		float Density;              //g/L or kg/m^3
		float HeatCapacity;         //J/kg*K
		float HeatConductivity;     // W/m*K

		std::optional<PhaseChange> CooledChange;
		std::optional<PhaseChange> HeatedChange;
		float SolidInertiaResistance;
		uint8_t FluidDispursionRate;

		uint8_t Flamability = 0; // 0 - 255

		uint32_t id = 0;
	};
}

Volume::VoxelElement* CreateVoxelElement(std::string id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids);

class VoxelBuilder{
public:
	VoxelBuilder(Volume::State State, float tCapacity, float tConductivity, float Density);
	VoxelBuilder& SetName(std::string Name);
	VoxelBuilder& SetColor(RGBA Color);
	VoxelBuilder& PhaseUp(std::string To, float Temperature);
	VoxelBuilder& PhaseDown(std::string To, float Temperature);
	VoxelBuilder& SetSolidInertiaResistance(float resistance);
	VoxelBuilder& SetFluidDispursionRate(uint8_t rate);
	VoxelBuilder& SetFlamability(uint8_t flamability);
	Volume::VoxelProperty Build();
private:
	Volume::State State;
	std::string Name = "NONAME";
	RGBA Color;
	float Density;					//Controls the elevation of the voxel (heavy carbon deoxide will sink below light oxygen)
	float HeatCapacity;				//Controls the ability to hold temperature 
	float HeatConductivity;			//Controls the ability to transfer temperature to other voxels
	std::optional<PhaseChange> CooledChange;
	std::optional<PhaseChange> HeatedChange;
	float SolidInertiaResistance; 	// 0 - 1 (0 being no resistance and 1 being full resistance)
	uint8_t FluidDispursionRate; 	// 0 - 255; how well does the fluid move from side to side (0 being no dispersion and 255 being full dispersion)
	uint8_t Flamability = 0;		// 0 - 255 (0 being no flamability and 255 being full flamability)
};

class VoxelRegistry {
public:
	static Volume::VoxelProperty* GetProperties(std::string id);
	static Volume::VoxelProperty* GetProperties(uint32_t id);
	static bool CanGetMovedByExplosion(Volume::State state);
	static bool CanGetDestroyedByExplosion(std::string id, float explosionPower);
	static bool CanBeMovedBySolid(Volume::State state);
	static bool CanBeMovedByLiquid(Volume::State state);

	static void RegisterVoxel(const std::string& name, Volume::VoxelProperty property);
	static void RegisterVoxels();
	static std::unordered_map<std::string, Volume::VoxelProperty> registry;
	static std::unordered_map<uint32_t, Volume::VoxelProperty*> idRegistry;
private:
	static uint32_t idCounter;
	static bool registriesClosed;
};