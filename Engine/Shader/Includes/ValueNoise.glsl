uint Hash(uvec2 p){
    p = p * 1664525u + 1013904223u;
    p.x ^= p.y >> 16;
    p.y ^= p.x << 16;
    return p.x ^ p.y;
}
float ValueNoise(uvec2 p) {
    return float(Hash(p) & 0xFFFFu) / float(0xFFFFu);
}