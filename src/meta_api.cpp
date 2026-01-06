#include <extdll.h>
#include <meta_api.h>
#include "fltk_gui.hpp"

#define METAMOD_GUI_VERSION "0.1.0"

meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;

plugin_info_t Plugin_info =
{
	META_INTERFACE_VERSION,			// ifvers
	"MetamodGUI",				// name
	METAMOD_GUI_VERSION,				// version
	__DATE__,						// date
	"StevenLafl",					// author
	"https://github.com/stevenlafl/metamod-gui",	// url
	"MGUI",						// logtag
	PT_ANYTIME,						// (when) loadable
	PT_ANYTIME,						// (when) unloadable
};

extern DLL_FUNCTIONS g_DllFunctionTable;
extern DLL_FUNCTIONS g_DllFunctionTable_Post;

extern enginefuncs_t g_EngineFunctionsTable;
extern enginefuncs_t g_EngineFunctionsTable_Post;

C_DLLEXPORT int Meta_Query(char *interfaceVersion, plugin_info_t **plinfo, mutil_funcs_t *pMetaUtilFuncs)
{
	*plinfo = &Plugin_info;
	gpMetaUtilFuncs = pMetaUtilFuncs;
	return TRUE;
}

META_FUNCTIONS gMetaFunctionTable =
{
	NULL,						// pfnGetEntityAPI		HL SDK; called before game DLL
	NULL,						// pfnGetEntityAPI_Post		META; called after game DLL
	GetEntityAPI2,				// pfnGetEntityAPI2		HL SDK2; called before game DLL
	GetEntityAPI2_Post,			// pfnGetEntityAPI2_Post	META; called after game DLL
	GetNewDLLFunctions,			// pfnGetNewDLLFunctions	HL SDK2; called before game DLL
	GetNewDLLFunctions_Post,	// pfnGetNewDLLFunctions_Post	META; called after game DLL
	GetEngineFunctions,			// pfnGetEngineFunctions	META; called before HL engine
	GetEngineFunctions_Post,	// pfnGetEngineFunctions_Post	META; called after HL engine
};

void gui_open_cmd() {
	g_engfuncs.pfnServerPrint("MetamodGUI: Opening GUI window...\n");
	FltkGUI::getInstance().show();
}

void gui_close_cmd() {
	g_engfuncs.pfnServerPrint("MetamodGUI: Closing GUI window...\n");
	FltkGUI::getInstance().hide();
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
	gpMetaGlobals = pMGlobals;
	gpGamedllFuncs = pGamedllFuncs;

	g_engfuncs.pfnServerPrint("\n######################\n# MetamodGUI Loaded! #\n######################\n\n");

	// Initialize FLTK GUI
	if (!FltkGUI::getInstance().initialize()) {
		g_engfuncs.pfnServerPrint("MetamodGUI: Failed to initialize GUI!\n");
	} else {
		g_engfuncs.pfnServerPrint("MetamodGUI: GUI initialized successfully\n");

		// Auto-launch the GUI window
		FltkGUI::getInstance().show();
		g_engfuncs.pfnServerPrint("MetamodGUI: GUI launched (close window to quit server)\n");
	}

	// Register commands
	REG_SVR_COMMAND("gui_open", gui_open_cmd);
	REG_SVR_COMMAND("gui_close", gui_close_cmd);
	g_engfuncs.pfnServerPrint("MetamodGUI: Registered 'gui_open' and 'gui_close' commands\n");

	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
	g_engfuncs.pfnServerPrint("MetamodGUI: Shutting down...\n");
	FltkGUI::getInstance().shutdown();
	return TRUE;
}
