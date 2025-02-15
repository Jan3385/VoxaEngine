#include "Voxel.h"
#include "Chunk.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace Volume;

VoxelElement::VoxelElement()
	:id("Oxygen")
{
	this->properties = VoxelRegistry::GetProperties("Oxygen");
	this->position = Vec2i(0, 0);
	this->temperature = Temperature(21);
}

VoxelElement::VoxelElement(std::string id, Vec2i position, Temperature temperature)
	:id(id), position(position)
{
	this->properties = VoxelRegistry::GetProperties(id);
	this->temperature = temperature;

	srand(position.getX() + position.getY() * rand()); // temp fix

	//tint the color factor 1 to 1.2
	float factor = 1 + ((rand() % 20) / 100.0f);
	this->color = RGBA(
		std::clamp(static_cast<int>(this->properties->pColor.r * factor), 0, 255),
		std::clamp(static_cast<int>(this->properties->pColor.g * factor), 0, 255),
		std::clamp(static_cast<int>(this->properties->pColor.b * factor), 0, 255),
		std::clamp(static_cast<int>(this->properties->pColor.a), 0, 255)
	);
}

VoxelElement::~VoxelElement()
{
}

bool VoxelElement::CheckTransitionTemps(ChunkMatrix& matrix) {	
	if(this->properties->HeatedChange.has_value()){
		if (this->temperature.GetCelsius() > 
			this->properties->HeatedChange.value().TemperatureAt.GetCelsius() + Volume::TEMP_TRANSITION_THRESHOLD)
    	{
    		this->DieAndReplace(matrix, this->properties->HeatedChange.value().To);
    		return true;
    	}
	}
	if(this->properties->CooledChange.has_value()){
		if (this->temperature.GetCelsius() < 
			this->properties->CooledChange.value().TemperatureAt.GetCelsius() - Volume::TEMP_TRANSITION_THRESHOLD)
    	{
			this->DieAndReplace(matrix, this->properties->CooledChange.value().To);
    		return true;
    	}
	}
	return false;
}

