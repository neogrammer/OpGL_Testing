#pragma once
#include <vector>
#include <glad/glad.h>
#include <string>

namespace util
{
    extern GLuint LoadTexture2DArray(const std::vector<std::string>& paths, int& outW, int& outH);
    
}