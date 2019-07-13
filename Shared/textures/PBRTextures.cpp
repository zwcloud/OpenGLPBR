#include "PBRTextures.h"
#include "resource.h"
#include "Windows.h"
#include <stdint.h>
#include <fstream>

DWORD pbrImageResourceIds[5] =
{
    PBR_albedo,
    PBR_normal,
    PBR_metallic,
    PBR_roughness,
    PBR_ao,
};

const char* pbrImageFileNames[5] =
{
    "albedo.png",
    "normal.png",
    "metallic.png",
    "roughness.png",
    "ao.png",
};

void GetPBRTextureData(HMODULE hInstance, const char* imageFileName, uint8_t** data, uint32_t* dataByteSize)
{
    for (int i = 0; i < 5; i++)
    {
        if(strcmp(pbrImageFileNames[i], imageFileName) == 0)
        {
            auto resourceId = pbrImageResourceIds[i];
            HRSRC hResInfo = FindResource(hInstance, MAKEINTRESOURCE(resourceId), RT_RCDATA);
            *dataByteSize = SizeofResource(hInstance, hResInfo);
            HGLOBAL handle = LoadResource(hInstance, hResInfo);
            *data = static_cast<uint8_t*>(LockResource(handle));
            return;
        }
    }
    *data = nullptr;
    *dataByteSize = 0;
}
