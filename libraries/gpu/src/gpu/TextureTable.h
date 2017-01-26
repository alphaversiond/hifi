//
//  Texture.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/16/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_TextureTable_h
#define hifi_gpu_TextureTable_h

#include "Forward.h"

#include <array>

namespace gpu {

class TextureTable {
public:
    static const size_t COUNT = 8;
    TextureTable();
    TextureTable(const std::initializer_list<TexturePointer>& textures);
    TextureTable(const std::array<TexturePointer, COUNT>& textures);

    // Only for gpu::Context
    const GPUObjectPointer gpuObject{};

    void setTexture(size_t index, const TexturePointer& texturePointer);
    void setTexture(size_t index, const TextureView& textureView);

    Stamp getStamp() const { return _stamp; }
private:
    std::array<TexturePointer, COUNT> _textures;
    Stamp _stamp{ 0 };
};

}

#endif
