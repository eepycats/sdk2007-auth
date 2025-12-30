#include "windows.h"
#include "MinHook.h"

typedef int (__fastcall *InitiateConnection_t)(
		void* thisptr,
		void* edx,
        int a2,
        int a3,
        unsigned int netlong,
        int a5,
        int a6,
        unsigned int a7,
        int a8,
        char* ticket,
        int size);
typedef void* (*CreateInterfaceFn)(const char* name, int*);
InitiateConnection_t ogInitConn;
static int new_vstdlib = 0;

int __fastcall InitiateConnection_hook(
        void* thisptr,
		void* edx,
        int a2,
        int a3,
        unsigned int netlong,
        int a5,
        int a6,
        unsigned int a7,
        int a8,
        char* enckey,
        int enckeysize){
	return ogInitConn(thisptr, 0, a2,a3,netlong,a5,a6,a7,a8,enckey,enckeysize)-8;
}



char __fastcall Load(void* this, void* edx, void* f1, void* f2) {	
	MH_Initialize();
	void* vstdlib = GetModuleHandle(L"vstdlib.dll");
	void* engine = GetModuleHandle(L"engine.dll");
	if (!vstdlib || !engine){
		return 0;
	}
	
	CreateInterfaceFn vstdlib_factory = (CreateInterfaceFn)GetProcAddress(vstdlib, "CreateInterface");

	char iname[128];
	int current_version = 1;
	void* found = NULL;

	for (;;) {
		sprintf_s(iname, sizeof(iname), "VEngineCvar%03d", current_version);

		void* _current = vstdlib_factory(iname, NULL);
		if (found && !_current) {
			current_version--;
			break;
		}
		current_version++;
		found = _current;
	}

	if (current_version > 3) {
		new_vstdlib = 1;
	}
	MH_CreateHook((void*) ( (uintptr_t)engine + 0x0D6780), (void*)InitiateConnection_hook, (void**)&ogInitConn);
	MH_EnableHook(MH_ALL_HOOKS);
	return 1;
}

void Unload() {
	MH_Uninitialize();
	return;
}

void Pause() {}
void UnPause() {}

const char* GetPluginDescription() {
	return "sdk2007 auth hook";
}

void __stdcall LevelInit(char const* pMapName) {}
void __stdcall ServerActivate(void* pEdictList, int edictCount, int clientMax) {}
void __stdcall GameFrame(char simulating) {}
void __stdcall LevelShutdown(void) {}
void __stdcall ClientActive(void* pEntity) {}
void __stdcall ClientDisconnect(void* pEntity) {}
void __stdcall ClientPutInServer(void* pEntity, char const* playername) {}
void __stdcall SetCommandClient(int index) {}
void __stdcall ClientSettingsChanged(void* pEdict) {}

int __stdcall ClientConnect(void* bAllowConnect, void* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) { return 0; }

void ClientCommand() {
	__asm {
		mov eax, [new_vstdlib]

		cmp eax, 0
		je old_interface
		xor eax, eax
		ret 8

		old_interface :
		xor eax, eax
		ret 4
	}
}

int __stdcall  NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) { return 0; };
void __stdcall OnQueryCvarValueFinished(int iCookie, void* pPlayerEntity, int eStatus, const char* pCvarName, const char* pCvarValue) {};
void __stdcall OnEdictAllocated(void* edict) {};
void __stdcall OnEdictFreed(const void* edict) {};

static void* vtable[] = {
	Load,
	Unload,
	Pause,
	UnPause,
	GetPluginDescription,
	LevelInit,
	ServerActivate,
	GameFrame,
	LevelShutdown,
	ClientActive,
	ClientDisconnect,
	ClientPutInServer,
	SetCommandClient,
	ClientSettingsChanged,
	ClientConnect,
	ClientCommand,
	NetworkIDValidated,
	OnQueryCvarValueFinished,
	OnEdictAllocated,
	OnEdictFreed
};

void* pVtable = &vtable;
void* ppVtable = &pVtable;

__declspec(dllexport) void* CreateInterface(const char* name, int* _) {
	if (strstr(name, "ISERVERPLUGINCALLBACKS")) return ppVtable;
	else return NULL;
}