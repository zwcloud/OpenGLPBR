#include "Texture.h"
#include "resource.h"
#include "Windows.h"
#include <stdint.h>
#include <fstream>

DWORD hdrImageResourceId[] = {
    Binary_HDR_Brooklyn_Bridge_Planks_2k,
    Binary_HDR_Bryant_Park_2k,
    Binary_HDR_Factory_Catwalk_2k
};
const char* hdrImageFileName[] = { "Brooklyn_Bridge_Planks_2k.hdr", "Bryant_Park_2k.hdr","Factory_Catwalk_2k.hdr" };

void ExtractHDRTextures(HMODULE hInstance)
{
    for (int i = 0; i < 3; i++)
    {
        auto resourceId = hdrImageResourceId[i];
        auto filename = hdrImageFileName[i];

        HRSRC hResInfo = FindResource(hInstance, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        DWORD byteSize = SizeofResource(hInstance, hResInfo);
        HGLOBAL handle = LoadResource(hInstance, hResInfo);
        const char* data = static_cast<char*>(LockResource(handle));

        std::fstream file;
        file.open(filename, std::ios::out | std::ios::binary);
        file.write(data, byteSize);
        file.close();
    }

}
