#pragma once
#include <cstdint>

enum class Block : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Snow
};

inline bool IsSolid(Block b) { return b != Block::Air; }