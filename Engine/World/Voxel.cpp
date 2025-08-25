#include "World/Voxel.h"
#include "World/ChunkMatrix.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "Voxel.h"

using namespace Volume;

Random Volume::voxelRandomGenerator = Random();

VoxelElement::VoxelElement()
	:id("Oxygen")
{
	this->properties = Registry::VoxelRegistry::GetProperties("Oxygen");
	this->position = vector::ZERO;
	this->temperature = Temperature(21);
}

VoxelElement::VoxelElement(std::string id, Vec2i position, Temperature temperature, float amount)
	:id(id), position(position), amount(amount)
{
	this->properties = Registry::VoxelRegistry::GetProperties(id);
	this->temperature = temperature;

	this->color = this->properties->pColor;

	if(this->properties->TextureMap){
		RGBA tint = (*this->properties->TextureMap)(this->position.x, this->position.y);
		if(tint.a == 0) tint = RGBA(255, 255, 255, 255);

		int8_t alpha = this->color.a;
		this->color = this->color * tint;
		this->color.a = alpha;
	}
	if(this->properties->RandomColorTints){
		//tint the color factor 1 to 1.2
		float factor = Volume::voxelRandomGenerator.GetFloat(1.0f, 1.2f);
		this->color = RGBA(
			std::clamp(static_cast<int>(this->color.r * factor), 0, 255),
			std::clamp(static_cast<int>(this->color.g * factor), 0, 255),
			std::clamp(static_cast<int>(this->color.b * factor), 0, 255),
			std::clamp(static_cast<int>(this->color.a), 0, 255)
		);
	}
}

VoxelElement::~VoxelElement()
{
}

std::string Volume::VoxelElement::ShouldTransitionToID()
{
	float tempC = this->temperature.GetCelsius();

	if(this->properties->HeatedChange.has_value()){
		const auto& heated = this->properties->HeatedChange.value();
		if (tempC > heated.temperatureAt.GetCelsius() + Volume::TEMP_TRANSITION_THRESHOLD)
		{
			return heated.to;
		}
	}

	if(this->properties->CooledChange.has_value()){
		const auto& cooled = this->properties->CooledChange.value();
		if (tempC < cooled.temperatureAt.GetCelsius() - Volume::TEMP_TRANSITION_THRESHOLD)
		{
			return cooled.to;
		}
	}

	return "";
}

void VoxelElement::Swap(Vec2i &toSwapPos, ChunkMatrix &matrix)
{
    // Get the pointer to the voxel at the swap position
    VoxelElement *swapVoxel = matrix.VirtualGetAt(toSwapPos);

    if (!swapVoxel || swapVoxel->id == "Empty") return;

	//transfer heat between the two voxels
	float heatDifference = this->temperature.GetCelsius() - swapVoxel->temperature.GetCelsius();
	float maxHeatTransfer = 500;
	float heatTransfer = std::clamp(
		heatDifference * this->properties->HeatConductivity * 5,
		-maxHeatTransfer,
		maxHeatTransfer
	);

	bool anyHeatCapacityZero = this->properties->HeatCapacity == 0 || swapVoxel->properties->HeatCapacity == 0;
	
	if(!anyHeatCapacityZero){
		this->temperature.SetCelsius(this->temperature.GetCelsius() - heatTransfer / this->properties->HeatCapacity);
		swapVoxel->temperature.SetCelsius(swapVoxel->temperature.GetCelsius() + heatTransfer / swapVoxel->properties->HeatCapacity);
	}

    // Get the position of the current voxel
    Vec2i tempPos = this->position;

    //flip positions
    this->position = toSwapPos;
    swapVoxel->position = tempPos;

    // Swap the current voxel with the one at the swap position
    matrix.VirtualSetAt_NoDelete(this);
    matrix.VirtualSetAt_NoDelete(swapVoxel);
}

void VoxelElement::DieAndReplace(ChunkMatrix &matrix, std::string id)
{
	matrix.PlaceVoxelAt(this->position, id, this->temperature, false, this->amount, true);
}

bool Volume::VoxelElement::IsMoveableSolid()
{
    if(this->GetState() != State::Solid) return false;

	VoxelSolid *solid = dynamic_cast<VoxelSolid*>(this);
	if (solid->isStatic) return false;

	return true;
}

bool Volume::VoxelElement::IsUnmoveableSolid()
{
	if(this->GetState() != State::Solid) return false;

	VoxelSolid *solid = dynamic_cast<VoxelSolid*>(this);
	if (!solid->isStatic) return false;

	return true;
}

bool Volume::VoxelElement::IsStateBelowDensity(State state, float density) const
{
	if (this->GetState() == state && this->properties->Density < density)
	{
		return true;
	}
	return false;
}

