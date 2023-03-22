#include "stdafx.h"
#include "stdio.h"
#include "includes\injector\injector.hpp"
#include "includes\mINI\src\mini\ini.h"
#include <cmath>
#include <d3dx9.h>

struct bVector3 
{
    float x;
    float y;
    float z;
};

struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
};

uint32_t g_ParticleSystemEnable = 1;
uint32_t SaveESI = 0;

uint32_t loc_78944A = 0x78944A;
uint32_t loc_7893EA = 0x7893EA;

uint32_t Attrib_FindCollection = 0x0052CD40;
uint32_t AddXenonEffect = 0x004B7830;

float ContrailMinIntensity = 0.1f;
float ContrailMaxIntensity = 0.75f;
float ContrailSpeed = 35.0f;

bool bLimitContrailRate = true;
uint32_t ContrailFrameDelay = 1;
float ContrailTargetFPS = 30.0f;

void(__cdecl* AddXenonEffect_Abstract)(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity) = (void(__cdecl*)(void*, void*, bMatrix4*, bVector4*, float))AddXenonEffect;


uint32_t ContrailFC = 0;
uint32_t RenderConnFC = 0;
void AddXenonEffect_Contrail_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
{
    float newintensity = ContrailMaxIntensity;

    double carspeed = ((sqrt((*vel).x * (*vel).x + (*vel).y * (*vel).y + (*vel).z * (*vel).z) - ContrailSpeed)) / ContrailSpeed;
    newintensity = std::lerp(ContrailMinIntensity, ContrailMaxIntensity, carspeed);
    if (newintensity > ContrailMaxIntensity)
        newintensity = ContrailMaxIntensity;


    if (!bLimitContrailRate)
        return AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, newintensity);

    if ((ContrailFC + ContrailFrameDelay) <= RenderConnFC)
    {
        if (ContrailFC != RenderConnFC)
        {
            ContrailFC = RenderConnFC;
            AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, newintensity);
        }
    }
    RenderConnFC++;
}

void __declspec(naked) CarRenderConnHook()
{
	_asm
	{
                mov SaveESI, esi
		        mov     eax, g_ParticleSystemEnable
                test    eax, eax
                jz      loc_7E13A5
                mov     al, [edi+19Ch]
                test    al, al
                jz      loc_7E13A5
                test    ebx, ebx
                jz      loc_7E140C
                mov     eax, [esi+8]
                cmp     eax, 1
                jz      short loc_7E1346
                cmp     eax, 2
                jnz     loc_7E13A5

loc_7E1346:                             ; CODE XREF: sub_7E1160+169↑j
                mov     edx, [ebx]
                mov     ecx, ebx
                call    dword ptr [edx+2Ch]
                test    al, al
                jz      short loc_7E13A5
                push    16AFDE7Bh
                push    6F5943F1h
                call    Attrib_FindCollection
                add     esp, 8
                push    3F400000h       ; float
                mov     esi, eax
                mov     eax, [edi+34h]
                mov     ecx, [edi+30h]
                push    eax
                push    ecx

loc_7E1397:                             ; CODE XREF: sub_7E1160+1DD↑j
                push    esi
                push    0
                call    AddXenonEffect_Contrail_Hook
                mov     esi, SaveESI
                add     esp, 14h
loc_7E13A5:
                test ebx, ebx
                jz short loc_7E140C
                cmp dword ptr [esi+8], 3
                jmp loc_7893EA
loc_7E140C:
                jmp loc_78944A
	}
}

double GetTargetFrametime()
{
    return **(double**)0x6D8B8F;
}

void InitConfig()
{
    mINI::INIFile inifile("NFSPS_Contrails.ini");
    mINI::INIStructure ini;
    inifile.read(ini);

    if (ini.has("MAIN"))
    {
        if (ini["MAIN"].has("ContrailSpeed"))
            ContrailSpeed = std::stof(ini["MAIN"]["ContrailSpeed"]);
        if (ini["MAIN"].has("LimitContrailRate"))
            bLimitContrailRate = std::stol(ini["MAIN"]["LimitContrailRate"]) != 0;
    }

    if (ini.has("Limits"))
    {
        if (ini["Limits"].has("ContrailTargetFPS"))
            ContrailTargetFPS = std::stof(ini["Limits"]["ContrailTargetFPS"]);
        if (ini["Limits"].has("ContrailMinIntensity"))
            ContrailMinIntensity = std::stof(ini["Limits"]["ContrailMinIntensity"]);
        if (ini["Limits"].has("ContrailMaxIntensity"))
            ContrailMaxIntensity = std::stof(ini["Limits"]["ContrailMaxIntensity"]);
    }


    static float fGameTargetFPS = GetTargetFrametime();

    if (fGameTargetFPS == ContrailTargetFPS)
        bLimitContrailRate = false;
    else
    {
        static float fContrailFrameDelay = (fGameTargetFPS / ContrailTargetFPS);
        ContrailFrameDelay = (uint32_t)round(fContrailFrameDelay);
    }
}

int Init()
{
    //freopen("CON", "w", stdout);
	//freopen("CON", "w", stderr);

    InitConfig();

    // backport carbon code
    injector::MakeJMP(0x7893E2, CarRenderConnHook, true);
    
    // set a different eEffect which isn't buggy with transformations
    injector::WriteMemory<uint32_t>(0x004B61F2, 0x00BFC0B0, true);

    //static double fGameTargetFPS = GetTargetFrametime();
    //
    //static double fContrailFrameDelay = (fGameTargetFPS / ContrailTargetFPS);
    //ContrailFrameDelay = (uint32_t)round(fContrailFrameDelay);

	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}

