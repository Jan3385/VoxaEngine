#include "Directions.glsl"

uint GetVoxelIndex(uint x, uint y, uint chunk) {
    return chunk * CHUNK_SIZE_SQUARED + y * CHUNK_SIZE + x;
}

// output uint(-1) means bad transfer, skip
uint GetIndexFromLocalPositionAtChunk(ivec2 localPos, uint chunk, uint numberOfChunks) {
    // forbid diagonals
    if((localPos.x < 0 || localPos.x >= CHUNK_SIZE) && (localPos.y < 0 || localPos.y >= CHUNK_SIZE))
        return uint(-1);

    uint index = 0;
    
    // if out of bounds from current chunk
    if(localPos.x < 0 || localPos.x >= CHUNK_SIZE || localPos.y < 0 || localPos.y >= CHUNK_SIZE) {
        int nC = -1;
        if(localPos.x < 0){ // left neighbor chunk
            // Find the left neighbor chunk
            for(int j = 0; j < numberOfChunks; ++j){
                if(chunkData[j].chunkRight == chunk){
                    nC = chunkData[j].chunk;
                    break;
                }
            }
            if(nC == -1) return uint(-1);
            // x = CHUNK_SIZE-1 in neighbor, y stays the same
            index = GetVoxelIndex(CHUNK_SIZE - 1, localPos.y, nC);

        }else if (localPos.x >= CHUNK_SIZE){ // right neighbor chunk
            for(int j = 0; j < numberOfChunks; ++j){
                if(chunkData[j].chunkLeft == chunk){
                    nC = chunkData[j].chunk;
                    break;
                }
            }
            if(nC == -1) return uint(-1);
            // x = 0 in neighbor, y stays the same
            index = GetVoxelIndex(0, localPos.y, nC);

        }else if(localPos.y < 0){ // top neighbor chunk
            for(int j = 0; j < numberOfChunks; ++j){
                if(chunkData[j].chunkDown == chunk){
                    nC = chunkData[j].chunk;
                    break;
                }
            }
            if(nC == -1) return uint(-1);
            // y = CHUNK_SIZE-1 in neighbor, x stays the same
            index = GetVoxelIndex(localPos.x, CHUNK_SIZE - 1, nC);

        }else if(localPos.y >= CHUNK_SIZE){ // bottom neighbor chunk
            for(int j = 0; j < numberOfChunks; ++j){
                if(chunkData[j].chunkUp == chunk){
                    nC = chunkData[j].chunk;
                    break;
                }
            }
            if(nC == -1) return uint(-1);
            // y = 0 in neighbor, x stays the same
            index = GetVoxelIndex(localPos.x, 0, nC);

        }
    }else{ // if in bounds
        index = GetVoxelIndex(localPos.x, localPos.y, chunk);
    }

    return index;
}

ivec3 GetLocalPosAndChunkFromOOBLocalPos(ivec2 localPos, uint chunk, uint numberOfChunks) {
    // forbid diagonals
    if((localPos.x < 0 || localPos.x >= CHUNK_SIZE) && (localPos.y < 0 || localPos.y >= CHUNK_SIZE))
        return ivec3(-1, -1, -1);

    int nC = -1;
    if(localPos.x < 0){ // left neighbor chunk
        for(int j = 0; j < numberOfChunks; ++j){
            if(chunkData[j].chunkRight == chunk){
                nC = chunkData[j].chunk;
                break;
            }
        }
        if(nC == -1) return ivec3(0, 0, 0);
        return ivec3(CHUNK_SIZE - 1, localPos.y, nC);

    }else if (localPos.x >= CHUNK_SIZE){ // right neighbor chunk
        for(int j = 0; j < numberOfChunks; ++j){
            if(chunkData[j].chunkLeft == chunk){
                nC = chunkData[j].chunk;
                break;
            }
        }
        if(nC == -1) return ivec3(0, 0, 0);
        return ivec3(0, localPos.y, nC);

    }else if(localPos.y < 0){ // top neighbor chunk
        for(int j = 0; j < numberOfChunks; ++j){
            if(chunkData[j].chunkDown == chunk){
                nC = chunkData[j].chunk;
                break;
            }
        }
        if(nC == -1) return ivec3(0, 0, 0);
        return ivec3(localPos.x, CHUNK_SIZE - 1, nC);

    }else if(localPos.y >= CHUNK_SIZE){ // bottom neighbor chunk
        for(int j = 0; j < numberOfChunks; ++j){
            if(chunkData[j].chunkUp == chunk){
                nC = chunkData[j].chunk;
                break;
            }
        }
        if(nC == -1) return ivec3(0, 0, 0);
        return ivec3(localPos.x, 0, nC);
    }

    return ivec3(localPos.x, localPos.y, chunk); // in bounds
}