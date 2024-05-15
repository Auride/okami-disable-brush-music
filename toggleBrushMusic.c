// Compile with:
// gcc toggleBrushMusic.c -o toggleBrushMusic.exe

// If you don't have the gcc C compiler, you can get it from
// https://winlibs.com/

#define TARGET_PROCESS_NAME "okami.exe"
#define MODULE_NAME "main.dll"

// The offset from the module where the patch bytes will be written to:
#define PATCH_LOC 0x4494BA
#define PATCH_LEN 6

// Normally, the game checks a flag located at main.dll+B6B2B9 bit 7 (most
// significant). If the bit is 1, it plays the brush music while in brush mode.
// This is normally set when entering brush mode, along with several other
// nearby flags associated with other effects. By making the game load a byte of
// all zeroes instead of reading the byte where the flag is, it always plays the
// normal area background music.
//
// One minor flaw is that this instruction change doesn't just disable the brush
// music; if you are standing in front of a save mirror and then brush, it goes
// from the save mirror music to the area music. If you unbrush, it goes back to
// the save mirror music.
#define ORIG_BYTES \
/* mov eax,[main.dll+B6B2B8] */{0x8B, 0x05, 0xF8, 0x1D, 0x72, 0x00}
#define PATCH_BYTES \
/* mov eax,00000000 */         {0xB8, 0x00, 0x00, 0x00, 0x00, \
/* nop */                       0x90};

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

#define ALIGN_PAGE_DOWN(VALUE) (((DWORD64)VALUE) & ~((0x1000ULL) - 1))
#define ALIGN_PAGE_UP(VALUE) ((((DWORD64)VALUE) + ((0x1000ULL) - 1)) & ~((0x1000ULL) - 1))


DWORD FindProcessId(LPCSTR ProcessName)
{
	DWORD processId = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &pe32))
	{
		do
		{
			if (strcmp(ProcessName, pe32.szExeFile) == 0)
			{
				processId = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &pe32));
	}

	CloseHandle(snapshot);

	return processId;
}

DWORD64 FindModuleBase(DWORD ProcessId, LPCSTR ModuleName)
{
	DWORD64 base = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &me32))
	{
		do
		{
			if (strcmp(ModuleName, me32.szModule) == 0)
			{
				base = (DWORD64)me32.modBaseAddr;
				break;
			}
		} while (Module32Next(snapshot, &me32));
	}

	CloseHandle(snapshot);

	return base;
}

DWORD InjectDllIntoProcess(DWORD ProcessId, LPCSTR DllFilePath)
{
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);

	LPVOID dllPathAddr = VirtualAllocEx(process, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	WriteProcessMemory(process, dllPathAddr, DllFilePath, strlen(DllFilePath), NULL);

	HMODULE kernel32Mod = GetModuleHandleA("Kernel32");
	LPVOID loadLibProc = GetProcAddress(kernel32Mod, "LoadLibraryA");

	HANDLE thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibProc, dllPathAddr, 0, NULL);

	WaitForSingleObject(thread, INFINITE);

	DWORD exitCode;

	GetExitCodeThread(thread, &exitCode);

	CloseHandle(thread);

	VirtualFreeEx(process, dllPathAddr, 0, MEM_RELEASE);

	CloseHandle(process);

	return exitCode;
}

VOID ReadFromMemory(HANDLE Process, DWORD64 Base, DWORD64 Size, PVOID Buffer)
{
	DWORD64 pageBase = ALIGN_PAGE_DOWN(Base);
	DWORD64 pageSize = ALIGN_PAGE_UP(Size);

	DWORD oldProtect = 0;

	if (VirtualProtectEx(Process, (PVOID)pageBase, pageSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		ReadProcessMemory(Process, (PVOID)Base, Buffer, Size, NULL);
		VirtualProtectEx(Process, (PVOID)pageBase, pageSize, oldProtect, &oldProtect);
	}
}

VOID WriteIntoMemory(HANDLE Process, DWORD64 Base, DWORD64 Size, PVOID Buffer)
{
	DWORD64 pageBase = ALIGN_PAGE_DOWN(Base);
	DWORD64 pageSize = ALIGN_PAGE_UP(Size);

	DWORD oldProtect = 0;

	if (VirtualProtectEx(Process, (PVOID)pageBase, pageSize, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		WriteProcessMemory(Process, (PVOID)Base, Buffer, Size, NULL);
		VirtualProtectEx(Process, (PVOID)pageBase, pageSize, oldProtect, &oldProtect);
	}
}

int main()
{
	// Find memory location where patch will be applied/reverted:
	DWORD processId = FindProcessId(TARGET_PROCESS_NAME);
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	DWORD64 moduleBase = FindModuleBase(processId, MODULE_NAME);
	DWORD64 targetLoc = moduleBase + PATCH_LOC;

	byte origBytes[PATCH_LEN] = ORIG_BYTES;
	byte patchBytes[PATCH_LEN] = PATCH_BYTES;
	byte currBytes[PATCH_LEN];

	// Check if patch is already applied:
	ReadFromMemory(process, targetLoc, PATCH_LEN, currBytes);
	boolean isPatchApplied = TRUE;
	for (size_t i = 0; i < PATCH_LEN; ++i) {
		if (patchBytes[i] != currBytes[i]) {
			isPatchApplied = FALSE;
			break;
		}
	}

	if (isPatchApplied) {
		WriteIntoMemory(process,targetLoc, PATCH_LEN, origBytes);
		printf("Patch reverted to original game bytes.");
	} else {
		WriteIntoMemory(process,targetLoc, PATCH_LEN, patchBytes);
		printf("Patch applied.");
	}
}