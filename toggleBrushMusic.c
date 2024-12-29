// Compile with:
// gcc toggleBrushMusic.c -o toggleBrushMusic.exe

// If you don't have the gcc C compiler, you can get it from
// https://winlibs.com/

#include <stdio.h>
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

boolean TogglePatch(
	const char* description,
	const HANDLE process, const DWORD64 moduleBase,
	const DWORD64 patchLoc, const size_t patchLen,
	char* origBytes, char* patchBytes
) {
	DWORD64 targetLoc = moduleBase + patchLoc;

	char* currBytes = malloc(patchLen);
	// Check if patch is already applied:
	ReadFromMemory(process, targetLoc, patchLen, currBytes);
	boolean isPatchAlreadyApplied = TRUE;
	for (size_t i = 0; i < patchLen; ++i) {
		if (patchBytes[i] != currBytes[i]) {
			isPatchAlreadyApplied = FALSE;
			// break;
		}
	}


	if (isPatchAlreadyApplied) {
		WriteIntoMemory(process,targetLoc, patchLen, origBytes);
		printf("\"%s\" patch reverted to original game bytes.\n", description);
	} else {
		// Check if original bytes are correct:
		for (size_t i = 0; i < patchLen; ++i) {
			if (origBytes[i] != currBytes[i]) {
				printf("ERROR: \"%s\" failed because original bytes did not match.\n", description);
				free(currBytes);
				return FALSE;
			}
		}
		WriteIntoMemory(process,targetLoc, patchLen, patchBytes);
		printf("\"%s\" patch applied.\n", description);
	}
	free(currBytes);
	return !isPatchAlreadyApplied;
}

int main(){
	// Find memory location where patch will be applied/reverted:
	DWORD processId = FindProcessId("okami.exe");
	if (processId == 0) {
		printf("Process \"%s\" is not running. No changes were made.", "okami.exe");
		return 0;
	}

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	DWORD64 moduleBase = FindModuleBase(processId, "main.dll");

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
	TogglePatch(
		"Disable Celestial Brush Music", process, moduleBase, 0x4494BA, 6,
		// Original:
		/* mov eax,[main.dll+B6B2B8] */"\x8B\x05\xF8\x1D\x72\x00",
		//Patch:
		/* mov eax,00000000 */         "\xB8\x00\x00\x00\x00"\
		/* nop */                      "\x90"
	);

	// The above patch by itself works 90% of the time, but in Ryo and North
	// Ryo, it just makes the music go silent when opening the brush. To fix it,
	// we can nop out this function call which I believe is associated with
	// setting up the fade-out of the Ryo music to silence.
	TogglePatch(
		"Fix Ryo music", process, moduleBase, 0x446C8C, 5,
		// Original:
		/* call main.dll+4487E0 */"\xE8\x4F\x1B\x00\x00",
		//Patch:
		/* nop (5 bytes) */       "\x0F\x1F\x44\x00\x01"
	);
	CloseHandle(process);
}