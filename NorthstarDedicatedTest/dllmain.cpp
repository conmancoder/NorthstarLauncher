#include "pch.h"
#include "hooks.h"
#include "main.h"
#include "squirrel.h"
#include "dedicated.h"
#include "dedicatedmaterialsystem.h"
#include "sourceconsole.h"
#include "logging.h"
#include "concommand.h"
#include "modmanager.h"
#include "filesystem.h"
#include "serverauthentication.h"
#include "scriptmodmenu.h"
#include "scriptserverbrowser.h"
#include "keyvalues.h"
#include "masterserver.h"
#include "gameutils.h"
#include "chatcommand.h"
#include "modlocalisation.h"
#include "playlist.h"
#include "securitypatches.h"
#include "miscserverscript.h"
#include "clientauthhooks.h"
#include "scriptbrowserhooks.h"
#include "scriptmainmenupromos.h"
#include "miscclientfixes.h"
#include "miscserverfixes.h"
#include "memalloc.h"

bool initialised = false;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    // pls no xD
    //if (!initialised)
    //    InitialiseNorthstar();
    //initialised = true;

    return TRUE;
}

void WaitForDebugger(HMODULE baseAddress)
{
    // earlier waitfordebugger call than is in vanilla, just so we can debug stuff a little easier
    //if (CommandLine()->CheckParm("-waitfordebugger"))
    if (strstr(GetCommandLineA(), "-waitfordebugger"))
    {
        spdlog::info("waiting for debugger...");

        while (!IsDebuggerPresent())
            Sleep(100);
    }
}

bool InitialiseNorthstar()
{
    if (initialised)
    {
        spdlog::warn("Called InitialiseNorthstar more than once!");
        return false;
    }
    initialised = true;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    InitialiseLogging();

    // apply initial hooks
    InstallInitialHooks();
    InitialiseInterfaceCreationHooks();

    AddDllLoadCallback("engine.dll", WaitForDebugger);
    AddDllLoadCallback("engine.dll", InitialiseEngineGameUtilFunctions);
    AddDllLoadCallback("server.dll", InitialiseServerGameUtilFunctions);
    AddDllLoadCallback("engine.dll", InitialiseEngineSpewFuncHooks);

    // dedi patches
    {
        AddDllLoadCallback("launcher.dll", InitialiseDedicatedOrigin);
        AddDllLoadCallback("engine.dll", InitialiseDedicated);
        AddDllLoadCallback("server.dll", InitialiseDedicatedServerGameDLL);
        AddDllLoadCallback("materialsystem_dx11.dll", InitialiseDedicatedMaterialSystem);
        // this fucking sucks, but seemingly we somehow load after rtech_game???? unsure how, but because of this we have to apply patches here, not on rtech_game load
        AddDllLoadCallback("engine.dll", InitialiseDedicatedRtechGame);
    }
    
    AddDllLoadCallback("engine.dll", InitialiseConVars);
    AddDllLoadCallback("engine.dll", InitialiseConCommands);

    // client-exclusive patches
    {
        AddDllLoadCallback("engine.dll", InitialiseClientEngineSecurityPatches);
        AddDllLoadCallback("client.dll", InitialiseClientSquirrel);
        AddDllLoadCallback("client.dll", InitialiseSourceConsole);
        AddDllLoadCallback("engine.dll", InitialiseChatCommands);
        AddDllLoadCallback("client.dll", InitialiseScriptModMenu);
        AddDllLoadCallback("client.dll", InitialiseScriptServerBrowser);
        AddDllLoadCallback("localize.dll", InitialiseModLocalisation);
        AddDllLoadCallback("engine.dll", InitialiseClientAuthHooks);
        AddDllLoadCallback("engine.dll", InitialiseScriptExternalBrowserHooks);
        AddDllLoadCallback("client.dll", InitialiseScriptMainMenuPromos);
        AddDllLoadCallback("client.dll", InitialiseMiscClientFixes);
    }

    AddDllLoadCallback("server.dll", InitialiseServerSquirrel);
    AddDllLoadCallback("engine.dll", InitialiseServerAuthentication);
    AddDllLoadCallback("engine.dll", InitialiseSharedMasterServer);
    AddDllLoadCallback("server.dll", InitialiseMiscServerScriptCommand);
    AddDllLoadCallback("server.dll", InitialiseMiscServerFixes);

    AddDllLoadCallback("engine.dll", InitialisePlaylistHooks);

    AddDllLoadCallback("filesystem_stdio.dll", InitialiseFilesystem);
    AddDllLoadCallback("engine.dll", InitialiseKeyValues);

    // mod manager after everything else
    AddDllLoadCallback("engine.dll", InitialiseModManager);

    // TODO: If you wanna make it more flexible and for example injectable with old Icepick injector
    // in this place you should iterate over all already loaded DLLs and execute their callbacks and mark them as executed
    // (as they will never get called otherwise and stuff will fail)

    return true;
}