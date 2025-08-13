#pragma once

#include <unordered_map>
#include <string>
#include <optional>
#include <vector>

#include "GL/glew.h"

#include "Math/Color.h"
#include "Math/Temperature.h"
#include "Math/Vector.h"

namespace Registry{
	struct ChemicalReaction{
		std::string from;
		std::string catalyst;
		std::string to;
		float reactionSpeed;			// range from 0 - 1 where 1 is an instant reaction and 0 is no reaction
		bool preserveCatalyst = true;
		float minTemperatureC = Volume::Temperature::absoluteZero.GetCelsius();
	};
	struct PhaseChange{
		Volume::Temperature temperatureAt;
		std::string to;
	};

	enum class DefaultVoxelConstructor {
		GasVoxel,
		LiquidVoxel,
		SolidVoxel,
		Custom,
	};
}

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
		Registry::DefaultVoxelConstructor Constructor;
		RGBA pColor;
		float Density;              //g/L or kg/m^3
		float HeatCapacity;         //J/kg*K
		float HeatConductivity;     // W/m*K

		std::optional<Registry::PhaseChange> CooledChange;
		std::optional<Registry::PhaseChange> HeatedChange;
		float SolidInertiaResistance;
		uint8_t FluidDispursionRate;

		uint8_t Flamability = 0; // 0 - 255

		uint32_t id = 0;
	};
}

Volume::VoxelElement* CreateVoxelElement(std::string id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids);
Volume::VoxelElement* CreateVoxelElement(uint32_t id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids);
Volume::VoxelElement* CreateVoxelElement(Volume::VoxelProperty* property, std::string id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids);

namespace Registry{
	class VoxelBuilder{
	public:
		VoxelBuilder(DefaultVoxelConstructor Constructor, float tCapacity, float tConductivity, float Density);
		VoxelBuilder& SetName(std::string Name);
		VoxelBuilder& SetColor(RGBA Color);
		VoxelBuilder& PhaseUp(std::string To, float Temperature);
		VoxelBuilder& PhaseDown(std::string To, float Temperature);
		VoxelBuilder& Reaction(std::string To, std::string Catalyst, float ReactionSpeed, bool PreserveCatalyst = true, float MinTemperatureC = Volume::Temperature::absoluteZero.GetCelsius());
		VoxelBuilder& SetSolidInertiaResistance(float resistance);
		VoxelBuilder& SetFluidDispursionRate(uint8_t rate);
		VoxelBuilder& SetFlamability(uint8_t flamability);
		Volume::VoxelProperty Build();
	private:
		DefaultVoxelConstructor Constructor;
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
		static std::string GetStringID(uint32_t id);
		static bool CanGetMovedByExplosion(Volume::State state);
		static bool CanGetDestroyedByExplosion(std::string id, float explosionPower);
		static bool CanBeMovedBySolid(Volume::State state);
		static bool CanBeMovedByLiquid(Volume::State state);

		static void RegisterVoxel(const std::string& name, Volume::VoxelProperty property);
		static void RegisterReaction(Registry::ChemicalReaction reaction);
		static void RegisterVoxels();
		static void CloseRegistry();
		static std::unordered_map<std::string, Volume::VoxelProperty> registry;
		static std::unordered_map<uint32_t, Volume::VoxelProperty*> idRegistry;

		/// Data stored as following:
		/// uint32_t fromID;
		/// uint32_t catalystID;
		/// uint32_t toID;
		/// float reactionSpeed;
		/// bool preserveCatalyst;
		/// float minTemperatureC;
		static GLuint chemicalReactionsBuffer;
		static std::vector<Registry::ChemicalReaction> reactionRegistry;  // cleared after inserted into a OpenGL buffer
	private:
		static uint32_t idCounter;
		static bool registryClosed;
	};
}