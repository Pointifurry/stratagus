//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name ccl.c		-	The craft configuration language. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "freecraft.h"

#include "iolib.h"
#include "ccl.h"
#include "missile.h"
#include "depend.h"
#include "upgrade.h"
#include "construct.h"
#include "unit.h"
#include "map.h"
#include "pud.h"
#include "ccl_sound.h"
#include "ui.h"
#include "interface.h"
#include "font.h"
#include "pathfinder.h"
#include "ai.h"
#include "campaign.h"
#include "trigger.h"
#include "settings.h"
#include "editor.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

global char* CclStartFile;		/// CCL start file
global int CclInConfigFile;		/// True while config file parsing

global char*	Tips[MAX_TIPS+1];	/// Array of tips
global int	ShowTips;		/// Show tips at start of level
global int	CurrentTip;		/// Current tip to display

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Protect SCM object against garbage collector.
**
**	@param obj	Scheme object
*/
global void CclGcProtect(SCM obj)
{
    SCM var;

    var=gh_symbol2scm("*ccl-protect*");
    setvar(var,cons(obj,symbol_value(var,NIL)),NIL);
}

/*............................................................................
..	Config
............................................................................*/

/**
**	Return the freecraft library path.
**
**	@return		Current libray path.
*/
local SCM CclFreeCraftLibraryPath(void)
{
    return gh_str02scm(FreeCraftLibPath);
}

#if 0
/**
**	Show tips at the start of a level.
*/
local SCM CclShowTips(void)
{
    static int InitTips=1;

    if( !InitTips )
	return SCM_UNSPECIFIED;

    InitTips=0;
    ShowTips=1;
    // JOHNS: Done by system: memset(Tips,0,sizeof(Tips));
    return SCM_UNSPECIFIED;
}
#endif

/**
**	Enable/disable Showing the tips at the start of a level.
**
**	@param flag	True = turn on, false = off.
**	@return		The old state of tips displayed.
*/
local SCM CclSetShowTips(SCM flag)
{
    int old;

    old=ShowTips;
    ShowTips=gh_scm2bool(flag);

    return gh_bool2scm(old);
}

/**
**	Add a new tip to the list of tips.
**
**	@param tip	A new tip to be displayed before level.
**
**	@todo	FIXME:	Memory for tips is never freed.
**		FIXME:	Make Tips dynamic.
*/
local SCM CclAddTip(SCM tip)
{
    int i;

    for( i=0; i<MAX_TIPS; i++ ) {
	if( Tips[i] && !strcmp(get_c_string(tip),Tips[i]) ) {
	    break;
	}
	if( Tips[i]==NULL ) {
	    Tips[i]=gh_scm2newstr(tip,NULL);
	    break;
	}
    }

    return tip;
}

