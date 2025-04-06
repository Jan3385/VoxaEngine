#include "voxelTypes.h"

#include <iostream>
#include "Chunk.h"

const RGBA Volume::FireVoxel::fireColors[8] = {
    RGBA(237, 31, 10, 210),
    RGBA(255, 87, 34, 210),
    RGBA(255, 154, 0, 210),
    RGBA(255, 144, 10, 210),
    RGBA(199, 75, 75, 210),
    RGBA(255, 69, 0, 210),
    RGBA(255, 140, 0, 210),
    RGBA(255, 165, 0, 210)
};

Volume::FireVoxel::FireVoxel(Vec2i position, Temperature temp, float pressure) : VoxelGas("Fire", position, temp, pressure){ }

// Spread the fire to adjacent voxels, returns true if this fire voxel is near oxygen
bool Volume::FireVoxel::Spread(ChunkMatrix *matrix, const VoxelElement *FireVoxel)
{
    //check for oxygen and spread
    bool isAroundOxygen = false;
    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(FireVoxel->position + dir);
        if(next){
            if(next->id == "Oxygen"){
                isAroundOxygen = true;
                break;
            }
            //ignite based on Flamability
            if((rand()%256) - next->properties->Flamability < 0){
                bool nextHasOxygen = false;
                for(Vec2i dir2 : vector::AROUND8){
                    VoxelElement* next2 = matrix->VirtualGetAt_NoLoad(FireVoxel->position + dir + dir2);
                    if(next2 && next2->id == "Oxygen"){
                        nextHasOxygen = true;
                        break;
                    }
                }
                //20% chance to ignite if there is no oxygen around
                if(nextHasOxygen || rand() % 100 < 20){
                    Temperature temp = FireVoxel->temperature;
                    if(temp.GetCelsius() < 250) 
                        temp.SetCelsius(250);

                    if(next->GetState() == State::Gas)
                        matrix->PlaceVoxelAt(FireVoxel->position + dir, "Fire", temp, true, next->amount, true);
                    else if(next->GetState() == State::Liquid)
                        matrix->PlaceVoxelAt(FireVoxel->position + dir, "Fire_Liquid", temp, true, next->amount, true);
                    else
                        matrix->PlaceVoxelAt(FireVoxel->position + dir, "Fire_Solid", temp, true, next->amount, true);
                }
            }
        }
    }
    return isAroundOxygen;
}

bool Volume::FireVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = Volume::FireVoxel::Spread(matrix, this);

    //flame burns faster without oxygen
    float amountChange;
    if(isAroundOxygen){
        forcedLifetimeTime--;
        amountChange = this->amount * 0.15f;
    }
    else{
        forcedLifetimeTime = std::max(forcedLifetimeTime - 3, 0);
        amountChange = this->amount * 0.25f;
    }

    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + dir);
        if(next && next->GetState() == State::Gas){
            this->amount -= amountChange;
            matrix->PlaceVoxelAt(this->position + dir, "Carbon_Dioxide", this->temperature, false, amountChange, false);
            break;
        }
    }

    //set random fire color
    this->color = fireColors[rand() % fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    //15% chance to dissapear
    if (rand() % 100 < 15 || forcedLifetimeTime <= 0 || this->amount <= 0)
    {
        if(this->amount <= 0)
            this->amount = 0.1f;
        
        if(this->amount > 8)
            this->amount = 8;
        
    	this->DieAndReplace(*matrix, "Carbon_Dioxide");
        return true;
    }
    VoxelGas::Step(matrix);
    return true;
}

Volume::FireLiquidVoxel::FireLiquidVoxel(Vec2i position, Temperature temp, float amount)
    : VoxelLiquid("Fire_Liquid", position, temp, amount)
{
}

bool Volume::FireLiquidVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = Volume::FireVoxel::Spread(matrix, this);

    //flame burns faster with oxygen
    float amountChange;
    if(isAroundOxygen)
        amountChange = (this->amount * 0.0001f) + 0.5;
    else
        amountChange = (this->amount * 0.00005f) + 1;

    amountChange = std::min(amountChange, this->amount); // prevent negative amount

    this->amount -= amountChange;

    float temperatureChangePercent = 0;
    if(this->amount > 0 && amountChange > 0.5f)
        temperatureChangePercent = (amountChange / this->amount);
    
    if(temperatureChangePercent < 0.15f)
        temperatureChangePercent = 0.15f;
    
    this->temperature = Temperature(std::max(this->temperature.GetCelsius(), temperatureChangePercent * 1000));

    //set random fire color
    this->color = Volume::FireVoxel::fireColors[rand() % Volume::FireVoxel::fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    if (this->amount <= 5)
    {
        this->amount = std::max(this->amount, 0.1f);

    	if(rand() % 100 < 10) // 10% chance to turn into ash
            this->DieAndReplace(*matrix, "Ash");
        else
            this->DieAndReplace(*matrix, "Carbon_Dioxide");

        return true;
    }
    VoxelLiquid::Step(matrix);
    return true;
}

Volume::FireSolidVoxel::FireSolidVoxel(Vec2i position, Temperature temp, float amount, bool isStatic)
    : VoxelSolid("Fire_Solid", position, temp, isStatic, amount)
{
}

bool Volume::FireSolidVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = Volume::FireVoxel::Spread(matrix, this);

    //flame burns faster with oxygen
    float amountChange;
    if(isAroundOxygen)
        amountChange = (this->amount * 0.0001f) + 0.5;
    else
        amountChange = (this->amount * 0.00005f) + 1;

    amountChange = std::min(amountChange, this->amount); // prevent negative amount

    this->amount -= amountChange;

    float temperatureChangePercent = 0;
    if(this->amount > 0 && amountChange > 0.5f)
        temperatureChangePercent = (amountChange / this->amount);
    
    if(temperatureChangePercent < 0.15f)
        temperatureChangePercent = 0.15f;
    
    this->temperature = Temperature(std::max(this->temperature.GetCelsius(), temperatureChangePercent * 1000));

    //set random fire color
    this->color = Volume::FireVoxel::fireColors[rand() % Volume::FireVoxel::fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    if (this->amount <= 5)
    {
        this->amount = std::max(this->amount, 0.1f);

        if(rand() % 100 < 10) // 10% chance to turn into ash
            this->DieAndReplace(*matrix, "Ash");
        else
            this->DieAndReplace(*matrix, "Carbon_Dioxide");

        return true;
    }
    VoxelSolid::Step(matrix);
    return true;
}
