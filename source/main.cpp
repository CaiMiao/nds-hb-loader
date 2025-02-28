/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "args.h"
#include "file_browse.h"
#include "hbmenu_banner.h"
#include "iconTitle.h"
#include "nds_loader_arm9.h"

#include "config/configurator.h"

using namespace std;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (pmMainLoop()) {
		swiWaitForVBlank();
	}
}

volatile bool top_screen_initialized = false;
volatile bool console_initialized = false;
volatile bool isConfig = false;
volatile bool isSkipExe = false;

void initTopScreen() {
	if (top_screen_initialized) return;
	iconTitleInit();
	top_screen_initialized = true;
}

void initConsole() {
	if (console_initialized) return;

	// Subscreen as a console
	videoSetModeSub(MODE_0_2D);
	vramSetBankH(VRAM_H_SUB_BG);
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);

	console_initialized = true;
}

static std::string getExecPath(const HBLDR_CONFIGS& confs) {
	static char executable_path[ENTRY_SIZE];

	scanKeys();

	switch(keysHeld() & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT
		| KEY_L | KEY_R | KEY_SELECT | KEY_START)) {
		case KEY_A | KEY_B:
			isConfig = true;
			break;
		case KEY_SELECT:
			isSkipExe = true;
			break;
		case KEY_A:
		case KEY_RIGHT:
			memcpy(executable_path, confs.hk_a.path, sizeof(confs.hk_a.path));
			break;
		case KEY_B:
		case KEY_DOWN:
			memcpy(executable_path, confs.hk_b.path, sizeof(confs.hk_b.path));
			break;
		case KEY_X:
		case KEY_UP:
			memcpy(executable_path, confs.hk_x.path, sizeof(confs.hk_x.path));
			break;
		case KEY_Y:
		case KEY_LEFT:
			memcpy(executable_path, confs.hk_y.path, sizeof(confs.hk_y.path));
			break;
		case KEY_L:
			memcpy(executable_path, confs.hk_l.path, sizeof(confs.hk_l.path));
			break;
		case KEY_R:
			memcpy(executable_path, confs.hk_r.path, sizeof(confs.hk_r.path));
			break;
		default:
			memcpy(executable_path, confs.hk_none.path, sizeof(confs.hk_none.path));
			break;
	}

	executable_path[ENTRY_SIZE - 1] = 0;
	return executable_path;
}

bool checkPath(const std::string& path) {
	const std::string mp[] = {"/", "fat:/", "sd:/"};
	for (auto s : mp)
	{
		if(path.starts_with(s))
			return true;
	}
	return false;
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	// so tapping power on DSi returns to DSi menu
	// pmClearResetJumpTarget();

	// install exception stub
	installExcptStub();

	if (!fatInitDefault()) {
		initConsole();
		iprintf ("fatinitDefault failed!\n");
		stop();
	}

	if (argc >=2 && argv[1][0]=='c' && argv[1][1]=='f' && argv[1][2]=='g') isConfig = true;
	// char* cwd = getcwd(NULL, 0);

	HBLDR_CONFIGS confs;
	readConfigsFromFile(&confs);

	const auto executable_path = getExecPath(confs);
	// if(executable_path.starts_with(cwd)) { // todo?: root sel?
	if(!isSkipExe && !isConfig && checkPath(executable_path)) {
		std::vector<std::string> argarray;
		int err = 10;
		if(argsFillArray(executable_path, argarray)) {
			err = runNdsFile(argarray);
		}
		initConsole();
		initTopScreen();
		iprintf("Failed to start\n%s.\n"
				"Error %i.\n"
				"Press START to continue boot.\n"
				"You can change the autoboot\n"
				"settings by holding A+B on\n"
				"launch", argarray[0].c_str(), err);
		while(pmMainLoop()) {
			swiWaitForVBlank();
			scanKeys();
			if((keysHeld() & KEY_START)) break;
		}
	}

	initConsole();
	initTopScreen();

	keysSetRepeat(25,5);

	if(isConfig) {
		configMenu(&confs);
	}

	// if (isSkipExe)
	// 	while(pmMainLoop()) {
	// 		swiWaitForVBlank();
	// 		scanKeys();
	// 		if((keysHeld() & KEY_START)) break;
	// 	}

	vector<string> extensionList = argsGetExtensionList();

	// chdir(cwd);
	chdir("fat:/");

	while(pmMainLoop()) {

		string filename = browseForFile(extensionList);
		if (filename.empty()) {
			continue;
		}

		// Construct a command line
		vector<string> argarray;
		if (!argsFillArray(filename, argarray)) {
			iprintf("Invalid NDS or arg file selected\n");
		} else {
			iprintf("Running %s with %d parameters\n", argarray[0].c_str(), argarray.size());

			// Make a copy of argarray using C strings, for the sake of runNdsFile
			vector<const char*> c_args;
			for (const auto& arg: argarray) {
				c_args.push_back(arg.c_str());
			}

			// Try to run the NDS file with the given arguments
			int err = runNdsFile(c_args[0], c_args.size(), &c_args[0]);
			iprintf("Start failed. Error %i\n", err);
		}

		argarray.clear();

		while (pmMainLoop()) {
			swiWaitForVBlank();
			scanKeys();
			if (!(keysHeld() & KEY_A)) break;
		}

	}

	return 0;
}