/**
**	For debug increase mining speed.
**
**	@param speed	Speed factor of gold mining.
*/
local SCM CclSpeedMine(SCM speed)
{
    SpeedMine=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase gold delivery speed.
**
**	@param speed	Speed factor of gold mining.
*/
local SCM CclSpeedGold(SCM speed)
{
    SpeedGold=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase wood chopping speed.
*/
local SCM CclSpeedChop(SCM speed)
{
    SpeedChop=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase wood delivery speed.
*/
local SCM CclSpeedWood(SCM speed)
{
    SpeedWood=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase haul speed.
*/
local SCM CclSpeedHaul(SCM speed)
{
    SpeedHaul=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase oil delivery speed.
*/
local SCM CclSpeedOil(SCM speed)
{
    SpeedOil=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase building speed.
*/
local SCM CclSpeedBuild(SCM speed)
{
    SpeedBuild=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase training speed.
*/
local SCM CclSpeedTrain(SCM speed)
{
    SpeedTrain=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase upgrading speed.
*/
local SCM CclSpeedUpgrade(SCM speed)
{
    SpeedUpgrade=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase researching speed.
*/
local SCM CclSpeedResearch(SCM speed)
{
    SpeedResearch=gh_scm2int(speed);

    return speed;
}

/**
**	For debug increase all speeds.
*/
local SCM CclSpeeds(SCM speed)
{
    SpeedMine=SpeedGold=
	SpeedChop=SpeedWood=
	SpeedHaul=SpeedOil=
	SpeedBuild=
	SpeedTrain=
	SpeedUpgrade=
	SpeedResearch=gh_scm2int(speed);

    return speed;
}

/**
**	Debug unit slots.
*/
local SCM CclUnits(void)
{
    Unit** slot;
    int freeslots;
    int destroyed;
    int nullrefs;
    int i;
    static char buf[80];

    i=0;
    slot=UnitSlotFree;
    while( slot ) {			// count the free slots
	++i;
	slot=(void*)*slot;
    }
    freeslots=i;

    //
    //	Look how many slots are used
    //
    destroyed=nullrefs=0;
    for( slot=UnitSlots; slot<UnitSlots+MAX_UNIT_SLOTS; ++slot ) {
	if( *slot
		&& (*slot<(Unit*)UnitSlots
			|| *slot>(Unit*)(UnitSlots+MAX_UNIT_SLOTS)) ) {
	    if( (*slot)->Destroyed ) {
		++destroyed;
	    } else if( !(*slot)->Refs ) {
		++nullrefs;
	    }
	}
    }

    sprintf(buf,"%d free, %d(%d) used, %d, destroyed, %d null"
	    ,freeslots,MAX_UNIT_SLOTS-1-freeslots,NumUnits,destroyed,nullrefs);
    SetStatusLine(buf);
    fprintf(stderr,"%d free, %d(%d) used, %d destroyed, %d null\n"
	    ,freeslots,MAX_UNIT_SLOTS-1-freeslots,NumUnits,destroyed,nullrefs);

    return gh_int2scm(destroyed);
}

/**
**	Compiled with sound.
*/
local SCM CclWithSound(void)
{
#ifdef WITH_SOUND
    return SCM_BOOL_T;
#else
    return SCM_BOOL_F;
#endif
}

/**
**	Get FreeCraft home path.
*/
local SCM CclGetFreeCraftHomePath(void)
{
    const char* cp;
    char* buf;

    cp=getenv("HOME");
    buf=alloca(strlen(cp)+sizeof(FREECRAFT_HOME_PATH)+2);
    strcpy(buf,cp);
    strcat(buf,"/");
    strcat(buf,FREECRAFT_HOME_PATH);

    return gh_str02scm(buf);
}

/**
**	Get FreeCraft library path.
*/
local SCM CclGetFreeCraftLibraryPath(void)
{
    return gh_str02scm(FREECRAFT_LIB_PATH);
}

/*............................................................................
..	Tables
............................................................................*/

/**
**	Load a pud. (Try in library path first)
**
**	@param file	filename of pud.
**
**	@return		FIXME: Nothing.
*/
local SCM CclLoadPud(SCM file)
{
    char* name;
    char buffer[1024];

    name=gh_scm2newstr(file,NULL);
    LoadPud(LibraryFileName(name,buffer),&TheMap);
    free(name);

    // FIXME: LoadPud should return an error
    return SCM_UNSPECIFIED;
}

/**
**	Define a map.
**
**	@param width	Map width.
**	@param height	Map height.
*/
local SCM CclDefineMap(SCM width,SCM height)
{
    TheMap.Width=gh_scm2int(width);
    TheMap.Height=gh_scm2int(height);

    TheMap.Fields=calloc(TheMap.Width*TheMap.Height,sizeof(*TheMap.Fields));
    TheMap.Visible[0]=calloc(TheMap.Width*TheMap.Height/8,1);
    InitUnitCache();
    // FIXME: this should be CreateMap or InitMap?

    // MapX=MapY=0;

    return SCM_UNSPECIFIED;
}

/*............................................................................
..	Commands
............................................................................*/

/**
**	Send command to ccl.
**
**	@param command	Zero terminated command string.
*/
global void CclCommand(char* command)
{
    char msg[80];
    int retval;

    strncpy(msg,command,sizeof(msg));

    // FIXME: cheat protection
    retval=repl_c_string(msg,0,0,sizeof(msg));
    DebugLevel3("\n%d=%s\n" _C_ retval _C_ msg);

    SetMessage(msg);
}

/*............................................................................
..	Setup
............................................................................*/

/**
**	Initialize ccl and load the config file(s).
*/
global void InitCcl(void)
{
    char* sargv[5];
    char* buf;

    sargv[0] = "FreeCraft";
    sargv[1] = "-v1";
    sargv[2] = "-g0";
    sargv[3] = "-h200000:20";
    buf=malloc(strlen(FreeCraftLibPath)+4);
#ifdef __MINGW32__
    sprintf(buf,"-l%s\\",FreeCraftLibPath);
#else
    sprintf(buf,"-l%s",FreeCraftLibPath);
#endif
    sargv[4] = buf;			// never freed
    siod_init(5,sargv);

    init_subr_0("library-path",CclFreeCraftLibraryPath);


    gh_new_procedure1_0("set-show-tips!",CclSetShowTips);
    gh_new_procedure1_0("add-tip",CclAddTip);

    gh_new_procedure1_0("speed-mine",CclSpeedMine);
    gh_new_procedure1_0("speed-gold",CclSpeedGold);
    gh_new_procedure1_0("speed-chop",CclSpeedChop);
    gh_new_procedure1_0("speed-wood",CclSpeedWood);
    gh_new_procedure1_0("speed-haul",CclSpeedHaul);
    gh_new_procedure1_0("speed-oil",CclSpeedOil);
    gh_new_procedure1_0("speed-build",CclSpeedBuild);
    gh_new_procedure1_0("speed-train",CclSpeedTrain);
    gh_new_procedure1_0("speed-upgrade",CclSpeedUpgrade);
    gh_new_procedure1_0("speed-research",CclSpeedResearch);
    gh_new_procedure1_0("speeds",CclSpeeds);

    IconCclRegister();
    MissileCclRegister();
    PlayerCclRegister();
    TilesetCclRegister();
    MapCclRegister();
    PathfinderCclRegister();
    ConstructionCclRegister();
    DecorationCclRegister();
    UnitTypeCclRegister();
    UpgradesCclRegister();
    DependenciesCclRegister();
    SelectionCclRegister();
    GroupCclRegister();
    UnitCclRegister();
    SoundCclRegister();
    FontsCclRegister();
    UserInterfaceCclRegister();
    AiCclRegister();
    CampaignCclRegister();
    TriggerCclRegister();
    CreditsCclRegister();

    EditorCclRegister();

    init_subr_1("load-pud",CclLoadPud);
    init_subr_2("define-map",CclDefineMap);

    gh_new_procedure0_0("units",CclUnits);

    gh_new_procedure0_0("with-sound",CclWithSound);
    gh_new_procedure0_0("get-freecraft-home-path",CclGetFreeCraftHomePath);
    gh_new_procedure0_0("get-freecraft-library-path"
	    ,CclGetFreeCraftLibraryPath);

    //
    //	Make some sombols for the compile options/features.
    //
#ifdef USE_THREAD
    gh_define("freecraft-feature-thread",SCM_BOOL_T);
#endif
#ifdef DEBUG
    gh_define("freecraft-feature-debug",SCM_BOOL_T);
#endif
#ifdef DEBUG_FLAGS
    gh_define("freecraft-feature-debug-flags",SCM_BOOL_T);
#endif
#ifdef USE_ZLIB
    gh_define("freecraft-feature-zlib",SCM_BOOL_T);
#endif
#ifdef USE_BZ2LIB
    gh_define("freecraft-feature-bz2lib",SCM_BOOL_T);
#endif
#ifdef USE_ZZIPLIB
    gh_define("freecraft-feature-zziplib",SCM_BOOL_T);
#endif
#ifdef USE_SDL
    gh_define("freecraft-feature-sdl",SCM_BOOL_T);
#endif
#ifdef USE_SDLA
    gh_define("freecraft-feature-sdl-audio",SCM_BOOL_T);
#endif
#ifdef USE_SDLCD
    gh_define("freecraft-feature-sdl-cd",SCM_BOOL_T);
#endif
#ifdef USE_X11
    gh_define("freecraft-feature-x11",SCM_BOOL_T);
#endif
#ifdef USE_SVGALIB
    gh_define("freecraft-feature-svgalib",SCM_BOOL_T);
#endif
#ifdef WITH_SOUND
    gh_define("freecraft-feature-with-sound",SCM_BOOL_T);
#endif
#ifdef UNIT_ON_MAP
    gh_define("freecraft-feature-unit-on-map",SCM_BOOL_T);
#endif
#ifdef UNITS_ON_MAP
    gh_define("freecraft-feature-units-on-map",SCM_BOOL_T);
#endif
#ifdef NEW_MAPDRAW
    gh_define("freecraft-feature-new-mapdraw",SCM_BOOL_T);
#endif
#ifdef HIERARCHIC_PATHFINDER
    gh_define("freecraft-feature-hierarchic-pathfinder",SCM_BOOL_T);
#endif
#ifdef NEW_FOW
    gh_define("freecraft-feature-new-fow",SCM_BOOL_T);
#endif
#ifdef NEW_AI
    gh_define("freecraft-feature-new-ai",SCM_BOOL_T);
#endif
#ifdef NEW_SHIPS
    gh_define("freecraft-feature-new-ships",SCM_BOOL_T);
#endif
#ifdef SLOW_INPUT
    gh_define("freecraft-feature-slow-input",SCM_BOOL_T);
#endif
#ifdef HAVE_EXPANSION
    gh_define("freecraft-feature-have-expansion",SCM_BOOL_T);
#endif
#ifdef USE_FLAC
    gh_define("freecraft-feature-flac",SCM_BOOL_T);
#endif
#ifdef USE_OGG
    gh_define("freecraft-feature-ogg",SCM_BOOL_T);
#endif
#ifdef USE_MAD
    gh_define("freecraft-feature-mp3",SCM_BOOL_T);
#endif
#ifdef USE_LIBCDA
    gh_define("freecraft-feature-libcda",SCM_BOOL_T);
#endif

    gh_define("*ccl-protect*",NIL);

    print_welcome();
}

/**
**	Load freecraft config file.
*/
global void LoadCcl(void)
{
    char* file;
    char* s;
    char buf[1024];
    extern LISP fast_load(LISP lfname,LISP noeval);

    //
    //	Load and evaluate configuration file
    //
    CclInConfigFile=1;
    file=LibraryFileName(CclStartFile,buf);
    ShowLoadProgress("Script %s\n",file);
    if( (s=strrchr(file,'.')) && s[1]=='C' ) {
	fast_load(gh_str02scm(file),NIL);
    } else {
	vload(file,0,1);
    }
    CclInConfigFile=0;
    user_gc(SCM_BOOL_F);		// Cleanup memory after load
}

/**
**	Save CCL Module.
**
**	@param file	Save file.
*/
global void SaveCcl(FILE* file)
{
    SCM list;
    extern SCM oblistvar;

    fprintf(file,"\n;;; -----------------------------------------\n");
    fprintf(file,";;; MODULE: CCL $Id$\n\n");

    for( list=oblistvar; CONSP(list); list=CDR(list) ) {
	SCM sym;

	sym=CAR(list);
	if( !gh_null_p(symbol_boundp(sym, NIL)) ) {
	    SCM value;

	    fprintf(file,";;(define %s\n",get_c_string(sym));
	    value = symbol_value(sym, NIL);
	    fprintf(file,";;");
	    lprin1f(value,file);
	    fprintf(file,"\n");
#ifdef DEBUG
	} else {
	    fprintf(file,";;%s unbound\n",get_c_string(sym));
#endif
	}
    }
}

//@}