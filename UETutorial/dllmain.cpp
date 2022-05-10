#include <Windows.h>
#include <iostream>
#include <vector>
#include "structs.h"

static uint64_t FindPattern(const char* signature, bool bRelative = false, uint32_t offset = 0)
{
	auto base_address = (uint64_t)GetModuleHandleW(NULL);
	static auto patternToByte = [](const char* pattern)
	{
		auto bytes = std::vector<int>{};
		const auto start = const_cast<char*>(pattern);
		const auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current)
		{
			if (*current == '?')
			{
				++current;
				if (*current == '?') ++current;
				bytes.push_back(-1);
			}
			else { bytes.push_back(strtoul(current, &current, 16)); }
		}
		return bytes;
	};

	const auto dosHeader = (PIMAGE_DOS_HEADER)base_address;
	const auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)base_address + dosHeader->e_lfanew);

	const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto patternBytes = patternToByte(signature);
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(base_address);

	const auto s = patternBytes.size();
	const auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i)
	{
		bool found = true;
		for (auto j = 0ul; j < s; ++j)
		{
			if (scanBytes[i + j] != d[j] && d[j] != -1)
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			auto address = (uint64_t)&scanBytes[i];
			if (bRelative)
				address = ((address + offset + 4) + *(int*)(address + offset));
			return address;
		}
	}
	return NULL;
}

DWORD WINAPI Main(LPVOID)
{
    AllocConsole();

    FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);
    
	auto ObjectsAddr = FindPattern("48 8B 05 ? ? ? ? 48 8D 14 C8 EB 03 49 8B D6 8B 42 08 C1 E8 1D A8 01 0F 85 ? ? ? ? F7 86 ? ? ? ? ? ? ? ?", true, 3);

	if (!ObjectsAddr)
	{
		MessageBoxA(0, "Objects could not be found!", "UETutorial", MB_ICONERROR);
		FreeLibraryAndExitThread(GetModuleHandleW(0), 0);
	}

	ObjObjects = decltype(ObjObjects)(ObjectsAddr);

	auto FreeAddr = FindPattern("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0C");

	if (!FreeAddr)
	{
		MessageBoxA(0, "Free could not be found!", "UETutorial", MB_ICONERROR);
		FreeLibraryAndExitThread(GetModuleHandleW(0), 0);
	}

	Free = decltype(Free)(FreeAddr);

	auto ToStringAddr = FindPattern("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9 75 43 E8 ? ? ? ? 4C 8B C8 8B 07 99 81 E2 ? ? ? ? 03 C2 8B C8 25 ? ? ? ?");

	if (!ToStringAddr)
	{
		MessageBoxA(0, "ToString could not be found!", "UETutorial", MB_ICONERROR);
		FreeLibraryAndExitThread(GetModuleHandleW(0), 0);
	}

	ToStringO = decltype(ToStringO)(ToStringAddr);

	std::cout << ObjObjects->GetObjectPtr(1) << '\n';

	std::cout << ObjObjects->GetObjectPtr(1)->GetFullName() << '\n';
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

