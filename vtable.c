#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "MinHook.h"
#include "windows.h"

// #define PRINT_DEBUG_STUFF

#pragma pack(push, 1)

typedef struct id_gcc32 {
    uint16_t inst;
    char pad[2];
    uint32_t lo;
    uint32_t hi;
} id_gcc32_t;

typedef struct steam2ticket_gcc32  // sizeof=0x18
{
    unsigned int version;
    id_gcc32_t userid;
    unsigned int publicip;
    unsigned int handle;
} steam2ticket_gcc32_t;
static_assert(sizeof(steam2ticket_gcc32_t) == 0x18);

typedef struct id_win32 {
    uint16_t inst;
    char pad[6];
    uint32_t lo;
    uint32_t hi;
} id_win32_t;

typedef struct steam2ticket_win32 {
    unsigned int version;
    char pad[4];
    id_win32_t userid;
    unsigned int publicip;
    unsigned int handle;
} steam2ticket_win32_t;
static_assert(sizeof(steam2ticket_win32_t) == 0x20);

#pragma pack(pop)

typedef int(__fastcall *InitiateConnection_t)(void *thisptr, void *edx,
                                              uintptr_t a2, int a3,
                                              unsigned int netlong, int a5,
                                              int a6, unsigned int a7, int a8,
                                              char *ticket, int size);
typedef void *(*CreateInterfaceFn)(const char *name, int *);
typedef void (*Msg_t)(const char *fmt, ...);

InitiateConnection_t ogInitConn;
Msg_t Msg;
static int new_vstdlib = 0;

size_t hexstr(const void *data, size_t len, char *out, size_t out_size) {
    static const char hex[] = "0123456789abcdef";

    size_t needed = len * 2;
    if (out_size < needed + 1) {
        return 0;
    }

    const uint8_t *cur = data;
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[cur[i] >> 0x4];
        out[i * 2 + 1] = hex[cur[i] & 0x0f];
    }

    out[needed] = '\0';
    return needed;
}

int __fastcall InitiateConnection_hook(void *thisptr, void *edx, uintptr_t a2,
                                       int a3, unsigned int netlong, int a5,
                                       int a6, unsigned int a7, int a8,
                                       char *enckey, int enckeysize) {
    int len = ogInitConn(thisptr, 0, a2, a3, netlong, a5, a6, a7, a8, enckey,
                         enckeysize);

#ifdef PRINT_DEBUG_STUFF
    char hexticket[4096 + 1];  // max ticket + 1
    hexstr((void *)a2, len, hexticket, sizeof(hexticket));
    Msg("length %i ticket %s\n", len, hexticket);
#endif

    if (len == 52) { // malformed proton ticket
        steam2ticket_win32_t newticket = {0};
        steam2ticket_gcc32_t *oldticket = (steam2ticket_gcc32_t *)(a2 + 28);
        newticket.version = oldticket->version;
        newticket.userid.inst = oldticket->userid.inst;
        newticket.userid.lo = oldticket->userid.lo;
        newticket.userid.hi = oldticket->userid.hi;
        newticket.publicip = oldticket->publicip;
        newticket.handle = oldticket->handle;
        memcpy(oldticket, &newticket, sizeof(newticket));
        return 60;
    }
    if (len == 60) {
        return len;
    } else {
        return len - 8;
    }
}

char __fastcall Load(void *this, void *edx, void *f1, void *f2) {
    MH_Initialize();
    void *vstdlib = GetModuleHandle(L"vstdlib.dll");
    void *engine = GetModuleHandle(L"engine.dll");
    void *tier0 = GetModuleHandle(L"tier0.dll");
    if (!vstdlib || !engine || !tier0) {
        return 0;
    }

    CreateInterfaceFn vstdlib_factory =
        (CreateInterfaceFn)GetProcAddress(vstdlib, "CreateInterface");
    Msg = (Msg_t)GetProcAddress(tier0, "Msg");

    char iname[128];
    int current_version = 1;
    void *found = NULL;

    for (;;) {
        sprintf_s(iname, sizeof(iname), "VEngineCvar%03d", current_version);

        void *_current = vstdlib_factory(iname, NULL);
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
    MH_CreateHook((void *)((uintptr_t)engine + 0x0D6780),
                  (void *)InitiateConnection_hook, (void **)&ogInitConn);
    MH_EnableHook(MH_ALL_HOOKS);
    return 1;
}

void Unload() {
    MH_Uninitialize();
    return;
}

void Pause() {}
void UnPause() {}

const char *GetPluginDescription() { return "sdk2007 auth hook"; }

void __stdcall LevelInit(char const *pMapName) {}
void __stdcall ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void __stdcall GameFrame(char simulating) {}
void __stdcall LevelShutdown(void) {}
void __stdcall ClientActive(void *pEntity) {}
void __stdcall ClientDisconnect(void *pEntity) {}
void __stdcall ClientPutInServer(void *pEntity, char const *playername) {}
void __stdcall SetCommandClient(int index) {}
void __stdcall ClientSettingsChanged(void *pEdict) {}

int __stdcall ClientConnect(void *bAllowConnect, void *pEntity,
                            const char *pszName, const char *pszAddress,
                            char *reject, int maxrejectlen) {
    return 0;
}

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

int __stdcall NetworkIDValidated(const char *pszUserName,
                                 const char *pszNetworkID) {
    return 0;
};
void __stdcall OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity,
                                        int eStatus, const char *pCvarName,
                                        const char *pCvarValue){};
void __stdcall OnEdictAllocated(void *edict){};
void __stdcall OnEdictFreed(const void *edict){};

static void *vtable[] = {Load,
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
                         OnEdictFreed};

void *pVtable = &vtable;
void *ppVtable = &pVtable;

__declspec(dllexport) void *CreateInterface(const char *name, int *_) {
    if (strstr(name, "ISERVERPLUGINCALLBACKS"))
        return ppVtable;
    else
        return NULL;
}