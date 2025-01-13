#pragma once

#include <map>
#include <memory>
#include <string>
#include "../Math/Vector.h"
#include "../Math/Color.h"

// Forward declaration of ChunkMatrix
class ChunkMatrix;

namespace Volume {
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
	};
	enum VoxelState {
		Gas,
		Liquid,
		ImmovableSolid,
		MovableSolid,
	};
	struct Temperature {
	private:
		float Temperature_C;  // Temperature in Celsius

		static constexpr float KELVIN_OFFSET = 273.15f;
		static constexpr float FAHRENHEIT_OFFSET = 32.0f;
		static constexpr float FAHRENHEIT_RATIO = 9.0f / 5.0f;
		static constexpr float FAHRENHEIT_INVERSE_RATIO = 5.0f / 9.0f;
	public:
		float GetKelvin() const { return Temperature_C + KELVIN_OFFSET; }
		void SetKelvin(float kelvin) { Temperature_C = kelvin - KELVIN_OFFSET; }
		float GetFahrenheit() const { return Temperature_C * FAHRENHEIT_RATIO + FAHRENHEIT_OFFSET; }
		void SetFahrenheit(float fahrenheit) { Temperature_C = (fahrenheit - FAHRENHEIT_OFFSET) * FAHRENHEIT_INVERSE_RATIO; }
		float GetCelsius() const { return Temperature_C; }
		void SetCelsius(float celsius) { Temperature_C = celsius; }

		Temperature() : Temperature_C(0) {}
		Temperature(float celsius) : Temperature_C(celsius) {}
	};
	struct VoxelProperty {
		const std::string name;
		const RGB pColor;
		const Volume::Temperature SolidToLiquidTemp_C;
		const Volume::Temperature LiquidToGasTemp_C;
		//g/L or kg/m^3
		const float Density;
	};
	//Interfaces
	class IGravity {
	public:
		virtual ~IGravity(){ };
		short int Acceleration = 1;
		bool IsFalling = false;
	};
	//Base class for all voxel elements
	class VoxelElement : public std::enable_shared_from_this<VoxelElement>
	{
	public:
		VoxelElement();
		VoxelElement(VoxelType type, Vec2i position);
		virtual ~VoxelElement();

		const VoxelType type;
		const VoxelProperty* properties = nullptr;
		Vec2i position;
		RGB color;
		Temperature temperature = Temperature(21.f);
		bool updatedThisFrame = false; //TODO: maybe set to true

		// Functions
		//return the state of the element
		virtual VoxelState GetState() { return VoxelState::ImmovableSolid; };
		//returm true if the voxel moved
		virtual bool Step(ChunkMatrix* matrix) { updatedThisFrame = true; return false; };
		//return true if the voxel acted on another voxel
		virtual bool ActOnAnother() { return false; };
		//return true if the voxel transitioned to another state
		virtual bool CheckTransitionTemps(ChunkMatrix& matrix);

		// Swap the voxel with another voxel
		void Swap(Vec2i& toSwapPos,ChunkMatrix& matrix);
		void DieAndReplace(ChunkMatrix& matrix, std::shared_ptr<VoxelElement> replacement);
	private:
		Volume::VoxelState state = VoxelState::ImmovableSolid;
	};

	class VoxelParticle : public VoxelElement {
	public:
		VoxelParticle();
		VoxelParticle(VoxelType type,const Vec2i& position, float angle, float speed);

		Vec2f fPosition;

		//Angle in radians
		float angle = 0;
		float speed = 0;
		short int particleIterations = 50;
		
		bool Step(ChunkMatrix* matrix) override;
	private: 
		//Particle variables
		Vec2f m_dPosition;
	};

	//Solid Voxels -> inherit from base voxel class
	class VoxelSolid : public VoxelElement {
	public:
		VoxelSolid() : VoxelElement(VoxelType::Dirt, Vec2i(0, 0)) {};
		VoxelSolid(VoxelType type, Vec2i position) : VoxelElement(type, position) {};
		virtual ~VoxelSolid() {};

		virtual VoxelState GetState() override { return VoxelState::ImmovableSolid; };
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;
	};
	//Solid immovable voxels -> inherit from solid voxels
	class VoxelImmovableSolid : public VoxelSolid {
	public:
		VoxelImmovableSolid() : VoxelSolid(VoxelType::Dirt, Vec2i(0, 0)) {};
		VoxelImmovableSolid(VoxelType type, Vec2i position) : VoxelSolid(type, position) {};
		~VoxelImmovableSolid() {};

		VoxelState GetState() override { return VoxelState::ImmovableSolid; };
		bool Step(ChunkMatrix* matrix) override { updatedThisFrame = true; return false; };
	};
	//solid movable voxels -> inherit from solid voxels
	class VoxelMovableSolid : public VoxelSolid, public IGravity {
	public:
		VoxelMovableSolid() : VoxelSolid(VoxelType::Sand, Vec2i(0, 0)) {};;
		VoxelMovableSolid(VoxelType type, Vec2i position) : VoxelSolid(type, position) {};;
		~VoxelMovableSolid() {};

		short unsigned int XVelocity = 0;     // 0 - short unsigned int max
		float InertiaResistance = 0; //0 - 1

		VoxelState GetState() override { return VoxelState::MovableSolid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
	private:
		void StopFalling();
	};
	//liquid voxels -> inherit from base voxel class
	class VoxelLiquid : public VoxelElement, public IGravity {
	public:
		VoxelLiquid() : VoxelElement(VoxelType::Water, Vec2i(0, 0)) {};
		VoxelLiquid(VoxelType type, Vec2i position) : VoxelElement(type, position) {};
		~VoxelLiquid() {};

		VoxelState GetState() override { return VoxelState::Liquid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
		Vec2i GetValidSideSwapPosition(ChunkMatrix& matrix, short int length);
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;

		short int dispursionRate = 10;
	};
	//Gas voxels -> inherit from base voxel class
	struct VoxelGas : public VoxelElement {
	public:
		VoxelGas() : VoxelElement(VoxelType::Oxygen, Vec2i(0, 0)) {};
		VoxelGas(VoxelType type, Vec2i position) : VoxelElement(type, position) {};
		~VoxelGas() {};

		VoxelState GetState() override { return VoxelState::Gas; };
		bool Step(ChunkMatrix* matrix) override;
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;
	};
}
class VoxelRegistry {
public:
	static const Volume::VoxelProperty& GetProperties(Volume::VoxelType type);
	static const bool CanGetMovedByExplosion(Volume::VoxelState state);
	static const bool CanGetDestroyedByExplosion(Volume::VoxelElement& element, float explosionPower);
	static const bool CanBeMovedBySolid(Volume::VoxelState state);
	static const bool CanBeMovedByLiquid(Volume::VoxelState state);
private:
	static const std::map<Volume::VoxelType, Volume::VoxelProperty> voxelProperties;
};
