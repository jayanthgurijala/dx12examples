/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "stdafx.h"
#include <vector>

namespace WICImageLoader
{
    struct ImageData
    {
        uint32_t width;
        uint32_t height;
        std::vector<uint8_t> pixels; // RGBA8
    };

    ImageData LoadImageFromMemory_WIC(const uint8_t* data, size_t sizeInBytes);

}