bool Volume::VoxelElement::IsStateAboveDensity(State state, float density) const
{
	if (this->GetState() == state && this->properties->Density > density)
	{
		return true;
	}
	return false;
}

bool VoxelSolid::Step(ChunkMatrix *matrix)
{
	if(isStatic){
		updatedThisFrame = true;
		return false;
	}

    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;

    //Fall below and if not falling try to move to the sides

    //falling down + acceleration + setting isFalling to near voxel handling
    if (StepAlongDirection(matrix, vector::DOWN, GetAcceleration())) { //if is able to fall down
    	isFalling = true;
    	IncrementAcceleration(1);

    	//try to set isFalling to true on adjasent voxels - simulates inertia
    	VoxelElement *left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    	VoxelElement *right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    	if (left && left->IsMoveableSolid())
    	{
    		VoxelSolid *leftMovable = dynamic_cast<VoxelSolid*>(left);
    		if ((1 - leftMovable->properties->SolidInertiaResistance) * 1000 > voxelRandomGenerator.GetInt(0, 1000)) {
    			leftMovable->isFalling = true;
    			leftMovable->XVelocity = 1;
    		}
    	}
    	if (right && right->IsMoveableSolid())
    	{
    		VoxelSolid *rightMovable = dynamic_cast<VoxelSolid*>(right);
    		if ((1 - rightMovable->properties->SolidInertiaResistance) * 1000 > voxelRandomGenerator.GetInt(0, 1000)) {
    			rightMovable->isFalling = true;
    			rightMovable->XVelocity = 1;
    		}
    	}

    	return true;
    }
    else if (isFalling) { //On the frame of the impact
    	StopFalling();
    	//If the voxel below is a solid, try to move to the sides

    	if (StepAlongDirection(matrix, Vec2i(-1, 1), 1)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}
    	if (StepAlongDirection(matrix, Vec2i(1, 1), 1)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}
    	if (StepAlongDirection(matrix, vector::LEFT, XVelocity)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}
    	if (StepAlongDirection(matrix, vector::RIGHT, XVelocity)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}


		VoxelElement *below = matrix->VirtualGetAt(this->position + vector::DOWN);
    	if (below && below->IsMoveableSolid())
    	{
    		VoxelSolid *belowMovable = dynamic_cast<VoxelSolid*>(below);
    		if ((1 - belowMovable->properties->SolidInertiaResistance) * 1000 > voxelRandomGenerator.GetInt(0, 1000)) {
    			belowMovable->isFalling = true;
    			belowMovable->XVelocity = 1;
    		}
    	}

    	XVelocity = 0;
    }

    //if pixel have not found a place to move to return false
    return false;
}

bool VoxelSolid::StepAlongDirection(ChunkMatrix *matrix, Vec2i direction, short int length)
{
    VoxelElement* side = matrix->VirtualGetAt(this->position + direction);
    if (side && Registry::VoxelRegistry::CanBeMovedBySolid(side->GetState()))
    {
    	Vec2i sidePos = side->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		sidePos += direction;
    		side = matrix->VirtualGetAt(sidePos);
    		if (!side || (side->GetState() != State::Gas))
    		{
    			sidePos -= direction;
    			this->Swap(sidePos, *matrix);
    			return true;
    		}
    	}
    	this->Swap(sidePos, *matrix);
    	return true;
    }
    return false;
}

void VoxelSolid::TryToMoveVoxelBelow(ChunkMatrix *matrix)
{
	//try to set isFalling to true on voxel below - simulates inertia
    VoxelElement* below = matrix->VirtualGetAt(this->position + vector::DOWN);
    if (below && below->IsMoveableSolid())
    {
    	VoxelSolid* belowMovable = dynamic_cast<VoxelSolid*>(below);
    	if ((1 - belowMovable->properties->SolidInertiaResistance) * 1000 > voxelRandomGenerator.GetInt(0, 1000)) {
    		belowMovable->isFalling = true;
    		belowMovable->XVelocity = 1;
    	}
    }
}

bool Volume::VoxelSolid::ShouldTriggerDirtyColliders()
{
	return true;
}

bool Volume::VoxelSolid::IsSolidCollider() const
{
	if (isFalling) return false;
	
	return true;
}

void VoxelSolid::StopFalling()
{
    XVelocity = ((GetAcceleration() / 6) / voxelRandomGenerator.GetInt(1, 2)) + 1;
    isFalling = false;
    SetAcceleration(1);
}

