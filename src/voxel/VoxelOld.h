#pragma once
#include <cstdint>
#include "Block.h"


inline int BlockLayer(Block b) {
    switch (b) {
    case Block::Grass: return 0;
    case Block::Dirt:  return 1;
    case Block::Stone: return 2;
    case Block::Sand:  return 3;
    case Block::Snow:  return 4;
    case Block::Water: return 5;
    default:           return 0; // Air shouldn't be rendered anyway
    }
}

inline bool IsOpaque(Block b) {
    return b != Block::Air && b != Block::Water;
}

inline bool IsTransparent(Block b) {
    return b == Block::Air || b == Block::Water;
}

inline bool IsSolid(Block b) { return b != Block::Air; }

inline bool ShouldRenderFace(Block self, Block neighbor) {
    if (self == Block::Water) {
        // ONLY exposed water faces (no water-vs-solid coplanar fights)
        return neighbor == Block::Air;
    }
    if (IsOpaque(self)) {
        return !IsOpaque(neighbor); // solid vs air or water
    }
    return false;
}