void VoxelElement::Swap(Vec2i &toSwapPos, ChunkMatrix &matrix)
{
    // Get the pointer to the voxel at the swap position
    VoxelElement *swapVoxel = matrix.VirtualGetAt(toSwapPos);

    if (!swapVoxel) return;

	//transfer heat between the two voxels
	float heatDifference = this->temperature.GetCelsius() - swapVoxel->temperature.GetCelsius();
	float maxHeatTransfer = heatDifference * 0.6f; // Limit to 60%
	float heatTransfer = std::clamp(
		heatDifference * this->properties->HeatConductivity * 5,
		-maxHeatTransfer,
		maxHeatTransfer
	);
	this->temperature.SetCelsius(this->temperature.GetCelsius() - heatTransfer / this->properties->HeatCapacity);
	swapVoxel->temperature.SetCelsius(swapVoxel->temperature.GetCelsius() + heatTransfer / swapVoxel->properties->HeatCapacity);

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
    //matrix.VirtualSetAt(replacement);
	matrix.PlaceVoxelAt(this->position, id, this->temperature, false);
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

VoxelParticle::VoxelParticle()
	: VoxelElement("Oxygen", Vec2i(0, 0), Temperature(21)), fPosition(Vec2f(0,0)), angle(0), speed(0),
	m_dPosition(0,0) { }

VoxelParticle::VoxelParticle(std::string id, const Vec2i &position, Temperature temp, float angle, float speed)
	: VoxelElement(id, position, temp), fPosition(Vec2f(position)), angle(angle), speed(speed),
	m_dPosition(speed* cos(angle), speed* sin(angle)) { }

bool VoxelParticle::Step(ChunkMatrix *matrix)
{
    updatedThisFrame = true;

    //new position variables
    this->fPosition += m_dPosition;
    Vec2i newPos = Vec2i(fPosition);

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, 0.4f);

    Vec2i futurePos = Vec2i(
    	static_cast<int>(fPosition.getX() + m_dPosition.getX()), 
    	static_cast<int>(fPosition.getY() + m_dPosition.getY())
    );

    VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos);
    if (!futureVoxel || futureVoxel->GetState() == State::Solid || particleIterations <= 0)
    {
    	this->position = newPos;

    	//check if the position for the particle exists
    	if (!matrix->IsValidWorldPosition(this->position))
    	{
    		return true;
    	}

		matrix->PlaceVoxelAt(this->position, this->id, this->temperature, false);
    	return true;
    }

    this->particleIterations--;
    this->position = newPos;

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
    if (StepAlongDirection(matrix, Vec2i(0, 1), Acceleration)) { //if is able to fall down
    	IsFalling = true;
    	Acceleration += 1;

    	//try to set isFalling to true on adjasent voxels - simulates inertia
    	VoxelElement *left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    	VoxelElement *right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    	if (left && left->IsMoveableSolid())
    	{
    		VoxelSolid *leftMovable = dynamic_cast<VoxelSolid*>(left);
    		if ((1 - leftMovable->properties->SolidInertiaResistance) * 1000 > rand() % 1000) {
    			leftMovable->IsFalling = true;
    			leftMovable->XVelocity = 1;
    		}
    	}
    	if (right && right->IsMoveableSolid())
    	{
    		VoxelSolid *rightMovable = dynamic_cast<VoxelSolid*>(right);
    		if ((1 - rightMovable->properties->SolidInertiaResistance) * 1000 > rand() % 1000) {
    			rightMovable->IsFalling = true;
    			rightMovable->XVelocity = 1;
    		}
    	}

    	return true;
    }
    else if (IsFalling) { //On the frame of the impact
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
    	if (StepAlongDirection(matrix, Vec2i(-1, 0), XVelocity)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}
    	if (StepAlongDirection(matrix, Vec2i(1, 0), XVelocity)){
			TryToMoveVoxelBelow(matrix);
			return true;
		}


		VoxelElement *below = matrix->VirtualGetAt(this->position + Vec2i(0, 1));
    	if (below && below->IsMoveableSolid())
    	{
    		VoxelSolid *belowMovable = dynamic_cast<VoxelSolid*>(below);
    		if ((1 - belowMovable->properties->SolidInertiaResistance) * 1000 > rand() % 1000) {
    			belowMovable->IsFalling = true;
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
    if (side && VoxelRegistry::CanBeMovedBySolid(side->GetState()))
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
    VoxelElement* below = matrix->VirtualGetAt(this->position + Vec2i(0, 1));
    if (below && below->IsMoveableSolid())
    {
    	VoxelSolid* belowMovable = dynamic_cast<VoxelSolid*>(below);
    	if ((1 - belowMovable->properties->SolidInertiaResistance) * 1000 > rand() % 1000) {
    		belowMovable->IsFalling = true;
    		belowMovable->XVelocity = 1;
    	}
    }
}

void VoxelSolid::StopFalling()
{
    XVelocity = ((Acceleration / 6) / (rand()%2 + 1)) + 1;
    IsFalling = false;
    Acceleration = 1;
}

bool VoxelLiquid::Step(ChunkMatrix *matrix)
{
	//TODO: make water only move when theres water above or below it
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;

    //falling down + acceleration
    if (StepAlongDirection(matrix, Vec2i(0, 1), Acceleration)) {
    	IsFalling = true;
    	Acceleration += 1;
    	return true;
    }
    else if (IsFalling) {
    	IsFalling = false;
    	Acceleration = 1;
    }

    //If the voxel below is a solid, try to move to the bottom left and bottom right
    VoxelElement* left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    VoxelElement* right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    if (left && (VoxelRegistry::CanBeMovedByLiquid(left->GetState()) || left->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	this->Swap(left->position, *matrix);
    	return true;
    }
    else if (right && (VoxelRegistry::CanBeMovedByLiquid(right->GetState()) || right->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	this->Swap(right->position, *matrix);
    	return true;
    }

    //if there is the same liquid voxel above, skip
    VoxelElement* above = matrix->VirtualGetAt(this->position + Vec2i(0, -1));
    VoxelElement* moreAbove = matrix->VirtualGetAt(this->position + Vec2i(0, -2));
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
    if (next && (VoxelRegistry::CanBeMovedByLiquid(next->GetState()) || next->IsStateBelowDensity(this->GetState(), this->properties->Density)))
    {
    	Vec2i nextPos = next->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		nextPos += direction;
    		next = matrix->VirtualGetAt(nextPos);
			//if the next voxel is a solid, stop
    		if (!next || !(VoxelRegistry::CanBeMovedByLiquid(next->GetState()) || next->IsStateBelowDensity(this->GetState(), this->properties->Density)))
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
    	CurrentPosition += Vec2i(-1, 0);
    	VoxelElement* side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
		if (side->GetState() == State::Solid) break;
    	if (VoxelRegistry::CanBeMovedByLiquid(side->GetState()) || side->IsStateBelowDensity(this->GetState(), this->properties->Density))
		{
			LastValidPosition = CurrentPosition;
		}
    }
    if (LastValidPosition != StartPosition) return LastValidPosition;
    CurrentPosition = StartPosition;
    for (short int i = 0; i < length; ++i)
    {
    	CurrentPosition += Vec2i(1, 0);
    	VoxelElement* side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
    	if (side->GetState() == State::Solid) break;
    	if (VoxelRegistry::CanBeMovedByLiquid(side->GetState()) || side->IsStateBelowDensity(this->GetState(), this->properties->Density))
    	{
    		LastValidPosition = CurrentPosition;
    	}
    }
    return LastValidPosition;
}

bool VoxelGas::Step(ChunkMatrix *matrix)
{
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;

	//try to move up
	if (MoveInDirection(matrix, Vec2i(0, -1)))
		return true;
	//try to fall down
	if (MoveInDirection(matrix, Vec2i(0, 1)))
		return true;
	//try to randomly move to the sides
	bool positiveX = rand() % 2;
	if(this->StepAlongSide(matrix, positiveX, rand()%3 + 2))
		return true;

    return false;
}

bool Volume::VoxelGas::MoveInDirection(ChunkMatrix *matrix, Vec2i direction)
{
    VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + direction); // gasses cannot load new chunks

	if(next == nullptr) return false; // if the next voxel is null, stop

	if(next->properties == this->properties) return false; // if the next voxel is the same, stop
	if(next->GetState() != State::Gas) return false; // if the next voxel is not gas, stop
	
	if(direction.getY() > 0){
		// down
		if(next->IsStateBelowDensity(this->GetState(), this->properties->Density) && !next->updatedThisFrame){
			this->Swap(next->position, *matrix);
			return true;
		}
	}else if(direction.getY() < 0){
		// up
		if(next->IsStateAboveDensity(this->GetState(), this->properties->Density)){
			this->Swap(next->position, *matrix);
			return true;
		}
	}else{
		// sides
		//this->Swap(next->position, *matrix);
		this->StepAlongSide(matrix, direction.getX() > 0, this->properties->FluidDispursionRate);
		return true;
	}
    
    return false;
}

bool Volume::VoxelGas::StepAlongSide(ChunkMatrix *matrix, bool positiveX, short int length)
{
	Vec2i direction = positiveX ? Vec2i(1, 0) : Vec2i(-1, 0);
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
