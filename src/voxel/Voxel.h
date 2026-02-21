#pragma once
#include <cstdint>

enum class Block : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Snow,
    Water
};

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
        // Render water face if it touches air or opaque blocks (shorelines / surface)
        return neighbor == Block::Air || IsOpaque(neighbor);
    }
    if (IsOpaque(self)) {
        // Render solid face if neighbor is NOT opaque (air or water)
        return !IsOpaque(neighbor);
    }
    return false; // Air never renders faces
}