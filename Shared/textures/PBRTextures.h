#pragma once
#include "Windows.h"
#include "stdint.h"

void GetPBRTextureData(HMODULE hInstance, const char* imageFileName, uint8_t** data, uint32_t* dataByteSize);