bool VoxelLiquid::Step(ChunkMatrix *matrix)
{
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }

    updatedThisFrame = true;

    //falling down + acceleration
    if (StepAlongDirection(matrix, vector::DOWN, GetAcceleration())) {
    	isFalling = true;
		IncrementAcceleration(1);
    	return true;
    }
    else if (isFalling) { //On the frame of the impact (stops falling)
    	isFalling = false;
		SetAcceleration(1);
    }

	VoxelElement* above = matrix->VirtualGetAt(this->position + vector::UP);
	
	// if the liquid is above its avalible density, expand
	if(this->amount > VoxelLiquid::DesiredDensity){
		if(above && above->GetState() == State::Gas){
			matrix->PlaceVoxelAt(this->position+vector::UP, this->id, this->temperature, false, this->amount/2, false);
			this->amount /= 2;
			return true;
		}else if(above && above->properties == this->properties){
			float missingAmount = std::max(VoxelLiquid::DesiredDensity - above->amount, 0.0f);
			float transferAmount = std::min(missingAmount, this->amount);
			above->amount += transferAmount;
			this->amount -= transferAmount;

			if(missingAmount > 0.0f){
				Vec2i localPos = Vec2i(this->position.x % Chunk::CHUNK_SIZE, this->position.y % Chunk::CHUNK_SIZE);
				matrix->GetChunkAtWorldPosition(this->position)->dirtyRect.Include(localPos);
				return true;
			}
		}
	}

	VoxelElement* below = matrix->VirtualGetAt(this->position + Vec2i(0, 1));
	if(below && below->GetState() == State::Liquid && below->properties == this->properties){
		if(below->amount < VoxelLiquid::DesiredDensity){
			float missingAmount = VoxelLiquid::DesiredDensity - below->amount;
			float transferAmount = std::min(missingAmount, this->amount);
			below->amount += transferAmount;
			this->amount -= transferAmount;

			if(this->amount <= 0){
				if(above->GetState() == State::Gas){
					DieAndReplace(*matrix, above->id);
				}else{
					DieAndReplace(*matrix, "Empty");
				}
			}
			Vec2i localPos = Vec2i(this->position.x % Chunk::CHUNK_SIZE, this->position.y % Chunk::CHUNK_SIZE);
			
			matrix->GetChunkAtWorldPosition(this->position)->dirtyRect.Include(localPos);
			return true;
		}
	}

    //If the voxel below is a solid, try to move to the bottom left and bottom right
    VoxelElement* left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    VoxelElement* right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    if (left && (Registry::VoxelRegistry::CanBeMovedByLiquid(left->GetState()) || left->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	this->Swap(left->position, *matrix);
    	return true;
    }
    else if (right && (Registry::VoxelRegistry::CanBeMovedByLiquid(right->GetState()) || right->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	this->Swap(right->position, *matrix);
    	return true;
    }

    //if there is the same liquid voxel above, skip
    VoxelElement* moreAbove = matrix->VirtualGetAt(this->position + (vector::UP * 2));
    if (above && above->properties == this->properties && moreAbove && moreAbove->properties == this->properties)
    	return false;

    Vec2i MovePosition = GetValidSideSwapPosition(*matrix, this->properties->FluidDispursionRate);
    if (MovePosition != this->position) {
    	this->Swap(MovePosition, *matrix);
    	return true;
    }
    //if pixel have not found a place to move to return false
    return false;
}

bool VoxelLiquid::StepAlongDirection(ChunkMatrix *matrix, Vec2i direction, short int length)
{
    VoxelElement* next = matrix->VirtualGetAt(this->position + direction);
    if (next && (Registry::VoxelRegistry::CanBeMovedByLiquid(next->GetState()) || next->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	Vec2i nextPos = next->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		nextPos += direction;
    		next = matrix->VirtualGetAt(nextPos);
			//if the next voxel is a solid, stop
    		if (!next || !(Registry::VoxelRegistry::CanBeMovedByLiquid(next->GetState()) || next->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    		{
    			nextPos -= direction;
    			this->Swap(nextPos, *matrix);
    			return true;
    		}
    	}
    	this->Swap(nextPos, *matrix);
    	return true;
    }
    return false;
}

Vec2i VoxelLiquid::GetValidSideSwapPosition(ChunkMatrix &matrix, short int length)
{
    Vec2i LastValidPosition = this->position;
    Vec2i StartPosition = this->position;
    Vec2i CurrentPosition = this->position;

    //try to move to the sides
    for (short int i = 0; i < length; ++i)
    {
    	CurrentPosition += vector::LEFT;
    	VoxelElement* side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
		if (side->GetState() == State::Solid) break;
    	if (Registry::VoxelRegistry::CanBeMovedByLiquid(side->GetState()) || side->IsStateBelowDensity(this->GetState(), this->properties->Density))
		{
			LastValidPosition = CurrentPosition;
		}
    }
    if (LastValidPosition != StartPosition) return LastValidPosition;
    CurrentPosition = StartPosition;
    for (short int i = 0; i < length; ++i)
    {
    	CurrentPosition += vector::RIGHT;
    	VoxelElement* side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
    	if (side->GetState() == State::Solid) break;
    	if (Registry::VoxelRegistry::CanBeMovedByLiquid(side->GetState()) || side->IsStateBelowDensity(this->GetState(), this->properties->Density))
    	{
    		LastValidPosition = CurrentPosition;
    	}
    }
    return LastValidPosition;
}

Volume::VoxelGas::VoxelGas(std::string id, Vec2i position, Temperature temp, float amount)
: VoxelElement(id, position, temp, amount)
{
}

bool VoxelGas::Step(ChunkMatrix *matrix)
{
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;

	// Create vacuum in low enough amounts
	if(this->amount < VoxelGas::MinimumGasAmount){
		this->DieAndReplace(*matrix, "Empty");
		return true;
	}

	//look around and try to spread based on pressure
	for(Vec2i dir : vector::AROUND4){
		VoxelElement* next = matrix->VirtualGetAt(this->position + dir);
		if(next && next->GetState() == State::Gas && this->properties != next->properties){
			float nextAmount = next->amount;
			if(this->amount - nextAmount > 0.3f){
				// small amount of gas deletion happens.. no idea why
				if(matrix->TryToDisplaceGas(this->position + dir, this->id, this->temperature, this->amount - nextAmount, false)){
					this->amount = nextAmount;
					break;
				}
			}
		}
	}

	//try to move up
	if (MoveInDirection(matrix, vector::UP))
		return true;
	//try to fall down
	if (MoveInDirection(matrix, vector::DOWN))
		return true;

	//try to randomly move to the sides
	Vec2i direction = voxelRandomGenerator.GetBool() ? vector::RIGHT : vector::LEFT;
	if(MoveInDirection(matrix, direction))
		return true;

    return false;
}

bool Volume::VoxelGas::MoveInDirection(ChunkMatrix *matrix, Vec2i direction)
{
    VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + direction); // gasses cannot load new chunks

	if(next == nullptr) return false; // if the next voxel is null, stop

	if(next->properties == this->properties) return false; // if the next voxel is the same, stop
	
	if(next->GetState() != State::Gas) return false; // if the next voxel is not gas, stop
	
	if(direction.y > 0){
		// down
		if(next->IsStateBelowDensity(this->GetState(), this->properties->Density) && !next->updatedThisFrame){
			this->Swap(next->position, *matrix);
			return true;
		}
	}else if(direction.y < 0){
		// up
		if(next->IsStateAboveDensity(this->GetState(), this->properties->Density)){
			this->Swap(next->position, *matrix);
			return true;
		}
	}else{
		// sides
		this->StepAlongSide(matrix, direction.x > 0, this->properties->FluidDispursionRate);
		return true;
	}
    
    return false;
}

bool Volume::VoxelGas::StepAlongSide(ChunkMatrix *matrix, bool positiveX, short int length)
{
	Vec2i direction = positiveX ? vector::RIGHT : vector::LEFT;
    VoxelElement* next = matrix->VirtualGetAt(this->position + direction);
    if (next && next->GetState() == State::Gas && next->properties != this->properties)
    {
    	Vec2i nextPos = next->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		nextPos += direction;
    		next = matrix->VirtualGetAt(nextPos);
			//if the next voxel is a solid, stop
    		if (next && (next->GetState() != State::Gas || next->properties == this->properties))
    		{
    			nextPos -= direction;
    			this->Swap(nextPos, *matrix);
    			return true;
    		}
    	}
    	this->Swap(nextPos, *matrix);
    	return true;
    }
    return false;
}

int Volume::GetLiquidVoxelPercentile(std::vector<VoxelElement *> voxels)
{
	if(voxels.empty()) return 0;
	int liquidCount = 0;
	for (const auto& voxel : voxels)
	{
		if (voxel->GetState() == State::Liquid)
			liquidCount++;
	}
	return (liquidCount * 100) / voxels.size();
}

void Volume::IGravity::SetAcceleration(short int acceleration)
{
	this->Acceleration = acceleration;
	if (this->Acceleration > MAX_ACCELERATION) {
		this->Acceleration = MAX_ACCELERATION; // prevent too high acceleration
	}
}

void Volume::IGravity::IncrementAcceleration(short int amount)
{
	this->Acceleration += amount;
	if (this->Acceleration > MAX_ACCELERATION) {
		this->Acceleration = MAX_ACCELERATION; // prevent too high acceleration
	}
}
