#include "TextureUtils.h"

#include <stb_image.h>
#include <iostream>
#include <GLFW/glfw3.h>

GLuint util::LoadTexture2DArray(const std::vector<std::string>& paths, int& outW, int& outH)
{
    stbi_set_flip_vertically_on_load(false); // since you said you had to unflip

    int w = 0, h = 0, ch = 0;
    std::vector<unsigned char*> layers;
    layers.reserve(paths.size());

    for (size_t i = 0; i < paths.size(); i++) {
        unsigned char* data = stbi_load(paths[i].c_str(), &w, &h, &ch, 4); // force RGBA
        if (!data) {
            std::cout << "Failed to load: " << paths[i] << "\n";
            // free already loaded
            for (auto* p : layers) stbi_image_free(p);
            return 0;
        }

        if (i == 0) { outW = w; outH = h; }
        else if (w != outW || h != outH) {
            std::cout << "Texture size mismatch: " << paths[i]
                << " expected " << outW << "x" << outH
                << " got " << w << "x" << h << "\n";
            stbi_image_free(data);
            for (auto* p : layers) stbi_image_free(p);
            return 0;
        }

        layers.push_back(data);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0,
        GL_RGBA8,
        outW, outH,
        (GLsizei)layers.size(),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);

    for (int layer = 0; layer < (int)layers.size(); layer++) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
            0,
            0, 0, layer,
            outW, outH, 1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            layers[layer]);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // keep crisp up close
    // atlas/cube-net rules:
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, maxAniso));
#endif
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_LOD_BIAS, -0.25f);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    for (auto* p : layers) stbi_image_free(p);
    return tex;
}