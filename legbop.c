/* SPDX-License-Identifier: GPL-3.0-or-later */
/* legbop v1.0 (December 2022)
 * Copyright (C) 2016-2022 Norbert de Jonge <nlmdejonge@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see [ www.gnu.org/licenses/ ].
 *
 * To properly read this code, set your program's tab stop to: 2.
 */

/*========== Includes ==========*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#include <windows.h>
#undef PlaySound
#endif

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
/*========== Includes ==========*/

/*========== Defines ==========*/
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#define SLASH "\\"
#define DEVNULL "NUL"
#else
#define SLASH "/"
#define DEVNULL "/dev/null"
#endif

#define EXIT_NORMAL 0
#define EXIT_ERROR 1
#define EDITOR_NAME "legbop"
#define EDITOR_VERSION "v1.0 (December 2022)"
#define COPYRIGHT "Copyright (C) 2022 Norbert de Jonge"
#define OFFSET_LEVEL0 0x1DCC4 /*** various ***/
#define MAX_LEVEL_SIZE 20000
#define LEVELS 17
#define ROOMS 24
#define TILES 30
#define EVENTS 256
#define ROM_DIR "rom"
#define BACKUP ROM_DIR SLASH "rom.bak"
#define MAX_PATHFILE 200
#define MAX_TOWRITE 720
#define WINDOW_WIDTH 640 + 2 + 50 /*** 692 ***/
#define WINDOW_HEIGHT 496 + 2 + 75 /*** 573 ***/
#define MAX_IMG 200
#define MAX_CON 30
#define MAX_TYPE 10
#define REFRESH 25 /*** That is 40 frames per second, 1000/25. ***/
#define FONT_SIZE_15 15
#define FONT_SIZE_11 11
#define FONT_SIZE_20 20
#define NUM_SOUNDS 20 /*** Sounds that may play at the same time. ***/
#define MAX_TEXT 100
#define ADJ_BASE_X 422
#define ADJ_BASE_Y 60
#define MAX_OPTION 100
#define BYTE_SIZE 255
#define UNKNOWN 145
#define SAVE_SPACE 4
#define WARN_BYTES_FREE 100
#define MAX_WARNING 200
#define MAX_ERROR 200
#define MAX_INFO 200

/*** Intro slides. ***/
#define SLIDES 5
#define SLIDES_LINES 4
#define SLIDES_LINES_CHARS 14 /*** 13 + \0 ***/
#define SLIDES_BYTES 150
#define SLIDES_BYTES_MAX SLIDES * SLIDES_LINES * SLIDES_LINES_CHARS
#define SLIDES_OFFSET 0x13016

#define VERIFY_OFFSET 0x134
#define VERIFY_TEXT "PRINCE O PERSIA" /*** Sic. ***/
#define VERIFY_SIZE 15

#define BROKEN_ROOM_X 438
#define BROKEN_ROOM_Y 76
#define BROKEN_LEFT_X 423
#define BROKEN_LEFT_Y 76
#define BROKEN_RIGHT_X 453
#define BROKEN_RIGHT_Y 76
#define BROKEN_UP_X 438
#define BROKEN_UP_Y 61
#define BROKEN_DOWN_X 438
#define BROKEN_DOWN_Y 91

#define PNG_VARIOUS "various"
#define PNG_LIVING "living"
#define PNG_SLIVING "sliving"
#define PNG_BUTTONS "buttons"
#define PNG_EXTRAS "extras"
#define PNG_ROOMS "rooms"
#define PNG_GAMEPAD "gamepad"
#define PNG_ALPHABET "alphabet"

#define OFFSETD_X 25 /*** Pixels from the left, where tiles are visible. ***/
#define OFFSETD_Y 50 /*** Pixels from the top, where tiles are visible. ***/
#define TTPD_1 -16 /*** Top row, pixels behind interface. ***/
#define TTPD_O 0 /*** Other rows, pixels behind superjacent rows. ***/
#define DD_X 64 /*** Horizontal distance between (overlapping) tiles. ***/
#define DD_Y 160 /*** Vertical distance between (overlapping) tiles. ***/

#define TILEWIDTH 63 /*** On tiles screen. ***/
#define TILEHEIGHT 79 /*** On tiles screen. ***/
#define TILESX1 2 + (TILEWIDTH + 2) * 0
#define TILESX2 2 + (TILEWIDTH + 2) * 1
#define TILESX3 2 + (TILEWIDTH + 2) * 2
#define TILESX4 2 + (TILEWIDTH + 2) * 3
#define TILESX5 2 + (TILEWIDTH + 2) * 4
#define TILESX6 2 + (TILEWIDTH + 2) * 5
#define TILESX7 2 + (TILEWIDTH + 2) * 6
#define TILESX8 2 + (TILEWIDTH + 2) * 7
#define TILESX9 2 + (TILEWIDTH + 2) * 8
#define TILESX10 2 + (TILEWIDTH + 2) * 9
#define TILESY1 2 + (TILEHEIGHT + 2) * 0
#define TILESY2 2 + (TILEHEIGHT + 2) * 1
#define TILESY3 2 + (TILEHEIGHT + 2) * 2
#define TILESY4 2 + (TILEHEIGHT + 2) * 3
#define TILESY5 2 + (TILEHEIGHT + 2) * 4
#define TILESY6 2 + (TILEHEIGHT + 2) * 5

/*** Playtesting. ***/
#define OFFSET_TRAINING 0x664
#define OFFSET_START 0x2E3
#define OFFSET_REDORB 0x23E

#ifndef O_BINARY
#define O_BINARY 0
#endif
/*========== Defines ==========*/

int iDebug;
unsigned char arLevel[MAX_LEVEL_SIZE + 2];
unsigned char arLevelOut[MAX_LEVEL_SIZE + 2];
int iLevelSize;
int iLevelRead;
char sPathFile[MAX_PATHFILE + 2];
int iChanged;
int iScreen;
TTF_Font *font1;
TTF_Font *font2;
TTF_Font *font3;
SDL_Window *window;
SDL_Renderer *ascreen;
int iScale;
int iFullscreen;
SDL_Cursor *curArrow;
SDL_Cursor *curWait;
SDL_Cursor *curHand;
SDL_Cursor *curText;
int iNoAudio;
int iNoController;
int iPreLoaded;
int iDownAt;
int iSelected;
int iChangeEvent;
int iCurLevel;
int iExtras;
int arBrokenRoomLinks[LEVELS + 2];
int iCurRoom;
int iMovingRoom;
int iMovingNewBusy;
int iChangingBrokenRoom;
int iChangingBrokenSide;
int iLastTile;
int iXPos, iYPos;
int iInfo;
int arMovingRooms[ROOMS + 1 + 2][ROOMS + 2];
unsigned int gamespeed;
Uint32 looptime;
char cCurType;
int arDone[ROOMS + 2];
int iStartRoomsX, iStartRoomsY;
int iMovingNewX, iMovingNewY;
int iMinX, iMaxX, iMinY, iMaxY;
int iMovingOldX, iMovingOldY;
int arRoomConnectionsBroken[LEVELS + 2][ROOMS + 2][4 + 2];
int iOnTile;
int iCloseOn;
int iHelpOK;
int iEXESave;
int iOKOn;
int iYesOn;
int iNoOn;
int iCopied;
int iStartLevel;
int iOutOffset;
int iRepeatOffset;
int iHighStore;
unsigned char sUnknown[UNKNOWN + 2];
int iCustomTile;
int iEventTooltip, iEventTooltipOld;
int iCustomHover, iCustomHoverOld;
int iMednafen;
char sInfo[MAX_INFO + 2];
int iNoAnim;
int iFlameFrame;
int iModified;

/*** EXE ***/
int iEXEMinutesLeft;
int iEXEHitPoints;
int iEXEHair, iEXEHairR, iEXEHairG, iEXEHairB;
int iEXESkin, iEXESkinR, iEXESkinG, iEXESkinB;
int iEXESuit, iEXESuitR, iEXESuitG, iEXESuitB;
unsigned char sIntroSlides[SLIDES_BYTES_MAX + 2];
char arIntroSlides[SLIDES + 2]
	[SLIDES_LINES + 2][SLIDES_LINES_CHARS + 2];
int iS, iL, iN;
int iBytesLeft;
int arSlideSizes[SLIDES + 2];

/*** for text ***/
SDL_Color color_bl = {0x00, 0x00, 0x00, 255};
SDL_Color color_wh = {0xff, 0xff, 0xff, 255};
SDL_Color color_blue = {0x00, 0x00, 0xff, 255};
SDL_Color color_red = {0xff, 0x00, 0x00, 255};
SDL_Surface *message;
SDL_Texture *messaget;
SDL_Rect offset;

/*** for copying ***/
unsigned char arCopyPasteTile[TILES + 2];
unsigned char cCopyPasteGuardTile;
unsigned char cCopyPasteGuardDir;

/*** controller ***/
int iController;
SDL_GameController *controller;
char sControllerName[MAX_CON + 2];
SDL_Joystick *joystick;
SDL_Haptic *haptic;
Uint32 joyleft, joyright, joyup, joydown;
Uint32 trigleft, trigright;
int iXJoy1, iYJoy1, iXJoy2, iYJoy2;

/*** These are the levels. ***/
unsigned char arRoomTiles[LEVELS + 2][ROOMS + 2][TILES + 2];
unsigned char arRoomLinks[LEVELS + 2][ROOMS + 2][4 + 2];
unsigned char arStartLocation[LEVELS + 2][3 + 2];
unsigned char arGuardTile[LEVELS + 2][ROOMS + 2];
unsigned char arGuardDir[LEVELS + 2][ROOMS + 2];
unsigned char arEventsFromRoom[LEVELS + 2][EVENTS + 2];
unsigned char arEventsFromTile[LEVELS + 2][EVENTS + 2];
unsigned char arEventsOpenClose[LEVELS + 2][EVENTS + 2];
unsigned char arEventsToRoom[LEVELS + 2][EVENTS + 2];
unsigned char arEventsToTile[LEVELS + 2][EVENTS + 2];
int arNrEvents[LEVELS + 2];

int iDX, iDY, iTTP1, iTTPO;
int iHor[10 + 2];
int iVer0, iVer1, iVer2, iVer3, iVer4;

SDL_Texture *imgloading;
SDL_Texture *imgd[0xFF + 2][2 + 2];
SDL_Texture *imgp[0xFF + 2][2 + 2];
SDL_Texture *imgblack;
SDL_Texture *imgprincel[2 + 2], *imgprincer[2 + 2];
SDL_Texture *imgguardl[2 + 2], *imgguardr[2 + 2];
SDL_Texture *imgskeletonl[2 + 2], *imgskeletonr[2 + 2];
SDL_Texture *imgshadowl[2 + 2], *imgshadowr[2 + 2];
SDL_Texture *imgjaffarl[2 + 2], *imgjaffarr[2 + 2];
SDL_Texture *imgdisabled;
SDL_Texture *imgunk[2 + 2];
SDL_Texture *imgup_0;
SDL_Texture *imgup_1;
SDL_Texture *imgdown_0;
SDL_Texture *imgdown_1;
SDL_Texture *imgleft_0;
SDL_Texture *imgleft_1;
SDL_Texture *imgright_0;
SDL_Texture *imgright_1;
SDL_Texture *imgudno;
SDL_Texture *imglrno;
SDL_Texture *imgudnonfo;
SDL_Texture *imgprevon_0;
SDL_Texture *imgprevon_1;
SDL_Texture *imgnexton_0;
SDL_Texture *imgnexton_1;
SDL_Texture *imgprevoff;
SDL_Texture *imgnextoff;
SDL_Texture *imgbar;
SDL_Texture *imgextras[10 + 2];
SDL_Texture *imgroomson_0;
SDL_Texture *imgroomson_1;
SDL_Texture *imgroomsoff;
SDL_Texture *imgbroomson_0;
SDL_Texture *imgbroomson_1;
SDL_Texture *imgbroomsoff;
SDL_Texture *imgeventson_0;
SDL_Texture *imgeventson_1;
SDL_Texture *imgeventsoff;
SDL_Texture *imgsaveon_0;
SDL_Texture *imgsaveon_1;
SDL_Texture *imgsaveoff;
SDL_Texture *imgquit_0;
SDL_Texture *imgquit_1;
SDL_Texture *imgrl;
SDL_Texture *imgbrl;
SDL_Texture *imgsrc;
SDL_Texture *imgsrs;
SDL_Texture *imgsrm;
SDL_Texture *imgsrp;
SDL_Texture *imgsrb;
SDL_Texture *imgevents;
SDL_Texture *imgsele;
SDL_Texture *imgeventu;
SDL_Texture *imgsell;
SDL_Texture *imgdungeon;
SDL_Texture *imgpalace;
SDL_Texture *imgclosebig_0;
SDL_Texture *imgclosebig_1;
SDL_Texture *imgborderb;
SDL_Texture *imgborders;
SDL_Texture *imgbordersl;
SDL_Texture *imgborderbl;
SDL_Texture *imgfadedl;
SDL_Texture *imgpopup;
SDL_Texture *imgok[2 + 2];
SDL_Texture *imgsave[2 + 2];
SDL_Texture *imgpopup_yn;
SDL_Texture *imgyes[2 + 2];
SDL_Texture *imgno[2 + 2];
SDL_Texture *imghelp;
SDL_Texture *imgexe;
SDL_Texture *imgexewarning;
SDL_Texture *imgfadeds;
SDL_Texture *imgroom[25 + 2]; /*** 25 is "?", for all high room links ***/
SDL_Texture *imgetooltip;
SDL_Texture *imgchover;
SDL_Texture *imgmednafen;
SDL_Texture *imglinkwarnlr;
SDL_Texture *imglinkwarnud;
SDL_Texture *imgspriteflamed;
SDL_Texture *imgspriteflamep;
SDL_Texture *imgalphabet[26 + 2];
SDL_Texture *imgspace;
SDL_Texture *imgunknown;
SDL_Texture *imgseltextline;
SDL_Texture *imgvwarning;

struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

void ShowUsage (void);
void GetPathFile (void);
void LoadLevels (void);
int DecompressLevel (int iFd, int iOffset);
void SaveLevels (void);
void PrintTileName (int iLevel, int iRoom, int iTile, int iTileValue);
void AddDuplicates (int iByteToWrite, int iNrDuplicates);
void PrIfDe (char *sString);
char cShowDirection (int iDirection);
char cShowOpenClose (int iOpenClose);
int CompressLevel (int iNrBytes);
int AddRepeat (int iNrRepeated);
void Quit (void);
void InitScreen (void);
void InitPopUpSave (void);
void ShowPopUpSave (void);
void LoadFonts (void);
void MixAudio (void *unused, Uint8 *stream, int iLen);
void PlaySound (char *sFile);
void PreLoadSet (char cTypeP, int iTile);
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage);
void ShowScreen (void);
void InitPopUp (void);
void ShowPopUp (void);
void Help (void);
void ShowHelp (void);
void EXE (void);
void ShowEXE (void);
void InitScreenAction (char *sAction);
void RunLevel (int iLevel);
int StartGame (void *unused);
void ClearRoom (void);
void UseTile (int iTile, int iLocation, int iRoom);
void Zoom (int iToggleFull);
void LinkMinus (void);
int BrokenRoomLinks (int iPrint);
void ChangeEvent (int iAmount, int iChangePos);
void ChangeCustom (int iAmount);
void Prev (void);
void Next (void);
void CallSave (void);
void Sprinkle (void);
void SetLocation (int iRoom, int iLocation, int iTile);
void FlipRoom (int iAxis);
void CopyPaste (int iAction);
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY);
int MouseSelectAdj (void);
int OnLevelBar (void);
void ChangePos (void);
void RemoveOldRoom (void);
void AddNewRoom (int iX, int iY, int iRoom);
void LinkPlus (void);
void EventRoom (int iRoom, int iFromTo);
void EventTile (int iX, int iY, int iFromTo);
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo);
void CustomRenderCopy (SDL_Texture* src, SDL_Rect* srcrect,
	SDL_Rect *dstrect, char *sImageInfo);
void CreateBAK (void);
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font);
void InitRooms (void);
void WhereToStart (void);
void CheckSides (int iRoom, int iX, int iY);
void ShowRooms (int iRoom, int iX, int iY, int iNext);
void BrokenRoomChange (int iRoom, int iSide, int *iX, int *iY);
void ShowChange (void);
int OnTile (void);
void ChangePosAction (char *sAction);
void DisableSome (void);
int IsDisabled (int iTile);
void CenterNumber (int iNumber, int iX, int iY,
	SDL_Color fore, int iHex);
int Unused (int iTile);
int RaiseDropEvent (int iTile, int iEvent, int iAmount);
void OpenURL (char *sURL);
void EXELoad (void);
void EXESave (void);
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged);
void ColorRect (int iX, int iY, int iW, int iH, int iR, int iG, int iB);
void ToFiveBitRGB (int iIn, int *iR, int *iG, int *iB);
void TotalEvents (int iAmount);
void GetOptionValue (char *sArgv, char *sValue);
int IsEven (int iValue);
void IntroSlides (void);
void ModifyForMednafen (int iLevel);
void ModifyBack (void);

/*****************************************************************************/
int main (int argc, char *argv[])
/*****************************************************************************/
{
	int iArgLoop;
	SDL_version verc, verl;
	time_t tm;
	char sStartLevel[MAX_OPTION + 2];

	iDebug = 0;
	iExtras = 0;
	iLastTile = 0x00;
	iInfo = 0;
	iScale = 1;
	iOnTile = 1;
	iCopied = 0;
	iNoAudio = 0;
	iFullscreen = 0;
	iNoController = 0;
	iStartLevel = 1;
	iCustomTile = 0x00;
	iMednafen = 0;
	iNoAnim = 0;
	iModified = 0;

	if (argc > 1)
	{
		for (iArgLoop = 1; iArgLoop <= argc - 1; iArgLoop++)
		{
			if ((strcmp (argv[iArgLoop], "-h") == 0) ||
				(strcmp (argv[iArgLoop], "-?") == 0) ||
				(strcmp (argv[iArgLoop], "--help") == 0))
			{
				ShowUsage();
			}
			else if ((strcmp (argv[iArgLoop], "-v") == 0) ||
				(strcmp (argv[iArgLoop], "--version") == 0))
			{
				printf ("%s %s\n", EDITOR_NAME, EDITOR_VERSION);
				exit (EXIT_NORMAL);
			}
			else if ((strcmp (argv[iArgLoop], "-d") == 0) ||
				(strcmp (argv[iArgLoop], "--debug") == 0))
			{
				iDebug = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-n") == 0) ||
				(strcmp (argv[iArgLoop], "--noaudio") == 0))
			{
				iNoAudio = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-z") == 0) ||
				(strcmp (argv[iArgLoop], "--zoom") == 0))
			{
				iScale = 2;
			}
			else if ((strcmp (argv[iArgLoop], "-f") == 0) ||
				(strcmp (argv[iArgLoop], "--fullscreen") == 0))
			{
				iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			else if ((strncmp (argv[iArgLoop], "-l=", 3) == 0) ||
				(strncmp (argv[iArgLoop], "--level=", 8) == 0))
			{
				GetOptionValue (argv[iArgLoop], sStartLevel);
				iStartLevel = atoi (sStartLevel);
				if ((iStartLevel < 1) || (iStartLevel > LEVELS))
				{
					iStartLevel = 1;
				}
			}
			else if ((strcmp (argv[iArgLoop], "-s") == 0) ||
				(strcmp (argv[iArgLoop], "--static") == 0))
			{
				iNoAnim = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-k") == 0) ||
				(strcmp (argv[iArgLoop], "--keyboard") == 0))
			{
				iNoController = 1;
			}
			else
			{
				ShowUsage();
			}
		}
	}

	GetPathFile();

	srand ((unsigned)time(&tm));

	LoadLevels();

	/*** Show the SDL version used for compiling and linking. ***/
	if (iDebug == 1)
	{
		SDL_VERSION (&verc);
		SDL_GetVersion (&verl);
		printf ("[ INFO ] Compiled with SDL %u.%u.%u, linked with SDL %u.%u.%u.\n",
			verc.major, verc.minor, verc.patch, verl.major, verl.minor, verl.patch);
	}

	InitScreen();
	Quit();

	return 0;
}
/*****************************************************************************/
void ShowUsage (void)
/*****************************************************************************/
{
	printf ("%s %s\n%s\n\n", EDITOR_NAME, EDITOR_VERSION, COPYRIGHT);
	printf ("Usage:\n");
	printf ("  %s [OPTIONS]\n\nOptions:\n", EDITOR_NAME);
	printf ("  -h, -?,    --help           display this help and exit\n");
	printf ("  -v,        --version        output version information and"
		" exit\n");
	printf ("  -d,        --debug          also show levels on the console\n");
	printf ("  -n,        --noaudio        do not play sound effects\n");
	printf ("  -z,        --zoom           double the interface size\n");
	printf ("  -f,        --fullscreen     start in fullscreen mode\n");
	printf ("  -l=NR,     --level=NR       start in level NR\n");
	printf ("  -s,        --static         do not display animations\n");
	printf ("  -k,        --keyboard       do not use a game controller\n");
	printf ("\n");
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void GetPathFile (void)
/*****************************************************************************/
{
	int iFound;
	DIR *dDir;
	struct dirent *stDirent;
	char sExtension[100 + 2];
	char sError[MAX_ERROR + 2];
	char sVerify[VERIFY_SIZE + 2];
	int iFd;

	iFound = 0;

	dDir = opendir (ROM_DIR);
	if (dDir == NULL)
	{
		printf ("[FAILED] Cannot open directory \"%s\": %s!\n",
			ROM_DIR, strerror (errno));
		exit (EXIT_ERROR);
	}

	while ((stDirent = readdir (dDir)) != NULL)
	{
		if (iFound == 0)
		{
			if ((strcmp (stDirent->d_name, ".") != 0) &&
				(strcmp (stDirent->d_name, "..") != 0))
			{
				snprintf (sExtension, 100, "%s", strrchr (stDirent->d_name, '.'));
				if ((toupper (sExtension[1]) == 'G') &&
					(toupper (sExtension[2]) == 'B') &&
					(toupper (sExtension[3]) == 'C'))
				{
					iFound = 1;
					snprintf (sPathFile, MAX_PATHFILE, "%s%s%s", ROM_DIR, SLASH,
						stDirent->d_name);
					if (iDebug == 1)
					{
						printf ("[  OK  ] Found Game Boy Color ROM \"%s\".\n", sPathFile);
					}
				}
			}
		}
	}

	closedir (dDir);

	if (iFound == 0)
	{
		snprintf (sError, MAX_ERROR, "Cannot find a .gbc ROM in"
			" directory \"%s\"!", ROM_DIR);
		printf ("[FAILED] %s\n", sError);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Error", sError, NULL);
		exit (EXIT_ERROR);
	}

	/*** Is the file accessible? ***/
	if (access (sPathFile, R_OK|W_OK) == -1)
	{
		printf ("[FAILED] Cannot access \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Is the file a PoP1 for GBC ROM file? ***/
	iFd = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}
	lseek (iFd, VERIFY_OFFSET, SEEK_SET);
	read (iFd, sVerify, VERIFY_SIZE);
	close (iFd);
	sVerify[VERIFY_SIZE] = '\0';
	if (strcmp (sVerify, VERIFY_TEXT) != 0)
	{
		snprintf (sError, MAX_ERROR, "File %s is not a Prince of Persia"
			" for GBC ROM!", sPathFile);
		printf ("[FAILED] %s\n", sError);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Error", sError, NULL);
		exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
void LoadLevels (void)
/*****************************************************************************/
{
	int iFd;
	int iOffsetStart;
	int iOffsetEnd;
	int iLevel;
	int iTileValue;
	int iTiles;
	int iTemp;
	int iEventStart;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;
	int iByteLoop;
	int iSideLoop;
	int iGuardLoop;
	int iEventLoop;
	int iUnknownLoop;
	int iLevelLoop;

	iFd = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	iOffsetStart = OFFSET_LEVEL0;

	for (iLevelLoop = 1; iLevelLoop <= LEVELS; iLevelLoop++)
	{
		/*** We present level 0 to users as level 15. ***/
		switch (iLevelLoop)
		{
			case 1: iLevel = 15; break;
			case 16: iLevel = 16; break;
			case 17: iLevel = 17; break;
			default: iLevel = iLevelLoop - 1; break;
		}

		/*** This is the princess room during the ending. ***/
		if (iLevel == 16)
		{
			read (iFd, sUnknown, UNKNOWN);
			iOffsetStart+=UNKNOWN;
			if (iDebug == 1)
			{
				for (iUnknownLoop = 0; iUnknownLoop < UNKNOWN; iUnknownLoop++)
				{
					printf ("0x%02x ", sUnknown[iUnknownLoop]);
				}
				printf ("\n\n");
			}
		}

		/*** Decompress the level into arLevel. ***/
		if (iDebug == 1)
		{
			printf ("[ INFO ] Level %i starts at offset 0x%02x (%i).\n",
				iLevel, iOffsetStart, iOffsetStart);
		}
		iOffsetEnd = DecompressLevel (iFd, iOffsetStart);

		if (iDebug == 1)
		{
			printf ("[ INFO ] Level %i ends at offset 0x%02x (%i).\n",
				iLevel, iOffsetEnd, iOffsetEnd);
			printf ("[ INFO ] Compressed level size: %i\n",
				iOffsetEnd - iOffsetStart + 1);
			printf ("\n");
			for (iByteLoop = 0; iByteLoop < iLevelSize; iByteLoop++)
			{
				printf ("0x%02x ", arLevel[iByteLoop]);
			}
			printf ("\n");
		}

		iOffsetStart = iOffsetEnd + 1;

		/*** Extract tiles. ***/
		iTiles = -1;
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			if (iDebug == 1)
			{
				printf ("\n[Level %i] Room %i:\n\n", iLevel, iRoomLoop);
			}
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				iTiles++;
				iTileValue = arLevel[iTiles];
				arRoomTiles[iLevel][iRoomLoop][iTileLoop] = iTileValue;

				/*** Debug. ***/
				if (iDebug == 1)
				{
					if (iTileValue == 0xFF)
					{
						printf ("Unused room.\n");
					} else {
						PrintTileName (iLevel, iRoomLoop, iTileLoop, iTileValue);
						if ((iTileLoop == 10) || (iTileLoop == 20))
						{
							printf ("\n");
							for (iTemp = 1; iTemp <= 79; iTemp++) { printf ("-"); }
							printf ("\n");
						} else if (iTileLoop != 30) { printf ("|"); }
					}
				}

				if (iTileValue == 0xFF) { break; }
			}
			PrIfDe ("\n");
		}

		/*** Extract room links. ***/
		PrIfDe ("[  OK  ] Loading: Room Links\n");
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			for (iSideLoop = 1; iSideLoop <= 4; iSideLoop++)
			{
				iTiles++;
				arRoomLinks[iLevel][iRoomLoop][iSideLoop] = arLevel[iTiles];
			}
			if (iDebug == 1)
			{
				printf ("[ INFO ] Room %i is connected to room (0 = none):"
					" l%i, r%i, u%i, d%i\n", iRoomLoop,
					arRoomLinks[iLevel][iRoomLoop][1],
					arRoomLinks[iLevel][iRoomLoop][2],
					arRoomLinks[iLevel][iRoomLoop][3],
					arRoomLinks[iLevel][iRoomLoop][4]);
			}
		}

		/*** Extract start location. ***/
		iTiles++;
		arStartLocation[iLevel][1] = arLevel[iTiles]; /*** Room. ***/
		iTiles++;
		arStartLocation[iLevel][2] = arLevel[iTiles] + 1; /*** Tile. ***/
		iTiles++;
		arStartLocation[iLevel][3] = arLevel[iTiles]; /*** Direction. ***/
		if (iDebug == 1)
		{
			printf ("[ INFO ] The prince starts in room: %i, tile %i, turned: %c\n",
				arStartLocation[iLevel][1], arStartLocation[iLevel][2],
				cShowDirection (arStartLocation[iLevel][3]));
		}

		/*** Extract guards. ***/
		for (iGuardLoop = 1; iGuardLoop <= ROOMS; iGuardLoop++)
		{
			iTiles++;
			arGuardTile[iLevel][iGuardLoop] = arLevel[iTiles];
			/*** This forces the direction bit to 0. Thanks Stack Overflow. ***/
			arGuardTile[iLevel][iGuardLoop]
				^= (-0 ^ arGuardTile[iLevel][iGuardLoop]) & (1 << 7);
			arGuardTile[iLevel][iGuardLoop]++;
			/*** Get the direction bit. ***/
			if (arLevel[iTiles] >= 128)
			{
				arGuardDir[iLevel][iGuardLoop] = 0xFF; /*** l ***/
			} else {
				arGuardDir[iLevel][iGuardLoop] = 0x00; /*** r ***/
			}
/***
			switch (arLevel[iTiles] & 1) // 1 = 00000001
			{
				case 1: arGuardDir[iLevel][iGuardLoop] = 0xFF; break; // l
				case 0: arGuardDir[iLevel][iGuardLoop] = 0x00; break; // r
			}
***/

			if (iDebug == 1)
			{
				if (arGuardTile[iLevel][iGuardLoop] <= TILES)
				{
					printf ("[ INFO ] A guard in room: %i, tile %i, turned: %c\n",
						iGuardLoop, arGuardTile[iLevel][iGuardLoop],
						cShowDirection (arGuardDir[iLevel][iGuardLoop]));
				}
			}
		}

		/*** Extract events. ***/
		arNrEvents[iLevel] = 0;
		for (iEventLoop = 1; iEventLoop <= EVENTS; iEventLoop++)
		{
			iEventStart = iTiles + ((iEventLoop - 1) * 5);
			if (arLevel[iEventStart + 1] == 0xFF) { break; }
			arEventsFromRoom[iLevel][iEventLoop] = arLevel[iEventStart + 1];
			arEventsFromTile[iLevel][iEventLoop] = arLevel[iEventStart + 2] + 1;
			arEventsOpenClose[iLevel][iEventLoop] = arLevel[iEventStart + 3];
			arEventsToRoom[iLevel][iEventLoop] = arLevel[iEventStart + 4];
			arEventsToTile[iLevel][iEventLoop] = arLevel[iEventStart + 5] + 1;
			arNrEvents[iLevel]++;

			if (iDebug == 1)
			{
				printf ("[ INFO ] Event: room %i, tile %i -%c- room %i tile %i\n",
					arEventsFromRoom[iLevel][iEventLoop],
					arEventsFromTile[iLevel][iEventLoop],
					cShowOpenClose (arEventsOpenClose[iLevel][iEventLoop]),
					arEventsToRoom[iLevel][iEventLoop],
					arEventsToTile[iLevel][iEventLoop]);
			}
		}

		PrIfDe ("[  OK  ] Checking for broken room links.\n");
		arBrokenRoomLinks[iLevel] = BrokenRoomLinks (1);

		if (iDebug == 1)
		{
			printf ("[  OK  ] Done processing level %i.\n\n", iLevel);
		}
	}

	close (iFd);
}
/*****************************************************************************/
int DecompressLevel (int iFd, int iOffset)
/*****************************************************************************/
{
	int iRead;
	unsigned char sRead[2 + 2];
	int iEOF;
	int iSection;
	int iRepeatedBytes;
	int iSubLoop;
	int arSubFrom[256 + 2];
	int arSubTo[256 + 2];
	int iNrDuplicates;
	int iNrDuplicatesNext;
	int iHighNibble, iLowNibble;
	int iNeedDuplicates;
	int iByteToWrite;
	int iDone;

	/*** Used for the level size. ***/
	int iLevelSizeA;
	int iLevelSizeB;
	char sLevelSize[10 + 2];

	lseek (iFd, iOffset, SEEK_SET);
	iEOF = 0;
	iSection = 0;
	iLevelRead = 0;
	iNrDuplicates = -1;
	iNrDuplicatesNext = -1;
	iRepeatedBytes = 0; /*** To prevent warnings. ***/
	do {
		iRead = read (iFd, sRead, 1);
		iOffset++;

		switch (iRead)
		{
			case -1:
				printf ("[FAILED] Could not read from \"%s\": %s!\n",
					sPathFile, strerror (errno));
				exit (EXIT_ERROR);
				break;
			case 0: iEOF = 1; break;
			default:
				switch (iSection)
				{
					case 0:
						/* The number of repeated bytes that have single
						 * byte replacements.
						 */
						iRepeatedBytes = sRead[0];

						/* Store the repeated bytes and their single
						 * byte replacements.
						 */
						for (iSubLoop = 1; iSubLoop <= iRepeatedBytes; iSubLoop++)
						{
							iRead = read (iFd, sRead, 2);
							iOffset+=2;
							arSubFrom[iSubLoop] = sRead[1];
							arSubTo[iSubLoop] = sRead[0];
							if (iDebug == 1)
							{
								printf ("[ INFO ] Substitute: 0x%02x -> 0x%02x\n",
									sRead[1], sRead[0]);
							}
						}
						iSection++;
						break;
					case 1:
						/*** Uncompressed level size. ***/
						iLevelSizeA = sRead[0];
						iRead = read (iFd, sRead, 1);
						iOffset++;
						iLevelSizeB = sRead[0];
						snprintf (sLevelSize, 10, "%02x%02x", iLevelSizeB, iLevelSizeA);
						iLevelSize = strtoul (sLevelSize, NULL, 16);
						if (iDebug == 1)
						{
							printf ("[ INFO ] Uncompressed level size: %i (0x%02x 0x%02x)\n",
								iLevelSize, iLevelSizeB, iLevelSizeA);
						}
						iSection++;
						break;
					case 2:
						/*** Level. ***/

						/*** Write the byte. ***/
						iByteToWrite = sRead[0];
						for (iSubLoop = 1; iSubLoop <= iRepeatedBytes; iSubLoop++)
						{
							if (arSubFrom[iSubLoop] == iByteToWrite)
							{
								iByteToWrite = arSubTo[iSubLoop];
								break;
							}
						}
						arLevel[iLevelRead] = iByteToWrite;
						iLevelRead++;

						/*** Does the last byte need duplicates? ***/
						iNeedDuplicates = 0;
						for (iSubLoop = 1; iSubLoop <= iRepeatedBytes; iSubLoop++)
						{
							if (arSubTo[iSubLoop] == sRead[0])
							{
								iNeedDuplicates = 1;
							}
						}

						/*** Add duplicates. ***/
						if (iNeedDuplicates == 1)
						{
							iDone = 0;
							do {
								if ((iNrDuplicates == -1) && (iNrDuplicatesNext != -1))
								{
									iNrDuplicates = iNrDuplicatesNext;
									iNrDuplicatesNext = -1;
								}

								if (iNrDuplicates == -1)
								{
									iRead = read (iFd, sRead, 1);
									iOffset++;
									iHighNibble = sRead[0] >> 4;
									iLowNibble = sRead[0] & 0x0F; /*** 0F = 00001111 ***/
									if (iDebug == 1)
									{
										printf ("[ INFO ] Duplicate (0x%02x): hi %i lo %i\n",
											sRead[0], iHighNibble, iLowNibble);
									}
									iNrDuplicates = iHighNibble;
									iNrDuplicatesNext = iLowNibble;
								}
								AddDuplicates (iByteToWrite, iNrDuplicates);
								if (iNrDuplicates != 0x0F) { iDone = 1; }
								iNrDuplicates = -1;
							} while (iDone == 0);
						}
						break;
				}
				break;
		}
	} while ((iEOF == 0) && ((iLevelRead < iLevelSize) || (iLevelSize == 0)));
	iOffset--;

	return (iOffset);
}
/*****************************************************************************/
void SaveLevels (void)
/*****************************************************************************/
{
	int iFd;
	int iByte;
	int iBit;
	int iBytesOut;
	int iBytesOutTotal;
	char sToWrite[MAX_TOWRITE + 2];
	int iLevel;
	off_t oOffset;
	int arRelativeOffset[LEVELS + 2];
	int iNrFF, iNrFFLoop;
	char sWarning[MAX_WARNING + 2];

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;
	int iEventLoop;
	int iByteLoop;
	int iLevelLoop;

	iFd = open (sPathFile, O_WRONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
	}

	lseek (iFd, OFFSET_LEVEL0, SEEK_SET);
	iBytesOutTotal = 0;

	for (iLevelLoop = 1; iLevelLoop <= LEVELS; iLevelLoop++)
	{
		switch (iLevelLoop)
		{
			case 1: iLevel = 15; break;
			case 16: iLevel = 16; break;
			case 17: iLevel = 17; break;
			default: iLevel = iLevelLoop - 1; break;
		}

		if (iLevel == 16)
		{
			write (iFd, sUnknown, UNKNOWN);
		}

		/*** Remember the new level start offsets. ***/
		arRelativeOffset[iLevelLoop] = 0x5CC4 + iBytesOutTotal;

		iByte = 0;
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				arLevel[iByte] = arRoomTiles[iLevel][iRoomLoop][iTileLoop];
				if (arLevel[iByte] == 0xFF) { iTileLoop+=29; }
				iByte++;
			}
		}

		/*** Room links. ***/
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			arLevel[iByte] = arRoomLinks[iLevel][iRoomLoop][1];
			arLevel[iByte + 1] = arRoomLinks[iLevel][iRoomLoop][2];
			arLevel[iByte + 2] = arRoomLinks[iLevel][iRoomLoop][3];
			arLevel[iByte + 3] = arRoomLinks[iLevel][iRoomLoop][4];
			iByte+=4;
		}

		/*** Start location. ***/
		arLevel[iByte] = arStartLocation[iLevel][1];
		arLevel[iByte + 1] = arStartLocation[iLevel][2] - 1;
		arLevel[iByte + 2] = arStartLocation[iLevel][3];
		iByte+=3;

		/*** Guards. ***/
		for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
		{
			arLevel[iByte] = arGuardTile[iLevel][iRoomLoop] - 1;

			/*** Make room for - by setting - the direction bit. ***/
			arLevel[iByte] = arLevel[iByte] + 128;

			/*** Obtain direction bit. ***/
			switch (arGuardDir[iLevel][iRoomLoop])
			{
				case 0xFF: iBit = 1; break; /*** l ***/
				case 0x00: iBit = 0; break; /*** r ***/
				default:
					printf ("[FAILED] Incorrect direction: 0x%02x\n",
						arGuardDir[iLevel][iRoomLoop]);
					exit (EXIT_ERROR);
					break;
			}

			/*** Set direction bit. Again, thanks Stack Overflow. ***/
			arLevel[iByte]
				^= (-iBit ^ arLevel[iByte]) & (1 << 7);

			iByte++;
		}

		/*** Events. ***/
		for (iEventLoop = 1; iEventLoop <= arNrEvents[iLevel]; iEventLoop++)
		{
			arLevel[iByte] = arEventsFromRoom[iLevel][iEventLoop];
			arLevel[iByte + 1] = arEventsFromTile[iLevel][iEventLoop] - 1;
			arLevel[iByte + 2] = arEventsOpenClose[iLevel][iEventLoop];
			arLevel[iByte + 3] = arEventsToRoom[iLevel][iEventLoop];
			arLevel[iByte + 4] = arEventsToTile[iLevel][iEventLoop] - 1;
			iByte+=5;
		}

		/*** End. ***/
		arLevel[iByte] = 0xFF;
		iByte++;

		iBytesOut = CompressLevel (iByte);

		for (iByteLoop = 0; iByteLoop < iBytesOut; iByteLoop++)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c", arLevelOut[iByteLoop]);
			write (iFd, sToWrite, 1);
		}

		iBytesOutTotal+=iBytesOut;
	}

	/*** Fill the rest with 0xFF. ***/
	oOffset = lseek (iFd, 0, SEEK_CUR);
	iNrFF = 0x1FFFF - oOffset;
	snprintf (sToWrite, MAX_TOWRITE, "%c", 0xFF);
	for (iNrFFLoop = 1; iNrFFLoop <= iNrFF; iNrFFLoop++)
		{ write (iFd, sToWrite, 1); }
	if (iNrFF <= WARN_BYTES_FREE)
	{
		snprintf (sWarning, MAX_WARNING,
			"All levels combined leave only %i free bytes! Use fewer rooms.", iNrFF);
		printf ("[ WARN ] %s\n", sWarning);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_WARNING,
			"Warning", sWarning, window);
	} else if (iDebug == 1) {
		printf ("[ INFO ] Free bytes left in the levels area: %i\n", iNrFF);
	}

	/*** Store the new level start offsets to the offsets table. ***/
	lseek (iFd, 0x1DC2C, SEEK_SET);
	/*** 0-15 ***/
	for (iLevelLoop = 1; iLevelLoop < LEVELS; iLevelLoop++)
	{
		sToWrite[0] = (arRelativeOffset[iLevelLoop] >> 0) & 0xFF;
		sToWrite[1] = (arRelativeOffset[iLevelLoop] >> 8) & 0xFF;
		write (iFd, sToWrite, 2);
	}
	/*** 0 ***/
	sToWrite[0] = (arRelativeOffset[1] >> 0) & 0xFF;
	sToWrite[1] = (arRelativeOffset[1] >> 8) & 0xFF;
	write (iFd, sToWrite, 2);
	/*** 16 ***/
	sToWrite[0] = ((arRelativeOffset[16] + UNKNOWN) >> 0) & 0xFF;
	sToWrite[1] = ((arRelativeOffset[16] + UNKNOWN) >> 8) & 0xFF;
	write (iFd, sToWrite, 2);
	/*** 17 ***/
	sToWrite[0] = ((arRelativeOffset[17] + UNKNOWN) >> 0) & 0xFF;
	sToWrite[1] = ((arRelativeOffset[17] + UNKNOWN) >> 8) & 0xFF;
	write (iFd, sToWrite, 2);

	close (iFd);

	PlaySound ("wav/save.wav");

	iChanged = 0;
}
/*****************************************************************************/
void PrintTileName (int iLevel, int iRoom, int iTile, int iTileValue)
/*****************************************************************************/
{
	int iHighNibble, iLowNibble;
	char sToPrint[10 + 2];

	iHighNibble = iTileValue >> 4;
	iLowNibble = iTileValue & 0x0F; /*** 0F = 00001111 ***/

	if ((iLowNibble == 0x0F) && (IsEven (iHighNibble))) /*** Raise. ***/
	{
		snprintf (sToPrint, 10, "raise%2i", iHighNibble);
		printf ("%s", sToPrint);
	} else if ((iLowNibble == 0x06) && (IsEven (iHighNibble))) { /*** Drop. ***/
		snprintf (sToPrint, 10, "drop%3i", iHighNibble);
		printf ("%s", sToPrint);
	} else switch (iTileValue) {
		case 0x00: printf ("empty 1"); break;
		case 0x01: printf ("floor 1"); break;
		case 0x02: printf ("spikes "); break;
		case 0x03: printf ("pillar "); break;
		case 0x05: printf ("mirrora"); break;
		case 0x07: printf ("arch+ta"); break;
		case 0x08: printf ("pillarb"); break;
		case 0x09: printf ("pillart"); break;
		case 0x0A: printf ("potionp"); break;
		case 0x0B: printf ("loose  "); break;
		case 0x0C: printf ("gatetop"); break;
		case 0x0D: printf ("mirror "); break;
		case 0x0E: printf ("rubble "); break;
		case 0x10: printf ("exit lO"); break;
		case 0x11: printf ("exit ri"); break;
		case 0x12: printf ("chomper"); break;
		case 0x13: printf ("torch  "); break;
		case 0x14: printf ("wall 1 "); break;
		case 0x15: printf ("skeleto"); break;
		case 0x16: printf ("sword  "); break;
		case 0x17: printf ("mirrorl"); break;
		case 0x18: printf ("balcony"); break;
		case 0x19: printf ("archbot"); break;
		case 0x1A: printf ("archtop"); break;
		case 0x1B: printf ("arch   "); break;
		case 0x1C: printf ("archlef"); break;
		case 0x1D: printf ("archrig"); break;
		case 0x1F: printf ("floorin"); break;
		case 0x20: printf ("emptys1"); break;
		case 0x21: printf ("floor+s"); break;
		case 0x24: printf ("gate op"); break;
		case 0x27: printf ("tapesf1"); break;
		case 0x2A: printf ("potionr"); break;
		case 0x2B: printf ("loose t"); break;
		case 0x2C: printf ("tapese1"); break;
		case 0x34: printf ("wall 2 "); break;
		case 0x40: printf ("emptys2"); break;
		case 0x41: printf ("floor+p"); break;
		case 0x44: printf ("gate cl"); break;
		case 0x47: printf ("tapesf2"); break;
		case 0x4A: printf ("potionl"); break;
		case 0x4C: printf ("tapese2"); break;
		case 0x50: printf ("exit lC"); break;
		case 0x60: printf ("window "); break;
		case 0x6A: printf ("potionf"); break;
		case 0x8A: printf ("potion^"); break;
		case 0xAA: printf ("potionb"); break;
		case 0xE0: printf ("empty 2"); break;
		case 0xE1: printf ("floor 2"); break;
		default:
			printf ("\n[ WARN ] Unknown tile in level %i, room %i,"
				" tile %i: 0x%02x!\n", iLevel, iRoom, iTile, iTileValue);
			break;
	}
}
/*****************************************************************************/
void AddDuplicates (int iByteToWrite, int iNrDuplicates)
/*****************************************************************************/
{
	int iDupLoop;

	for (iDupLoop = 1; iDupLoop <= iNrDuplicates; iDupLoop++)
	{
		arLevel[iLevelRead] = iByteToWrite;
		iLevelRead++;
	}
}
/*****************************************************************************/
void PrIfDe (char *sString)
/*****************************************************************************/
{
	if (iDebug == 1) { printf ("%s", sString); }
}
/*****************************************************************************/
char cShowDirection (int iDirection)
/*****************************************************************************/
{
	switch (iDirection)
	{
		case 0x00: return ('r'); break;
		case 0xFF: return ('l'); break;
	}
	return ('?');
}
/*****************************************************************************/
char cShowOpenClose (int iOpenClose)
/*****************************************************************************/
{
	switch (iOpenClose)
	{
		case 0x00: return ('c'); break; /*** Close. ***/
		case 0x01: return ('o'); break; /*** Open. ***/
	}
	return ('?');
}
/*****************************************************************************/
int CompressLevel (int iNrBytes)
/*****************************************************************************/
{
	unsigned char cLastByte;
	unsigned char cProcessingByte;
	unsigned char arBytesUnused[BYTE_SIZE + 2];
	unsigned char arBytesRepeated[BYTE_SIZE + 2];
	unsigned char arBytesReplace[BYTE_SIZE + 2];
	int arBytesSame[BYTE_SIZE + 2];
	int arBytesNotSame[BYTE_SIZE + 2];
	int arCouldSave[BYTE_SIZE + 2];
	int iNrRepeated;
	int iByte;
	int iCouldSave;
	int iRepeatedBytes;

	/*** Used for looping. ***/
	int iByteLoop;
	int iByteLoop2;

	/*** Defaults. ***/
	iOutOffset = 0;
	iRepeatedBytes = 0;
	iByte = 1;
	cLastByte = arLevel[0];
	for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
	{
		arBytesUnused[iByteLoop] = 0;
		arCouldSave[iByteLoop] = 0;
		arBytesRepeated[iByteLoop] = 0;
		arBytesSame[iByteLoop] = 0;
		arBytesNotSame[iByteLoop] = 0;
	}

	/*** Force the very last byte to be different from the previous. ***/
	switch (arLevel[iNrBytes - 1])
	{
		case 0: arLevel[iNrBytes] = 1; break;
		default: arLevel[iNrBytes] = 0; break;
	}

	/* For all bytes, check how many times they are the same as previous bytes
	 * and how many of them are not.
	 */
	while (iByte <= iNrBytes)
	{
		iNrRepeated = 0;
		while (arLevel[iByte++] == cLastByte)
		{
			/*** Do not move iByte++ here. ***/
			iNrRepeated++;
		}
		if (iNrRepeated != 0)
		{
			arBytesSame[cLastByte] = arBytesSame[cLastByte]
				+ (iNrRepeated * 2) - ((iNrRepeated / 0x0F) + 1);
		} else {
			arBytesNotSame[cLastByte]++;
		}
		cLastByte = arLevel[iByte - 1];
	}

	/*** Mark unused bytes for later use. ***/
	for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
	{
		if ((arBytesSame[iByteLoop] == 0) &&
			(arBytesNotSame[iByteLoop] == 0))
		{
			arBytesUnused[iByteLoop] = 1;
		}
	}

	for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
	{
		if (arBytesSame[iByteLoop] - arBytesNotSame[iByteLoop] > SAVE_SPACE)
		{
			/* Using a repeat count for byte iByteLoop saves bytes. This
			 * means we will use repeat counts with iByteLoop.
			 */
			arCouldSave[iByteLoop] = arBytesNotSame[iByteLoop];
			arBytesRepeated[iByteLoop] = 1;
			iRepeatedBytes++;
		} else if (arBytesSame[iByteLoop] > SAVE_SPACE) {
			arCouldSave[iByteLoop] = arBytesSame[iByteLoop] - SAVE_SPACE;
		}
	}

	for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
	{
		arBytesReplace[iByteLoop] = iByteLoop;
	}

	/* If there are bytes unused, check for the largest arCouldSave[] value,
	 * and then start the repeat and replace process for these.
	 */
	for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
	{
		if (arBytesUnused[iByteLoop] == 1)
		{
			/*** Get the byte that will save the most space. ***/
			iCouldSave = 0;
			iByte = 0;
			for (iByteLoop2 = 0; iByteLoop2 <= BYTE_SIZE; iByteLoop2++)
			{
				if (arCouldSave[iByteLoop2] > iCouldSave)
				{
					iCouldSave = arCouldSave[iByteLoop2];
					iByte = iByteLoop2;
				}
			}

			if (arCouldSave[iByte] > 0)
			{
				/*** iByteLoop will replace iByte. ***/
				arBytesReplace[iByte] = iByteLoop;

				if (arBytesRepeated[iByte] == 0)
				{
					iRepeatedBytes++;
				}
				arBytesRepeated[iByte] = 2;

				/*** iByte is now (already) being replaced. ***/
				arCouldSave[iByte] = 0;

				/*** iByteLoop is now (already) in use. ***/
			 	arBytesUnused[iByteLoop] = 0;
			}
		}
	}

	/* The number of repeated bytes that have single
	 * byte replacements.
	 */
	arLevelOut[iOutOffset] = iRepeatedBytes;
	iOutOffset++;

	/*** Add the repeated bytes and their single byte replacements. ***/
	if (iRepeatedBytes > 0)
	{
		for (iByteLoop = 0; iByteLoop <= BYTE_SIZE; iByteLoop++)
		{
			if (arBytesRepeated[iByteLoop] != 0)
			{
				arLevelOut[iOutOffset] = iByteLoop;
				arLevelOut[iOutOffset + 1] = arBytesReplace[iByteLoop];
				iOutOffset+=2;
			}
		}
	}

	/*** Uncompressed level size. ***/
	arLevelOut[iOutOffset] = (iNrBytes >> 0) & 0xFF;
	arLevelOut[iOutOffset + 1] = (iNrBytes >> 8) & 0xFF;
	iOutOffset+=2;

	iRepeatOffset = 0;
	iByte = 0;
	while (iByte < iNrBytes)
	{
		cProcessingByte = arLevel[iByte];
		iByte++;
		arLevelOut[iOutOffset] = cProcessingByte;
		iOutOffset++;
		if (arBytesRepeated[cProcessingByte] != 0)
		{
			iNrRepeated = 0;
			while (arLevel[iByte] == cProcessingByte)
			{
				iByte++;
				iNrRepeated++;
			}

			if (iNrRepeated != 0)
			{
				/* Separate repeat counts never exceed 0x0F, because half a
				 * byte is 4 bits, which is 15.
				 */
				while (iNrRepeated >= 0x0F)
				{
					AddRepeat (0x0F);
					iNrRepeated-=0x0F;
				}
				AddRepeat (iNrRepeated);
			} else {
				if (arBytesRepeated[cProcessingByte] == 1)
				{
					AddRepeat (0);
				} else {
					/*** Replace single occurrences of bytes that get repeat counts. ***/
					arLevelOut[iOutOffset - 1] = arBytesReplace[cProcessingByte];
				}
			}
		}
	}

	/* If we are still remembering a repeat count (iHighStore), we must
	 * add it to the level. By itself, without another repeat count.
	 */
	if (iRepeatOffset != 0) { AddRepeat (0); }

	return (iOutOffset);
}
/*****************************************************************************/
int AddRepeat (int iNrRepeated)
/*****************************************************************************/
{
	if (iRepeatOffset != 0)
	{
		/* Store both the previous (iHighStore) and current repeat counts at
		 * iRepeatOffset; after the byte that required a repeat.
		 */
		arLevelOut[iRepeatOffset] = (iHighStore << 4) + iNrRepeated;
		iRepeatOffset = 0;
	} else {
		/*** Remember the repeat count. ***/
		iHighStore = iNrRepeated;
		iRepeatOffset = iOutOffset;
		iOutOffset++;
	}

	return (0);
}
/*****************************************************************************/
void Quit (void)
/*****************************************************************************/
{
	if (iChanged != 0) { InitPopUpSave(); }
	if (iModified == 1) { ModifyBack(); }
	TTF_CloseFont (font1);
	TTF_CloseFont (font2);
	TTF_CloseFont (font3);
	TTF_Quit();
	SDL_Quit();
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void InitScreen (void)
/*****************************************************************************/
{
	SDL_AudioSpec fmt;
	char sImage[MAX_IMG + 2];
	SDL_Surface *imgicon;
	int iJoyNr;
	SDL_Event event;
	int iOldXPos, iOldYPos;
	const Uint8 *keystate;
	Uint32 oldticks, newticks;
	int iEventRoom;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iRoomLoop2;
	int iTileLoop;
	int iColLoop, iRowLoop;
	int iAlphabetLoop;

	if (SDL_Init (SDL_INIT_AUDIO|SDL_INIT_VIDEO|
		SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC) < 0)
	{
		printf ("[FAILED] Unable to init SDL: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	atexit (SDL_Quit);

	window = SDL_CreateWindow (EDITOR_NAME " " EDITOR_VERSION,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(WINDOW_WIDTH) * iScale, (WINDOW_HEIGHT) * iScale, iFullscreen);
	if (window == NULL)
	{
		printf ("[FAILED] Unable to create a window: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	ascreen = SDL_CreateRenderer (window, -1, 0);
	if (ascreen == NULL)
	{
		printf ("[FAILED] Unable to set video mode: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	/*** Some people may prefer linear, but we're going old school. ***/
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	if (iFullscreen != 0)
	{
		SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
			(WINDOW_HEIGHT) * iScale);
	}

	if (TTF_Init() == -1)
	{
		printf ("[FAILED] Could not initialize TTF!\n");
		exit (EXIT_ERROR);
	}

	LoadFonts();

	curArrow = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW);
	curWait = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT);
	curHand = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_HAND);
	curText = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_IBEAM);

	if (iNoAudio != 1)
	{
		PrIfDe ("[  OK  ] Initializing Audio\n");
		fmt.freq = 44100;
		fmt.format = AUDIO_S16;
		fmt.channels = 2;
		fmt.samples = 512;
		fmt.callback = MixAudio;
		fmt.userdata = NULL;
		if (SDL_OpenAudio (&fmt, NULL) < 0)
		{
			printf ("[FAILED] Unable to open audio: %s!\n", SDL_GetError());
			exit (EXIT_ERROR);
		}
		SDL_PauseAudio (0);
	}

	/*** icon ***/
	snprintf (sImage, MAX_IMG, "png%svarious%slegbop_icon.png",
		SLASH, SLASH);
	imgicon = IMG_Load (sImage);
	if (imgicon == NULL)
	{
		printf ("[ WARN ] Could not load \"%s\": %s!\n", sImage, strerror (errno));
	} else {
		SDL_SetWindowIcon (window, imgicon);
	}

	/*** Open the first available controller. ***/
	iController = 0;
	if (iNoController != 1)
	{
		for (iJoyNr = 0; iJoyNr < SDL_NumJoysticks(); iJoyNr++)
		{
			if (SDL_IsGameController (iJoyNr))
			{
				controller = SDL_GameControllerOpen (iJoyNr);
				if (controller)
				{
					snprintf (sControllerName, MAX_CON, "%s",
						SDL_GameControllerName (controller));
					if (iDebug == 1)
					{
						printf ("[ INFO ] Found a controller \"%s\"; \"%s\".\n",
							sControllerName, SDL_GameControllerNameForIndex (iJoyNr));
					}
					joystick = SDL_GameControllerGetJoystick (controller);
					iController = 1;

					/*** Just for fun, use haptic. ***/
					if (SDL_JoystickIsHaptic (joystick))
					{
						haptic = SDL_HapticOpenFromJoystick (joystick);
						if (SDL_HapticRumbleInit (haptic) == 0)
						{
							SDL_HapticRumblePlay (haptic, 1.0, 1000);
						} else {
							printf ("[ WARN ] Could not initialize the haptic device: %s!\n",
								SDL_GetError());
						}
					} else {
						PrIfDe ("[ INFO ] The game controller is not haptic.\n");
					}
				} else {
					printf ("[ WARN ] Could not open game controller %i: %s!\n",
						iController, SDL_GetError());
				}
			}
		}
		if (iController != 1) { PrIfDe ("[ INFO ] No controller found.\n"); }
	} else {
		PrIfDe ("[ INFO ] Using keyboard and mouse.\n");
	}

	/*******************/
	/* Preload images. */
	/*******************/

	/*** Loading... ***/
	PreLoad (PNG_VARIOUS, "loading.png", &imgloading);
	ShowImage (imgloading, 0, 0, "imgloading");
	SDL_RenderPresent (ascreen);

	iPreLoaded = 0;
	SDL_SetCursor (curWait);

	/*** Dungeon and palace tiles. ***/
	PreLoadSet ('d', 0x00); PreLoadSet ('p', 0x00);
	PreLoadSet ('d', 0x01); PreLoadSet ('p', 0x01);
	PreLoadSet ('d', 0x02); PreLoadSet ('p', 0x02);
	PreLoadSet ('d', 0x03); PreLoadSet ('p', 0x03);
	PreLoadSet ('d', 0x05); PreLoadSet ('p', 0x05);
	PreLoadSet ('d', 0x06); PreLoadSet ('p', 0x06);
	PreLoadSet ('d', 0x07); PreLoadSet ('p', 0x07);
	PreLoadSet ('d', 0x08); PreLoadSet ('p', 0x08);
	PreLoadSet ('d', 0x09); PreLoadSet ('p', 0x09);
	PreLoadSet ('d', 0x0A); PreLoadSet ('p', 0x0A);
	PreLoadSet ('d', 0x0B); PreLoadSet ('p', 0x0B);
	PreLoadSet ('d', 0x0C); PreLoadSet ('p', 0x0C);
	PreLoadSet ('d', 0x0D); PreLoadSet ('p', 0x0D);
	PreLoadSet ('d', 0x0E); PreLoadSet ('p', 0x0E);
	PreLoadSet ('d', 0x0F); PreLoadSet ('p', 0x0F);
	PreLoadSet ('d', 0x10); PreLoadSet ('p', 0x10);
	PreLoadSet ('d', 0x11); PreLoadSet ('p', 0x11);
	PreLoadSet ('d', 0x12); PreLoadSet ('p', 0x12);
	PreLoadSet ('d', 0x13); PreLoadSet ('p', 0x13);
	PreLoadSet ('d', 0x14); PreLoadSet ('p', 0x14);
	PreLoadSet ('d', 0x15); PreLoadSet ('p', 0x15);
	PreLoadSet ('d', 0x16); PreLoadSet ('p', 0x16);
	PreLoadSet ('d', 0x17); PreLoadSet ('p', 0x17);
	PreLoadSet ('d', 0x18); PreLoadSet ('p', 0x18);
	PreLoadSet ('d', 0x19); PreLoadSet ('p', 0x19);
	PreLoadSet ('d', 0x1A); PreLoadSet ('p', 0x1A);
	PreLoadSet ('d', 0x1B); PreLoadSet ('p', 0x1B);
	PreLoadSet ('d', 0x1C); PreLoadSet ('p', 0x1C);
	PreLoadSet ('d', 0x1D); PreLoadSet ('p', 0x1D);
	PreLoadSet ('d', 0x1F); PreLoadSet ('p', 0x1F);
	PreLoadSet ('d', 0x20); PreLoadSet ('p', 0x20);
	PreLoadSet ('d', 0x21); PreLoadSet ('p', 0x21);
	PreLoadSet ('d', 0x24); PreLoadSet ('p', 0x24);
	PreLoadSet ('d', 0x27); PreLoadSet ('p', 0x27);
	PreLoadSet ('d', 0x2A); PreLoadSet ('p', 0x2A);
	PreLoadSet ('d', 0x2B); PreLoadSet ('p', 0x2B);
	PreLoadSet ('d', 0x2C); PreLoadSet ('p', 0x2C);
	PreLoadSet ('d', 0x34); PreLoadSet ('p', 0x34);
	PreLoadSet ('d', 0x40); PreLoadSet ('p', 0x40);
	PreLoadSet ('d', 0x41); PreLoadSet ('p', 0x41);
	PreLoadSet ('d', 0x44); PreLoadSet ('p', 0x44);
	PreLoadSet ('d', 0x47); PreLoadSet ('p', 0x47);
	PreLoadSet ('d', 0x4A); PreLoadSet ('p', 0x4A);
	PreLoadSet ('d', 0x4C); PreLoadSet ('p', 0x4C);
	PreLoadSet ('d', 0x50); PreLoadSet ('p', 0x50);
	PreLoadSet ('d', 0x60); PreLoadSet ('p', 0x60);
	PreLoadSet ('d', 0x6A); PreLoadSet ('p', 0x6A);
	PreLoadSet ('d', 0x8A); PreLoadSet ('p', 0x8A);
	PreLoadSet ('d', 0xAA); PreLoadSet ('p', 0xAA);
	PreLoadSet ('d', 0xE0); PreLoadSet ('p', 0xE0);
	PreLoadSet ('d', 0xE1); PreLoadSet ('p', 0xE1);
	PreLoad ("dungeon", "0x13_sprite.png", &imgspriteflamed);
	PreLoad ("palace", "0x13_sprite.png", &imgspriteflamep);

	/*** various ***/
	PreLoad (PNG_VARIOUS, "black.png", &imgblack);
	PreLoad (PNG_VARIOUS, "disabled.png", &imgdisabled);
	PreLoad (PNG_VARIOUS, "unknown.png", &imgunk[1]);
	PreLoad (PNG_VARIOUS, "sel_unknown.png", &imgunk[2]);
	PreLoad (PNG_VARIOUS, "sel_room_current.png", &imgsrc);
	PreLoad (PNG_VARIOUS, "sel_room_start.png", &imgsrs);
	PreLoad (PNG_VARIOUS, "sel_room_moving.png", &imgsrm);
	PreLoad (PNG_VARIOUS, "sel_room_cross.png", &imgsrp);
	PreLoad (PNG_VARIOUS, "sel_room_broken.png", &imgsrb);
	PreLoad (PNG_VARIOUS, "sel_event.png", &imgsele);
	PreLoad (PNG_VARIOUS, "event_unused.png", &imgeventu);
	PreLoad (PNG_VARIOUS, "sel_level.png", &imgsell);
	PreLoad (PNG_VARIOUS, "border_small_live.png", &imgbordersl);
	PreLoad (PNG_VARIOUS, "border_big_live.png", &imgborderbl);
	PreLoad (PNG_VARIOUS, "faded_l.png", &imgfadedl);
	PreLoad (PNG_VARIOUS, "popup_yn.png", &imgpopup_yn);
	PreLoad (PNG_VARIOUS, "help.png", &imghelp);
	PreLoad (PNG_VARIOUS, "exe.png", &imgexe);
	PreLoad (PNG_VARIOUS, "exe_warning.png", &imgexewarning);
	PreLoad (PNG_VARIOUS, "faded_s.png", &imgfadeds);
	PreLoad (PNG_VARIOUS, "event_tooltip.png", &imgetooltip);
	PreLoad (PNG_VARIOUS, "custom_hover.png", &imgchover);
	PreLoad (PNG_VARIOUS, "Mednafen.png", &imgmednafen);
	PreLoad (PNG_VARIOUS, "link_warn_lr.png", &imglinkwarnlr);
	PreLoad (PNG_VARIOUS, "link_warn_ud.png", &imglinkwarnud);
	PreLoad (PNG_VARIOUS, "sel_textline.png", &imgseltextline);
	PreLoad (PNG_VARIOUS, "various_warning.png", &imgvwarning);
	if (iController != 1)
	{
		PreLoad (PNG_VARIOUS, "border_big.png", &imgborderb);
		PreLoad (PNG_VARIOUS, "border_small.png", &imgborders);
		PreLoad (PNG_VARIOUS, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_VARIOUS, "dungeon.png", &imgdungeon);
		PreLoad (PNG_VARIOUS, "events.png", &imgevents);
		PreLoad (PNG_VARIOUS, "level_bar.png", &imgbar);
		PreLoad (PNG_VARIOUS, "palace.png", &imgpalace);
		PreLoad (PNG_VARIOUS, "popup.png", &imgpopup);
		PreLoad (PNG_VARIOUS, "room_links.png", &imgrl);
	} else {
		PreLoad (PNG_GAMEPAD, "border_big.png", &imgborderb);
		PreLoad (PNG_GAMEPAD, "border_small.png", &imgborders);
		PreLoad (PNG_GAMEPAD, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_GAMEPAD, "dungeon.png", &imgdungeon);
		PreLoad (PNG_GAMEPAD, "events.png", &imgevents);
		PreLoad (PNG_GAMEPAD, "level_bar.png", &imgbar);
		PreLoad (PNG_GAMEPAD, "palace.png", &imgpalace);
		PreLoad (PNG_GAMEPAD, "popup.png", &imgpopup);
		PreLoad (PNG_GAMEPAD, "room_links.png", &imgrl);
	}

	/*** (s)living ***/
	PreLoad (PNG_LIVING, "prince_l.png", &imgprincel[1]);
	PreLoad (PNG_SLIVING, "prince_l.png", &imgprincel[2]);
	PreLoad (PNG_LIVING, "prince_r.png", &imgprincer[1]);
	PreLoad (PNG_SLIVING, "prince_r.png", &imgprincer[2]);
	PreLoad (PNG_LIVING, "guard_l.png", &imgguardl[1]);
	PreLoad (PNG_SLIVING, "guard_l.png", &imgguardl[2]);
	PreLoad (PNG_LIVING, "guard_r.png", &imgguardr[1]);
	PreLoad (PNG_SLIVING, "guard_r.png", &imgguardr[2]);
	PreLoad (PNG_LIVING, "skeleton_l.png", &imgskeletonl[1]);
	PreLoad (PNG_SLIVING, "skeleton_l.png", &imgskeletonl[2]);
	PreLoad (PNG_LIVING, "skeleton_r.png", &imgskeletonr[1]);
	PreLoad (PNG_SLIVING, "skeleton_r.png", &imgskeletonr[2]);
	PreLoad (PNG_LIVING, "shadow_l.png", &imgshadowl[1]);
	PreLoad (PNG_SLIVING, "shadow_l.png", &imgshadowl[2]);
	PreLoad (PNG_LIVING, "shadow_r.png", &imgshadowr[1]);
	PreLoad (PNG_SLIVING, "shadow_r.png", &imgshadowr[2]);
	PreLoad (PNG_LIVING, "jaffar_l.png", &imgjaffarl[1]);
	PreLoad (PNG_SLIVING, "jaffar_l.png", &imgjaffarl[2]);
	PreLoad (PNG_LIVING, "jaffar_r.png", &imgjaffarr[1]);
	PreLoad (PNG_SLIVING, "jaffar_r.png", &imgjaffarr[2]);

	/*** buttons ***/
	PreLoad (PNG_BUTTONS, "up_0.png", &imgup_0);
	PreLoad (PNG_BUTTONS, "up_1.png", &imgup_1);
	PreLoad (PNG_BUTTONS, "down_0.png", &imgdown_0);
	PreLoad (PNG_BUTTONS, "down_1.png", &imgdown_1);
	PreLoad (PNG_BUTTONS, "left_0.png", &imgleft_0);
	PreLoad (PNG_BUTTONS, "left_1.png", &imgleft_1);
	PreLoad (PNG_BUTTONS, "right_0.png", &imgright_0);
	PreLoad (PNG_BUTTONS, "right_1.png", &imgright_1);
	PreLoad (PNG_BUTTONS, "up_down_no.png", &imgudno);
	PreLoad (PNG_BUTTONS, "left_right_no.png", &imglrno);
	if (iController != 1)
	{
		PreLoad (PNG_BUTTONS, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_BUTTONS, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_BUTTONS, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_BUTTONS, "events_off.png", &imgeventsoff);
		PreLoad (PNG_BUTTONS, "events_on_0.png", &imgeventson_0);
		PreLoad (PNG_BUTTONS, "events_on_1.png", &imgeventson_1);
		PreLoad (PNG_BUTTONS, "next_off.png", &imgnextoff);
		PreLoad (PNG_BUTTONS, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_BUTTONS, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_BUTTONS, "No.png", &imgno[1]);
		PreLoad (PNG_BUTTONS, "OK.png", &imgok[1]);
		PreLoad (PNG_BUTTONS, "previous_off.png", &imgprevoff);
		PreLoad (PNG_BUTTONS, "previous_on_0.png", &imgprevon_0);
		PreLoad (PNG_BUTTONS, "previous_on_1.png", &imgprevon_1);
		PreLoad (PNG_BUTTONS, "quit_0.png", &imgquit_0);
		PreLoad (PNG_BUTTONS, "quit_1.png", &imgquit_1);
		PreLoad (PNG_BUTTONS, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_BUTTONS, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_BUTTONS, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_BUTTONS, "save_off.png", &imgsaveoff);
		PreLoad (PNG_BUTTONS, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_BUTTONS, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_BUTTONS, "Save.png", &imgsave[1]);
		PreLoad (PNG_BUTTONS, "sel_No.png", &imgno[2]);
		PreLoad (PNG_BUTTONS, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_BUTTONS, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_BUTTONS, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_BUTTONS, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_BUTTONS, "Yes.png", &imgyes[1]);
	} else {
		PreLoad (PNG_GAMEPAD, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_GAMEPAD, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_GAMEPAD, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_GAMEPAD, "events_off.png", &imgeventsoff);
		PreLoad (PNG_GAMEPAD, "events_on_0.png", &imgeventson_0);
		PreLoad (PNG_GAMEPAD, "events_on_1.png", &imgeventson_1);
		PreLoad (PNG_GAMEPAD, "next_off.png", &imgnextoff);
		PreLoad (PNG_GAMEPAD, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_GAMEPAD, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_GAMEPAD, "No.png", &imgno[1]);
		PreLoad (PNG_GAMEPAD, "OK.png", &imgok[1]);
		PreLoad (PNG_GAMEPAD, "previous_off.png", &imgprevoff);
		PreLoad (PNG_GAMEPAD, "previous_on_0.png", &imgprevon_0);
		PreLoad (PNG_GAMEPAD, "previous_on_1.png", &imgprevon_1);
		PreLoad (PNG_GAMEPAD, "quit_0.png", &imgquit_0);
		PreLoad (PNG_GAMEPAD, "quit_1.png", &imgquit_1);
		PreLoad (PNG_GAMEPAD, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_GAMEPAD, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_GAMEPAD, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_GAMEPAD, "save_off.png", &imgsaveoff);
		PreLoad (PNG_GAMEPAD, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_GAMEPAD, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_GAMEPAD, "Save.png", &imgsave[1]);
		PreLoad (PNG_GAMEPAD, "sel_No.png", &imgno[2]);
		PreLoad (PNG_GAMEPAD, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_GAMEPAD, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_GAMEPAD, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_GAMEPAD, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_GAMEPAD, "Yes.png", &imgyes[1]);
	}

	/*** extras ***/
	PreLoad (PNG_EXTRAS, "extras_00.png", &imgextras[0]);
	PreLoad (PNG_EXTRAS, "extras_01.png", &imgextras[1]);
	PreLoad (PNG_EXTRAS, "extras_02.png", &imgextras[2]);
	PreLoad (PNG_EXTRAS, "extras_03.png", &imgextras[3]);
	PreLoad (PNG_EXTRAS, "extras_04.png", &imgextras[4]);
	PreLoad (PNG_EXTRAS, "extras_05.png", &imgextras[5]);
	PreLoad (PNG_EXTRAS, "extras_06.png", &imgextras[6]);
	PreLoad (PNG_EXTRAS, "extras_07.png", &imgextras[7]);
	PreLoad (PNG_EXTRAS, "extras_08.png", &imgextras[8]);
	PreLoad (PNG_EXTRAS, "extras_09.png", &imgextras[9]);
	PreLoad (PNG_EXTRAS, "extras_10.png", &imgextras[10]);

	/*** rooms ***/
	PreLoad (PNG_ROOMS, "room1.png", &imgroom[1]);
	PreLoad (PNG_ROOMS, "room2.png", &imgroom[2]);
	PreLoad (PNG_ROOMS, "room3.png", &imgroom[3]);
	PreLoad (PNG_ROOMS, "room4.png", &imgroom[4]);
	PreLoad (PNG_ROOMS, "room5.png", &imgroom[5]);
	PreLoad (PNG_ROOMS, "room6.png", &imgroom[6]);
	PreLoad (PNG_ROOMS, "room7.png", &imgroom[7]);
	PreLoad (PNG_ROOMS, "room8.png", &imgroom[8]);
	PreLoad (PNG_ROOMS, "room9.png", &imgroom[9]);
	PreLoad (PNG_ROOMS, "room10.png", &imgroom[10]);
	PreLoad (PNG_ROOMS, "room11.png", &imgroom[11]);
	PreLoad (PNG_ROOMS, "room12.png", &imgroom[12]);
	PreLoad (PNG_ROOMS, "room13.png", &imgroom[13]);
	PreLoad (PNG_ROOMS, "room14.png", &imgroom[14]);
	PreLoad (PNG_ROOMS, "room15.png", &imgroom[15]);
	PreLoad (PNG_ROOMS, "room16.png", &imgroom[16]);
	PreLoad (PNG_ROOMS, "room17.png", &imgroom[17]);
	PreLoad (PNG_ROOMS, "room18.png", &imgroom[18]);
	PreLoad (PNG_ROOMS, "room19.png", &imgroom[19]);
	PreLoad (PNG_ROOMS, "room20.png", &imgroom[20]);
	PreLoad (PNG_ROOMS, "room21.png", &imgroom[21]);
	PreLoad (PNG_ROOMS, "room22.png", &imgroom[22]);
	PreLoad (PNG_ROOMS, "room23.png", &imgroom[23]);
	PreLoad (PNG_ROOMS, "room24.png", &imgroom[24]);
	PreLoad (PNG_ROOMS, "room25.png", &imgroom[25]); /*** "?"; high links ***/

	/*** alphabet ***/
	for (iAlphabetLoop = 1; iAlphabetLoop <= 26; iAlphabetLoop++)
	{
		snprintf (sImage, MAX_IMG, "%c.png", iAlphabetLoop + 0x40);
		PreLoad (PNG_ALPHABET, sImage, &imgalphabet[iAlphabetLoop]);
	}
	PreLoad (PNG_ALPHABET, "space.png", &imgspace);
	PreLoad (PNG_ALPHABET, "unknown.png", &imgunknown);

	if (iDebug == 1)
		{ printf ("[ INFO ] Preloaded images: %i\n", iPreLoaded); }
	SDL_SetCursor (curArrow);

	/*** Defaults. ***/
	iCurLevel = iStartLevel;
	iCurRoom = arStartLocation[iCurLevel][1];
	iDownAt = 0;
	iSelected = 1; /*** Start with the upper left selected. ***/
	iScreen = 1;
	iChangeEvent = 1;
	iFlameFrame = 1;
	oldticks = 0;

	iTTP1 = TTPD_1;
	iTTPO = TTPD_O;
	iDX = DD_X;
	iDY = DD_Y;
	iHor[0] = (iDX * -1) + OFFSETD_X;
	iHor[1] = (iDX * 0) + OFFSETD_X;
	iHor[2] = (iDX * 1) + OFFSETD_X;
	iHor[3] = (iDX * 2) + OFFSETD_X;
	iHor[4] = (iDX * 3) + OFFSETD_X;
	iHor[5] = (iDX * 4) + OFFSETD_X;
	iHor[6] = (iDX * 5) + OFFSETD_X;
	iHor[7] = (iDX * 6) + OFFSETD_X;
	iHor[8] = (iDX * 7) + OFFSETD_X;
	iHor[9] = (iDX * 8) + OFFSETD_X;
	iHor[10] = (iDX * 9) + OFFSETD_X;
	iVer0 = OFFSETD_Y - iTTP1 - (iDY * 1);
	iVer1 = OFFSETD_Y - iTTP1 + (iDY * 0);
	iVer2 = OFFSETD_Y - iTTP1 + (iDY * 1);
	iVer3 = OFFSETD_Y - iTTP1 + (iDY * 2);
	iVer4 = OFFSETD_Y - iTTP1 + (iDY * 3);

	ShowScreen();
	InitPopUp();
	while (1)
	{
		if (iNoAnim == 0)
		{
			/* This is for the animation; 20 fps (1000/50). The GBC runs at
			 * about 60 fps, but changes the torch flames every 3 frames.
			 */
			newticks = SDL_GetTicks();
			if (newticks > oldticks + 50)
			{
				iFlameFrame++;
				if (iFlameFrame == 5) { iFlameFrame = 1; }
				ShowScreen();
				oldticks = newticks;
			}
		}

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							InitScreenAction ("enter");
							break;
						case SDL_CONTROLLER_BUTTON_B:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDL_CONTROLLER_BUTTON_X:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (arBrokenRoomLinks[iCurLevel] == 0) {
								arBrokenRoomLinks[iCurLevel] = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDL_CONTROLLER_BUTTON_Y:
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								if (iChangeEvent > arNrEvents[iCurLevel])
									{ iChangeEvent = arNrEvents[iCurLevel]; }
								PlaySound ("wav/screen2or3.wav");
							} else {
								switch (arEventsOpenClose[iCurLevel][iChangeEvent])
								{
									case 0x00:
										arEventsOpenClose[iCurLevel][iChangeEvent] = 0x01;
										break;
									case 0x01:
										arEventsOpenClose[iCurLevel][iChangeEvent] = 0x00;
										break;
								}
								PlaySound ("wav/check_box.wav");
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_BACK:
							if ((iScreen == 2) && (arBrokenRoomLinks[iCurLevel] == 1))
							{
								LinkMinus();
							}
							if (iScreen == 3)
							{
								iEventRoom = arEventsFromRoom[iCurLevel][iChangeEvent];
								if ((iEventRoom >= 1) && (iEventRoom <= 23))
								{
									EventRoom (iEventRoom + 1, 0);
								} else {
									EventRoom (1, 0);
								}
							}
							break;
						case SDL_CONTROLLER_BUTTON_GUIDE:
							if (iChanged != 0) { CallSave(); } break;
						case SDL_CONTROLLER_BUTTON_START:
							RunLevel (iCurLevel);
							break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
							if (iCurLevel != 1)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Prev();
							}
							break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
							if (iCurLevel != LEVELS)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Next();
							}
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							InitScreenAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							InitScreenAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							InitScreenAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							InitScreenAction ("down"); break;
					}
					ShowScreen();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iYJoy1 = SDL_JoystickGetAxis (joystick, 1);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					iYJoy2 = SDL_JoystickGetAxis (joystick, 4);
					if ((iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((SDL_GetTicks() - joyleft) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("left from");
							}
							joyleft = SDL_GetTicks();
						}
					}
					if ((iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((SDL_GetTicks() - joyright) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("right from");
							}
							joyright = SDL_GetTicks();
						}
					}
					if ((iYJoy1 < -30000) || (iYJoy2 < -30000)) /*** up ***/
					{
						if ((SDL_GetTicks() - joyup) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("up from");
							}
							joyup = SDL_GetTicks();
						}
					}
					if ((iYJoy1 > 30000) || (iYJoy2 > 30000)) /*** down ***/
					{
						if ((SDL_GetTicks() - joydown) > 300)
						{
							if (iScreen == 1)
							{
								if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
									PlaySound ("wav/scroll.wav");
								}
							}
							if (iScreen == 3)
							{
								InitScreenAction ("down from");
							}
							joydown = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
					{
						if ((SDL_GetTicks() - trigleft) > 300)
						{
							if (iScreen == 2)
							{
								if (arBrokenRoomLinks[iCurLevel] == 0)
								{
									iMovingNewBusy = 0;
									switch (iMovingRoom)
									{
										case 0: iMovingRoom = ROOMS; break; /*** If disabled. ***/
										case 1: iMovingRoom = ROOMS; break;
										default: iMovingRoom--; break;
									}
								}
							}
							if (iScreen == 3)
							{
								if (arNrEvents[iCurLevel] < 20) /*** Randomly picked max. ***/
								{
									TotalEvents (1);
								} else {
									arNrEvents[iCurLevel] = 0;
									PlaySound ("wav/plus_minus.wav");
									iChanged++;
								}
							}
							trigleft = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
					{
						if ((SDL_GetTicks() - trigright) > 300)
						{
							if (iScreen == 2)
							{
								if (arBrokenRoomLinks[iCurLevel] == 0)
								{
									iMovingNewBusy = 0;
									switch (iMovingRoom)
									{
										case 0: iMovingRoom = 1; break; /*** If disabled. ***/
										case 24: iMovingRoom = 1; break;
										default: iMovingRoom++; break;
									}
								}
							}
							if (iScreen == 3)
							{
								if (iChangeEvent < arNrEvents[iCurLevel])
								{
									ChangeEvent (1, 0);
								} else {
									iChangeEvent = 1;
									PlaySound ("wav/plus_minus.wav");
								}
							}
							trigright = SDL_GetTicks();
						}
					}
					ShowScreen();
					break;
				case SDL_KEYDOWN: /*** https://wiki.libsdl.org/SDL2/SDL_Keycode ***/
					switch (event.key.keysym.sym)
					{
						case SDLK_F1:
							if (iScreen == 1)
							{
								Help(); SDL_SetCursor (curArrow);
							}
							break;
						case SDLK_F2:
							if (iScreen == 1)
							{
								EXE();
								SDL_SetCursor (curArrow);
								SDL_StopTextInput();
							}
							break;
						case SDLK_LEFTBRACKET:
							if (event.key.keysym.mod & KMOD_SHIFT)
							{
								InitScreenAction ("left brace"); /*** { ***/
							} else {
								InitScreenAction ("left bracket"); /*** [ ***/
							}
							break;
						case SDLK_RIGHTBRACKET:
							if (event.key.keysym.mod & KMOD_SHIFT)
							{
								InitScreenAction ("right brace"); /*** } ***/
							} else {
								InitScreenAction ("right bracket"); /*** ] ***/
							}
							break;
						case SDLK_d:
							RunLevel (iCurLevel);
							break;
						case SDLK_SLASH:
							if (iScreen == 1) { ClearRoom(); }
							break;
						case SDLK_BACKSLASH:
							if (iScreen == 1)
							{
								/*** Randomize the entire level. ***/
								for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
								{
									for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
									{
										UseTile (-1, iTileLoop, iRoomLoop);
									}
								}
								iChanged++;
								PlaySound ("wav/ok_close.wav");
							}
							break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (((event.key.keysym.mod & KMOD_LALT) ||
								(event.key.keysym.mod & KMOD_RALT)) && (iScreen == 1))
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							} else {
								InitScreenAction ("enter");
							}
							break;
						case SDLK_BACKSPACE:
							if ((iScreen == 2) && (arBrokenRoomLinks[iCurLevel] == 1))
							{
								LinkMinus();
							}
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDLK_LEFT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
										{
											iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										ChangeEvent (-1, 0);
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									ChangeEvent (-10, 0);
								}
							} else {
								InitScreenAction ("left");
							}
							break;
						case SDLK_RIGHT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
										{
											iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										ChangeEvent (1, 0);
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									ChangeEvent (10, 0);
								}
							} else {
								InitScreenAction ("right");
							}
							break;
						case SDLK_UP:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
									{
										iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("up");
							}
							break;
						case SDLK_DOWN:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
									{
										iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("down");
							}
							break;
						case SDLK_t:
							/*** Will change the environment. ***/
							break;
						case SDLK_MINUS:
						case SDLK_KP_MINUS:
							if (iCurLevel != 1)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Prev();
							}
							break;
						case SDLK_KP_PLUS:
						case SDLK_EQUALS:
							if (iCurLevel != LEVELS)
							{
								if (iChanged != 0) { InitPopUpSave(); }
								Next();
							}
							break;
						case SDLK_r:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (arBrokenRoomLinks[iCurLevel] == 0) {
								arBrokenRoomLinks[iCurLevel] = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_e:
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								if (iChangeEvent > arNrEvents[iCurLevel])
									{ iChangeEvent = arNrEvents[iCurLevel]; }
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_s:
							if (iChanged != 0) { CallSave(); } break;
						case SDLK_z:
							if (iScreen == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_f:
							if (iScreen == 1)
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_QUOTE:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LSHIFT) ||
									(event.key.keysym.mod & KMOD_RSHIFT))
								{
									Sprinkle();
									PlaySound ("wav/extras.wav");
									iChanged++;
								} else {
									SetLocation (iCurRoom, iSelected, iLastTile);
									PlaySound ("wav/ok_close.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_h:
							if (iScreen == 1)
							{
								FlipRoom (1);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}
							if (iScreen == 3)
							{
								if (event.key.keysym.mod & KMOD_LCTRL)
								{
									TotalEvents (-10);
								} else if (event.key.keysym.mod & KMOD_SHIFT) {
									TotalEvents (-1);
								} else {
									InitScreenAction ("left from");
								}
							}
							break;
						case SDLK_j:
							if (iScreen == 3)
							{
								if (event.key.keysym.mod & KMOD_LCTRL)
								{
									TotalEvents (10);
								} else if (event.key.keysym.mod & KMOD_SHIFT) {
									TotalEvents (1);
								} else {
									InitScreenAction ("right from");
								}
							}
							break;
						case SDLK_u:
							if (iScreen == 3)
							{
								InitScreenAction ("up from");
							}
							break;
						case SDLK_n:
							if (iScreen == 3)
							{
								InitScreenAction ("down from");
							}
							break;
						case SDLK_v:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (2);
									PlaySound ("wav/extras.wav");
									iChanged++;
								} else {
									FlipRoom (2);
									PlaySound ("wav/extras.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_c:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (1);
									PlaySound ("wav/extras.wav");
								}
							}
							if (iScreen == 3)
							{
								if (arEventsOpenClose[iCurLevel][iChangeEvent] != 0x00)
								{
									arEventsOpenClose[iCurLevel][iChangeEvent] = 0x00;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_o:
							if (iScreen == 3)
							{
								if (arEventsOpenClose[iCurLevel][iChangeEvent] != 0x01)
								{
									arEventsOpenClose[iCurLevel][iChangeEvent] = 0x01;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_i:
							if (iScreen == 1)
							{
								if (iInfo == 0) { iInfo = 1; } else { iInfo = 0; }
							}
							break;
						case SDLK_0: /*** empty ***/
						case SDLK_KP_0:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x00);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_1: /*** floor ***/
						case SDLK_KP_1:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x01);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_2: /*** loose tile ***/
						case SDLK_KP_2:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x0B);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_3: /*** closed gate ***/
						case SDLK_KP_3:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x44);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_4: /*** open gate ***/
						case SDLK_KP_4:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x24);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_5: /*** torch ***/
						case SDLK_KP_5:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x13);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_6: /*** spikes ***/
						case SDLK_KP_6:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x02);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_7: /*** small pillar ***/
						case SDLK_KP_7:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x03);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_8: /*** chomper ***/
						case SDLK_KP_8:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x12);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_9: /*** wall ***/
						case SDLK_KP_9:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelected, 0x14);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						default: break;
					}
					ShowScreen();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					/*** Mednafen information. ***/
					if (OnLevelBar() == 1)
					{
						if (iMednafen != 1) { iMednafen = 1; ShowScreen(); }
					} else {
						if (iMednafen != 0) { iMednafen = 0; ShowScreen(); }
					}

					if (iScreen == 2)
					{
						if (iMovingRoom != 0) { ShowScreen(); }
					}

					if (iScreen == 1)
					{
						/*** User hovers over tiles in the upper row. ***/
						if ((InArea (iHor[1], iVer1 + iTTP1, iHor[2], iVer2 + iTTPO)
							== 1) && (iSelected != 1))
							{ iSelected = 1; ShowScreen(); }
						else if ((InArea (iHor[2], iVer1 + iTTP1, iHor[3], iVer2 + iTTPO)
							== 1) && (iSelected != 2))
							{ iSelected = 2; ShowScreen(); }
						else if ((InArea (iHor[3], iVer1 + iTTP1, iHor[4], iVer2 + iTTPO)
							== 1) && (iSelected != 3))
							{ iSelected = 3; ShowScreen(); }
						else if ((InArea (iHor[4], iVer1 + iTTP1, iHor[5], iVer2 + iTTPO)
							== 1) && (iSelected != 4))
							{ iSelected = 4; ShowScreen(); }
						else if ((InArea (iHor[5], iVer1 + iTTP1, iHor[6], iVer2 + iTTPO)
							== 1) && (iSelected != 5))
							{ iSelected = 5; ShowScreen(); }
						else if ((InArea (iHor[6], iVer1 + iTTP1, iHor[7], iVer2 + iTTPO)
							== 1) && (iSelected != 6))
							{ iSelected = 6; ShowScreen(); }
						else if ((InArea (iHor[7], iVer1 + iTTP1, iHor[8], iVer2 + iTTPO)
							== 1) && (iSelected != 7))
							{ iSelected = 7; ShowScreen(); }
						else if ((InArea (iHor[8], iVer1 + iTTP1, iHor[9], iVer2 + iTTPO)
							== 1) && (iSelected != 8))
							{ iSelected = 8; ShowScreen(); }
						else if ((InArea (iHor[9], iVer1 + iTTP1, iHor[10], iVer2 + iTTPO)
							== 1) && (iSelected != 9))
							{ iSelected = 9; ShowScreen(); }
						else if ((InArea (iHor[10], iVer1 + iTTP1, iHor[10] + iDX,
							iVer2 + iTTPO) == 1) && (iSelected != 10))
						{ iSelected = 10; ShowScreen(); }

						/*** User hovers over tiles in the middle row. ***/
						else if ((InArea (iHor[1], iVer2 + iTTPO, iHor[2], iVer3 + iTTPO)
							== 1) && (iSelected != 11))
							{ iSelected = 11; ShowScreen(); }
						else if ((InArea (iHor[2], iVer2 + iTTPO, iHor[3], iVer3 + iTTPO)
							== 1) && (iSelected != 12))
							{ iSelected = 12; ShowScreen(); }
						else if ((InArea (iHor[3], iVer2 + iTTPO, iHor[4], iVer3 + iTTPO)
							== 1) && (iSelected != 13))
							{ iSelected = 13; ShowScreen(); }
						else if ((InArea (iHor[4], iVer2 + iTTPO, iHor[5], iVer3 + iTTPO)
							== 1) && (iSelected != 14))
							{ iSelected = 14; ShowScreen(); }
						else if ((InArea (iHor[5], iVer2 + iTTPO, iHor[6], iVer3 + iTTPO)
							== 1) && (iSelected != 15))
							{ iSelected = 15; ShowScreen(); }
						else if ((InArea (iHor[6], iVer2 + iTTPO, iHor[7], iVer3 + iTTPO)
							== 1) && (iSelected != 16))
							{ iSelected = 16; ShowScreen(); }
						else if ((InArea (iHor[7], iVer2 + iTTPO, iHor[8], iVer3 + iTTPO)
							== 1) && (iSelected != 17))
							{ iSelected = 17; ShowScreen(); }
						else if ((InArea (iHor[8], iVer2 + iTTPO, iHor[9], iVer3 + iTTPO)
							== 1) && (iSelected != 18))
							{ iSelected = 18; ShowScreen(); }
						else if ((InArea (iHor[9], iVer2 + iTTPO, iHor[10], iVer3 + iTTPO)
							== 1) && (iSelected != 19))
							{ iSelected = 19; ShowScreen(); }
						else if ((InArea (iHor[10], iVer2 + iTTPO, iHor[10] + iDX,
							iVer3 + iTTPO) == 1) && (iSelected != 20))
						{ iSelected = 20; ShowScreen(); }

						/*** User hovers over tiles in the bottom row. ***/
						else if ((InArea (iHor[1], iVer3 + iTTPO, iHor[2],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 21))
							{ iSelected = 21; ShowScreen(); }
						else if ((InArea (iHor[2], iVer3 + iTTPO, iHor[3],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 22))
							{ iSelected = 22; ShowScreen(); }
						else if ((InArea (iHor[3], iVer3 + iTTPO, iHor[4],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 23))
							{ iSelected = 23; ShowScreen(); }
						else if ((InArea (iHor[4], iVer3 + iTTPO, iHor[5],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 24))
							{ iSelected = 24; ShowScreen(); }
						else if ((InArea (iHor[5], iVer3 + iTTPO, iHor[6],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 25))
							{ iSelected = 25; ShowScreen(); }
						else if ((InArea (iHor[6], iVer3 + iTTPO, iHor[7],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 26))
							{ iSelected = 26; ShowScreen(); }
						else if ((InArea (iHor[7], iVer3 + iTTPO, iHor[8],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 27))
							{ iSelected = 27; ShowScreen(); }
						else if ((InArea (iHor[8], iVer3 + iTTPO, iHor[9],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 28))
							{ iSelected = 28; ShowScreen(); }
						else if ((InArea (iHor[9], iVer3 + iTTPO, iHor[10],
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 29))
							{ iSelected = 29; ShowScreen(); }
						else if ((InArea (iHor[10], iVer3 + iTTPO, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) && (iSelected != 30))
						{ iSelected = 30; ShowScreen(); }

						/*** extras ***/
						if ((InArea (610, 3, 619, 12) == 1) && (iExtras != 1))
							{ iExtras = 1; ShowScreen(); }
						else if ((InArea (620, 3, 629, 12) == 1) && (iExtras != 2))
							{ iExtras = 2; ShowScreen(); }
						else if ((InArea (630, 3, 639, 12) == 1) && (iExtras != 3))
							{ iExtras = 3; ShowScreen(); }
						else if ((InArea (640, 3, 649, 12) == 1) && (iExtras != 4))
							{ iExtras = 4; ShowScreen(); }
						else if ((InArea (650, 3, 659, 12) == 1) && (iExtras != 5))
							{ iExtras = 5; ShowScreen(); }
						else if ((InArea (610, 13, 619, 22) == 1) && (iExtras != 6))
							{ iExtras = 6; ShowScreen(); }
						else if ((InArea (620, 13, 629, 22) == 1) && (iExtras != 7))
							{ iExtras = 7; ShowScreen(); }
						else if ((InArea (630, 13, 639, 22) == 1) && (iExtras != 8))
							{ iExtras = 8; ShowScreen(); }
						else if ((InArea (640, 13, 649, 22) == 1) && (iExtras != 9))
							{ iExtras = 9; ShowScreen(); }
						else if ((InArea (650, 13, 659, 22) == 1) && (iExtras != 10))
							{ iExtras = 10; ShowScreen(); }
						else if ((InArea (610, 3, 659, 22) == 0) && (iExtras != 0))
							{ iExtras = 0; ShowScreen(); }
					}

					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (0, 50, 25, 548) == 1) /*** left arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][1] != 0) { iDownAt = 1; }
						}
						if (InArea (667, 50, 692, 548) == 1) /*** right arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][2] != 0) { iDownAt = 2; }
						}
						if (InArea (25, 25, 667, 50) == 1) /*** up arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][3] != 0) { iDownAt = 3; }
						}
						if (InArea (25, 548, 667, 573) == 1) /*** down arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][4] != 0) { iDownAt = 4; }
						}
						if (InArea (0, 25, 25, 50) == 1) /*** rooms ***/
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								iDownAt = 5;
							} else {
								iDownAt = 11;
							}
						}
						if (InArea (667, 25, 692, 50) == 1) /*** events ***/
						{
							iDownAt = 6;
						}
						if (InArea (0, 548, 25, 573) == 1) /*** save ***/
						{
							iDownAt = 7;
						}
						if (InArea (667, 548, 692, 573) == 1) /*** quit ***/
						{
							iDownAt = 8;
						}
						if (InArea (0, 0, 25, 25) == 1) /*** previous ***/
						{
							iDownAt = 9;
						}
						if (InArea (667, 0, 692, 25) == 1) /*** next ***/
						{
							iDownAt = 10;
						}

						if (iScreen == 2)
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								for (iRoomLoop = 0; iRoomLoop < ROOMS; iRoomLoop++) /*** x ***/
								{
									/*** y ***/
									for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
									{
										if (InArea (297 + (iRoomLoop * 15), 61 + (iRoomLoop2 * 15),
											310 + (iRoomLoop * 15), 74 + (iRoomLoop2 * 15)) == 1)
										{
											if (arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1] != 0)
											{
												iMovingNewBusy = 0;
												iMovingRoom =
													arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1];
											}
										}
									}
								}
								/*** y ***/
								for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
								{
									if (InArea (272, 61 + (iRoomLoop2 * 15),
										272 + 14, 61 + 14 + (iRoomLoop2 * 15)) == 1)
									{
										if (arMovingRooms[25][iRoomLoop2 + 1] != 0)
										{
											iMovingNewBusy = 0;
											iMovingRoom = arMovingRooms[25][iRoomLoop2 + 1];
										}
									}
								}

								/*** rooms broken ***/
								if (InArea (629, 63, 654, 88) == 1)
								{
									iDownAt = 11;
								}
							} else {
								MouseSelectAdj();
							}
						}
					}
					ShowScreen();
					break;
				case SDL_MOUSEBUTTONUP:
					iDownAt = 0;
					if (event.button.button == 1) /*** left mouse button, change ***/
					{
						if (InArea (0, 50, 25, 548) == 1) /*** left arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (667, 50, 692, 548) == 1) /*** right arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 25, 667, 50) == 1) /*** up arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 548, 667, 573) == 1) /*** down arrow ***/
						{
							if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
							{
								iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (0, 25, 25, 50) == 1) /*** rooms ***/
						{
							if (iScreen != 2)
							{
								iScreen = 2; iMovingRoom = 0; iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (667, 25, 692, 50) == 1) /*** events ***/
						{
							if (iScreen == 2)
							{
								arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								if (iChangeEvent > arNrEvents[iCurLevel])
									{ iChangeEvent = arNrEvents[iCurLevel]; }
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (0, 548, 25, 573) == 1) /*** save ***/
						{
							if (iChanged != 0) { CallSave(); }
						}
						if (InArea (667, 548, 692, 573) == 1) /*** quit ***/
						{
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
						}
						if (InArea (0, 0, 25, 25) == 1) /*** previous ***/
						{
							if (iChanged != 0) { InitPopUpSave(); }
							Prev();
							ShowScreen(); break;
						}
						if (InArea (667, 0, 692, 25) == 1) /*** next ***/
						{
							if (iChanged != 0) { InitPopUpSave(); }
							Next();
							ShowScreen(); break;
						}
						if (OnLevelBar() == 1) /*** level bar ***/
						{
							RunLevel (iCurLevel);
						}

						if (iScreen == 1)
						{
							if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
								iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
							{
								keystate = SDL_GetKeyboardState (NULL);
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
								{
									SetLocation (iCurRoom, iSelected, iLastTile);
									PlaySound ("wav/ok_close.wav"); iChanged++;
								} else {
									ChangePos();
									ShowScreen(); break; /*** ? ***/
								}
							}

							/*** 1 ***/
							if (InArea (610, 3, 619, 12) == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}

							/*** 4 ***/
							if (InArea (640, 3, 649, 12) == 1)
							{
								InitScreenAction ("env");
							}

							/*** 6 ***/
							if (InArea (610, 13, 619, 22) == 1)
							{
								Sprinkle();
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 8 ***/
							if (InArea (630, 13, 639, 22) == 1)
							{
								FlipRoom (1);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 3 ***/
							if (InArea (630, 3, 639, 12) == 1)
							{
								FlipRoom (2);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 2 ***/
							if (InArea (620, 3, 629, 12) == 1)
							{
								CopyPaste (1);
								PlaySound ("wav/extras.wav");
							}

							/*** 7 ***/
							if (InArea (620, 13, 629, 22) == 1)
							{
								CopyPaste (2);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 5 ***/
							if (InArea (650, 3, 659, 12) == 1)
							{
								Help(); SDL_SetCursor (curArrow);
							}

							/*** 10 ***/
							if (InArea (650, 13, 659, 22) == 1)
							{
								EXE();
								SDL_SetCursor (curArrow);
								SDL_StopTextInput();
							}
						}

						if (iScreen == 2) /*** room links screen ***/
						{
							if (arBrokenRoomLinks[iCurLevel] == 0)
							{
								for (iRoomLoop = 0; iRoomLoop < ROOMS; iRoomLoop++) /*** x ***/
								{
									/*** y ***/
									for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
									{
										if (InArea (297 + (iRoomLoop * 15), 61 + (iRoomLoop2 * 15),
											310 + (iRoomLoop * 15), 74 + (iRoomLoop2 * 15)) == 1)
										{
											if (iMovingRoom != 0)
											{
												if (arMovingRooms[iRoomLoop + 1][iRoomLoop2 + 1] == 0)
												{
													RemoveOldRoom();
													AddNewRoom (iRoomLoop + 1,
														iRoomLoop2 + 1, iMovingRoom);
													iChanged++;
												}
												iMovingRoom = 0; iMovingNewBusy = 0;
											}
										}
									}
								}
								/*** y ***/
								for (iRoomLoop2 = 0; iRoomLoop2 < ROOMS; iRoomLoop2++)
								{
									if (InArea (272, 61 + (iRoomLoop2 * 15),
										272 + 14, 61 + 14 + (iRoomLoop2 * 15)) == 1)
									{
										if (iMovingRoom != 0)
										{
											if (arMovingRooms[25][iRoomLoop2 + 1] == 0)
											{
												RemoveOldRoom();
												AddNewRoom (25, iRoomLoop2 + 1, iMovingRoom);
												iChanged++;
											}
											iMovingRoom = 0; iMovingNewBusy = 0;
										}
									}
								}

								/*** rooms broken ***/
								if (InArea (629, 63, 654, 88) == 1)
								{
									arBrokenRoomLinks[iCurLevel] = 1;
									PlaySound ("wav/screen2or3.wav");
								}

								/*** Horizontal level. ***/
								if (InArea (275, 434, 275 + 49, 434 + 49) == 1)
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										/*** Clear. ***/
										arRoomLinks[iCurLevel][iRoomLoop][1] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][2] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][3] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][4] = 0;

										if (iRoomLoop != 1)
											{ arRoomLinks[iCurLevel][iRoomLoop][1] = iRoomLoop - 1; }
										if (iRoomLoop != 24)
											{ arRoomLinks[iCurLevel][iRoomLoop][2] = iRoomLoop + 1; }
									}
									PlaySound ("wav/move_room.wav");
									iChanged++;
								}

								/*** Vertical level. ***/
								if (InArea (275, 486, 275 + 49, 486 + 49) == 1)
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										/*** Clear. ***/
										arRoomLinks[iCurLevel][iRoomLoop][1] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][2] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][3] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][4] = 0;

										if (iRoomLoop != 1)
											{ arRoomLinks[iCurLevel][iRoomLoop][3] = iRoomLoop - 1; }
										if (iRoomLoop != 24)
											{ arRoomLinks[iCurLevel][iRoomLoop][4] = iRoomLoop + 1; }
									}
									PlaySound ("wav/move_room.wav");
									iChanged++;
								}

								/*** Horizontal rectangle level. ***/
								if (InArea (327, 434, 327 + 49, 434 + 49) == 1)
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										/*** Clear. ***/
										arRoomLinks[iCurLevel][iRoomLoop][1] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][2] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][3] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][4] = 0;
									}
									arRoomLinks[iCurLevel][1][2] = 2;
									arRoomLinks[iCurLevel][1][4] = 7;
									arRoomLinks[iCurLevel][2][1] = 1;
									arRoomLinks[iCurLevel][2][2] = 3;
									arRoomLinks[iCurLevel][2][4] = 8;
									arRoomLinks[iCurLevel][3][1] = 2;
									arRoomLinks[iCurLevel][3][2] = 4;
									arRoomLinks[iCurLevel][3][4] = 9;
									arRoomLinks[iCurLevel][4][1] = 3;
									arRoomLinks[iCurLevel][4][2] = 5;
									arRoomLinks[iCurLevel][4][4] = 10;
									arRoomLinks[iCurLevel][5][1] = 4;
									arRoomLinks[iCurLevel][5][2] = 6;
									arRoomLinks[iCurLevel][5][4] = 11;
									arRoomLinks[iCurLevel][6][1] = 5;
									arRoomLinks[iCurLevel][6][4] = 12;
									arRoomLinks[iCurLevel][7][2] = 8;
									arRoomLinks[iCurLevel][7][3] = 1;
									arRoomLinks[iCurLevel][7][4] = 13;
									arRoomLinks[iCurLevel][8][1] = 7;
									arRoomLinks[iCurLevel][8][2] = 9;
									arRoomLinks[iCurLevel][8][3] = 2;
									arRoomLinks[iCurLevel][8][4] = 14;
									arRoomLinks[iCurLevel][9][1] = 8;
									arRoomLinks[iCurLevel][9][2] = 10;
									arRoomLinks[iCurLevel][9][3] = 3;
									arRoomLinks[iCurLevel][9][4] = 15;
									arRoomLinks[iCurLevel][10][1] = 9;
									arRoomLinks[iCurLevel][10][2] = 11;
									arRoomLinks[iCurLevel][10][3] = 4;
									arRoomLinks[iCurLevel][10][4] = 16;
									arRoomLinks[iCurLevel][11][1] = 10;
									arRoomLinks[iCurLevel][11][2] = 12;
									arRoomLinks[iCurLevel][11][3] = 5;
									arRoomLinks[iCurLevel][11][4] = 17;
									arRoomLinks[iCurLevel][12][1] = 11;
									arRoomLinks[iCurLevel][12][3] = 6;
									arRoomLinks[iCurLevel][12][4] = 18;
									arRoomLinks[iCurLevel][13][2] = 14;
									arRoomLinks[iCurLevel][13][3] = 7;
									arRoomLinks[iCurLevel][13][4] = 19;
									arRoomLinks[iCurLevel][14][1] = 13;
									arRoomLinks[iCurLevel][14][2] = 15;
									arRoomLinks[iCurLevel][14][3] = 8;
									arRoomLinks[iCurLevel][14][4] = 20;
									arRoomLinks[iCurLevel][15][1] = 14;
									arRoomLinks[iCurLevel][15][2] = 16;
									arRoomLinks[iCurLevel][15][3] = 9;
									arRoomLinks[iCurLevel][15][4] = 21;
									arRoomLinks[iCurLevel][16][1] = 15;
									arRoomLinks[iCurLevel][16][2] = 17;
									arRoomLinks[iCurLevel][16][3] = 10;
									arRoomLinks[iCurLevel][16][4] = 22;
									arRoomLinks[iCurLevel][17][1] = 16;
									arRoomLinks[iCurLevel][17][2] = 18;
									arRoomLinks[iCurLevel][17][3] = 11;
									arRoomLinks[iCurLevel][17][4] = 23;
									arRoomLinks[iCurLevel][18][1] = 17;
									arRoomLinks[iCurLevel][18][3] = 12;
									arRoomLinks[iCurLevel][18][4] = 24;
									arRoomLinks[iCurLevel][19][2] = 20;
									arRoomLinks[iCurLevel][19][3] = 13;
									arRoomLinks[iCurLevel][20][1] = 19;
									arRoomLinks[iCurLevel][20][2] = 21;
									arRoomLinks[iCurLevel][20][3] = 14;
									arRoomLinks[iCurLevel][21][1] = 20;
									arRoomLinks[iCurLevel][21][2] = 22;
									arRoomLinks[iCurLevel][21][3] = 15;
									arRoomLinks[iCurLevel][22][1] = 21;
									arRoomLinks[iCurLevel][22][2] = 23;
									arRoomLinks[iCurLevel][22][3] = 16;
									arRoomLinks[iCurLevel][23][1] = 22;
									arRoomLinks[iCurLevel][23][2] = 24;
									arRoomLinks[iCurLevel][23][3] = 17;
									arRoomLinks[iCurLevel][24][1] = 23;
									arRoomLinks[iCurLevel][24][3] = 18;
									PlaySound ("wav/move_room.wav");
									iChanged++;
								}

								/*** Vertical rectangle level. ***/
								if (InArea (327, 486, 327 + 49, 486 + 49) == 1)
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										/*** Clear. ***/
										arRoomLinks[iCurLevel][iRoomLoop][1] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][2] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][3] = 0;
										arRoomLinks[iCurLevel][iRoomLoop][4] = 0;
									}
									arRoomLinks[iCurLevel][1][2] = 2;
									arRoomLinks[iCurLevel][1][4] = 5;
									arRoomLinks[iCurLevel][2][1] = 1;
									arRoomLinks[iCurLevel][2][2] = 3;
									arRoomLinks[iCurLevel][2][4] = 6;
									arRoomLinks[iCurLevel][3][1] = 2;
									arRoomLinks[iCurLevel][3][2] = 4;
									arRoomLinks[iCurLevel][3][4] = 7;
									arRoomLinks[iCurLevel][4][1] = 3;
									arRoomLinks[iCurLevel][4][4] = 8;
									arRoomLinks[iCurLevel][5][2] = 6;
									arRoomLinks[iCurLevel][5][3] = 1;
									arRoomLinks[iCurLevel][5][4] = 9;
									arRoomLinks[iCurLevel][6][1] = 5;
									arRoomLinks[iCurLevel][6][2] = 7;
									arRoomLinks[iCurLevel][6][3] = 2;
									arRoomLinks[iCurLevel][6][4] = 10;
									arRoomLinks[iCurLevel][7][1] = 6;
									arRoomLinks[iCurLevel][7][2] = 8;
									arRoomLinks[iCurLevel][7][3] = 3;
									arRoomLinks[iCurLevel][7][4] = 11;
									arRoomLinks[iCurLevel][8][1] = 7;
									arRoomLinks[iCurLevel][8][3] = 4;
									arRoomLinks[iCurLevel][8][4] = 12;
									arRoomLinks[iCurLevel][9][2] = 10;
									arRoomLinks[iCurLevel][9][3] = 5;
									arRoomLinks[iCurLevel][9][4] = 13;
									arRoomLinks[iCurLevel][10][1] = 9;
									arRoomLinks[iCurLevel][10][2] = 11;
									arRoomLinks[iCurLevel][10][3] = 6;
									arRoomLinks[iCurLevel][10][4] = 14;
									arRoomLinks[iCurLevel][11][1] = 10;
									arRoomLinks[iCurLevel][11][2] = 12;
									arRoomLinks[iCurLevel][11][3] = 7;
									arRoomLinks[iCurLevel][11][4] = 15;
									arRoomLinks[iCurLevel][12][1] = 11;
									arRoomLinks[iCurLevel][12][3] = 8;
									arRoomLinks[iCurLevel][12][4] = 16;
									arRoomLinks[iCurLevel][13][2] = 14;
									arRoomLinks[iCurLevel][13][3] = 9;
									arRoomLinks[iCurLevel][13][4] = 17;
									arRoomLinks[iCurLevel][14][1] = 13;
									arRoomLinks[iCurLevel][14][2] = 15;
									arRoomLinks[iCurLevel][14][3] = 10;
									arRoomLinks[iCurLevel][14][4] = 18;
									arRoomLinks[iCurLevel][15][1] = 14;
									arRoomLinks[iCurLevel][15][2] = 16;
									arRoomLinks[iCurLevel][15][3] = 11;
									arRoomLinks[iCurLevel][15][4] = 19;
									arRoomLinks[iCurLevel][16][1] = 15;
									arRoomLinks[iCurLevel][16][3] = 12;
									arRoomLinks[iCurLevel][16][4] = 20;
									arRoomLinks[iCurLevel][17][2] = 18;
									arRoomLinks[iCurLevel][17][3] = 13;
									arRoomLinks[iCurLevel][17][4] = 21;
									arRoomLinks[iCurLevel][18][1] = 17;
									arRoomLinks[iCurLevel][18][2] = 19;
									arRoomLinks[iCurLevel][18][3] = 14;
									arRoomLinks[iCurLevel][18][4] = 22;
									arRoomLinks[iCurLevel][19][1] = 18;
									arRoomLinks[iCurLevel][19][2] = 20;
									arRoomLinks[iCurLevel][19][3] = 15;
									arRoomLinks[iCurLevel][19][4] = 23;
									arRoomLinks[iCurLevel][20][1] = 19;
									arRoomLinks[iCurLevel][20][3] = 16;
									arRoomLinks[iCurLevel][20][4] = 24;
									arRoomLinks[iCurLevel][21][2] = 22;
									arRoomLinks[iCurLevel][21][3] = 17;
									arRoomLinks[iCurLevel][22][1] = 21;
									arRoomLinks[iCurLevel][22][2] = 23;
									arRoomLinks[iCurLevel][22][3] = 18;
									arRoomLinks[iCurLevel][23][1] = 22;
									arRoomLinks[iCurLevel][23][2] = 24;
									arRoomLinks[iCurLevel][23][3] = 19;
									arRoomLinks[iCurLevel][24][1] = 23;
									arRoomLinks[iCurLevel][24][3] = 20;
									PlaySound ("wav/move_room.wav");
									iChanged++;
								}
							} else {
								if (MouseSelectAdj() == 1)
								{
									LinkPlus();
								}
							}
						}

						if (iScreen == 3) /*** events screen ***/
						{
							/*** total no. of events ***/
							if (InArea (150, 59, 150 + 13, 59 + 20) == 1)
								{ TotalEvents (-10); }
							if (InArea (165, 59, 165 + 13, 59 + 20) == 1)
								{ TotalEvents (-1); }
							if (InArea (235, 59, 235 + 13, 59 + 20) == 1)
								{ TotalEvents (1); }
							if (InArea (250, 59, 250 + 13, 59 + 20) == 1)
								{ TotalEvents (10); }

							/*** edit this event ***/
							if (InArea (472, 59, 472 + 13, 59 + 20) == 1)
								{ ChangeEvent (-10, 0); }
							if (InArea (487, 59, 487 + 13, 59 + 20) == 1)
								{ ChangeEvent (-1, 0); }
							if (InArea (557, 59, 557 + 13, 59 + 20) == 1)
								{ ChangeEvent (1, 0); }
							if (InArea (572, 59, 572 + 13, 59 + 20) == 1)
								{ ChangeEvent (10, 0); }

							/*** Opens ***/
							if (InArea (282, 306, 282 + 14, 306 + 14) == 1)
							{
								if (arEventsOpenClose[iCurLevel][iChangeEvent] != 0x01)
								{
									arEventsOpenClose[iCurLevel][iChangeEvent] = 0x01;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}

							/*** Closes ***/
							if (InArea (387, 306, 387 + 14, 306 + 14) == 1)
							{
								if (arEventsOpenClose[iCurLevel][iChangeEvent] != 0x00)
								{
									arEventsOpenClose[iCurLevel][iChangeEvent] = 0x00;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}

							/*** from room ***/
							if ((iYPos >= 136 * iScale) &&
								(iYPos <= (136 + 14) * iScale))
							{
								for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
								{
									if ((iXPos >= (282 + ((iRoomLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((282 + 14) +
										((iRoomLoop - 1) * 15)) * iScale))
									{
										EventRoom (iRoomLoop, 0);
									}
								}
							}

							/*** from tile ***/
							for (iColLoop = 1; iColLoop <= 3; iColLoop++)
							{
								for (iRowLoop = 1; iRowLoop <= 10; iRowLoop++)
								{
									if ((iXPos >= (282 + ((iRowLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((282 + 14) +
										((iRowLoop - 1) * 15)) * iScale))
									{
										if ((iYPos >= (238 + ((iColLoop - 1) * 15)) * iScale)
											&& (iYPos <= ((238 + 14) +
											((iColLoop - 1) * 15)) * iScale))
										{
											EventTile (iRowLoop, iColLoop, 0);
										}
									}
								}
							}

							/*** to room ***/
							if ((iYPos >= 377 * iScale) &&
								(iYPos <= (377 + 14) * iScale))
							{
								for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
								{
									if ((iXPos >= (282 + ((iRoomLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((282 + 14) +
										((iRoomLoop - 1) * 15)) * iScale))
									{
										EventRoom (iRoomLoop, 1);
									}
								}
							}

							/*** to tile ***/
							for (iColLoop = 1; iColLoop <= 3; iColLoop++)
							{
								for (iRowLoop = 1; iRowLoop <= 10; iRowLoop++)
								{
									if ((iXPos >= (282 + ((iRowLoop - 1) * 15)) * iScale)
										&& (iXPos <= ((282 + 14) +
										((iRowLoop - 1) * 15)) * iScale))
									{
										if ((iYPos >= (479 + ((iColLoop - 1) * 15)) * iScale)
											&& (iYPos <= ((479 + 14) +
											((iColLoop - 1) * 15)) * iScale))
										{
											EventTile (iRowLoop, iColLoop, 1);
										}
									}
								}
							}
						}
					}
					if (event.button.button == 2) /*** middle mouse button, clear ***/
					{
						if (iScreen == 1) { ClearRoom(); }
					}
					if (event.button.button == 3) /*** right mouse button, randomize ***/
					{
						if (iScreen == 1)
						{
							for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
							{
								for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
								{
									UseTile (-1, iTileLoop, iRoomLoop);
								}
							}
							PlaySound ("wav/ok_close.wav");
							iChanged++;
						}
						if (iScreen == 2)
						{
							if (arBrokenRoomLinks[iCurLevel] == 1)
							{
								if (MouseSelectAdj() == 1)
								{
									LinkMinus();
								}
							}
						}
					}
					ShowScreen();
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y > 0) /*** scroll wheel up ***/
					{
						if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** right ***/
								if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][2];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** up ***/
								if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][3];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					if (event.wheel.y < 0) /*** scroll wheel down ***/
					{
						if (InArea (iHor[1], iVer1 + iTTP1, iHor[10] + iDX,
							iVer3 + iDY + iTTPO) == 1) /*** middle field ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** left ***/
								if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][1];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** down ***/
								if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
								{
									iCurRoom = arRoomLinks[iCurLevel][iCurRoom][4];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					ShowScreen();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); } break;
				case SDL_QUIT:
					Quit(); break;
				default: break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
}
/*****************************************************************************/
void InitPopUpSave (void)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 1;

	PlaySound ("wav/popup_yn.wav");
	ShowPopUpSave();
	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							CallSave(); iPopUp = 0; break;
						case SDL_CONTROLLER_BUTTON_B:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_n:
							iPopUp = 0; break;
						case SDLK_y:
							CallSave(); iPopUp = 0; break;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (440, 376, 440 + 85, 376 + 32) == 1) /*** Yes ***/
						{
							iYesOn = 1;
							ShowPopUpSave();
						}
						if (InArea (167, 376, 167 + 85, 376 + 32) == 1) /*** No ***/
						{
							iNoOn = 1;
							ShowPopUpSave();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iYesOn = 0;
					iNoOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (440, 376, 440 + 85, 376 + 32) == 1) /*** Yes ***/
						{
							CallSave(); iPopUp = 0;
						}
						if (InArea (167, 376, 167 + 85, 376 + 32) == 1) /*** No ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUpSave(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); ShowPopUpSave(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowPopUpSave (void)
/*****************************************************************************/
{
	char arText[9 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfadedl, 0, 0, "imgfadedl");

	/*** popup ***/
	ShowImage (imgpopup_yn, 150, 148, "imgpopup_yn");

	/*** Yes ***/
	switch (iYesOn)
	{
		case 0: ShowImage (imgyes[1], 440, 376, "imgyes[1]"); break; /*** off ***/
		case 1: ShowImage (imgyes[2], 440, 376, "imgyes[2]"); break; /*** on ***/
	}

	/*** No ***/
	switch (iNoOn)
	{
		case 0: ShowImage (imgno[1], 167, 376, "imgno[1]"); break; /*** off ***/
		case 1: ShowImage (imgno[2], 167, 376, "imgno[2]"); break; /*** on ***/
	}

	if (iChanged == 1)
	{
		snprintf (arText[0], MAX_TEXT, "%s", "You made an unsaved change.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you want to save it?");
	} else {
		snprintf (arText[0], MAX_TEXT, "%s", "There are unsaved changes.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you wish to save these?");
	}

	DisplayText (180, 177, FONT_SIZE_15, arText, 2, font1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void LoadFonts (void)
/*****************************************************************************/
{
	font1 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_15 * iScale);
	if (font1 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
	font2 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_11 * iScale);
	if (font2 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
	font3 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf",
		FONT_SIZE_20 * iScale);
	if (font3 == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
}
/*****************************************************************************/
void MixAudio (void *unused, Uint8 *stream, int iLen)
/*****************************************************************************/
{
	int iTemp;
	int iAmount;

	if (unused != NULL) { } /*** To prevent warnings. ***/

	SDL_memset (stream, 0, iLen); /*** SDL2 ***/
	for (iTemp = 0; iTemp < NUM_SOUNDS; iTemp++)
	{
		iAmount = (sounds[iTemp].dlen-sounds[iTemp].dpos);
		if (iAmount > iLen)
		{
			iAmount = iLen;
		}
		SDL_MixAudio (stream, &sounds[iTemp].data[sounds[iTemp].dpos], iAmount,
			SDL_MIX_MAXVOLUME);
		sounds[iTemp].dpos += iAmount;
	}
}
/*****************************************************************************/
void PlaySound (char *sFile)
/*****************************************************************************/
{
	int iIndex;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;

	if (iNoAudio == 1) { return; }
	for (iIndex = 0; iIndex < NUM_SOUNDS; iIndex++)
	{
		if (sounds[iIndex].dpos == sounds[iIndex].dlen)
		{
			break;
		}
	}
	if (iIndex == NUM_SOUNDS) { return; }

	if (SDL_LoadWAV (sFile, &wave, &data, &dlen) == NULL)
	{
		printf ("[FAILED] Could not load %s: %s!\n", sFile, SDL_GetError());
		exit (EXIT_ERROR);
	}
	SDL_BuildAudioCVT (&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2,
		44100);
	/*** The "+ 1" is a workaround for SDL bug #2274. ***/
	cvt.buf = (Uint8 *)malloc (dlen * (cvt.len_mult + 1));
	memcpy (cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio (&cvt);
	SDL_FreeWAV (data);

	if (sounds[iIndex].data)
	{
		free (sounds[iIndex].data);
	}
	SDL_LockAudio();
	sounds[iIndex].data = cvt.buf;
	sounds[iIndex].dlen = cvt.len_cvt;
	sounds[iIndex].dpos = 0;
	SDL_UnlockAudio();
}
/*****************************************************************************/
void PreLoadSet (char cTypeP, int iTile)
/*****************************************************************************/
{
	char sDir[MAX_PATHFILE + 2];
	char sImage[MAX_IMG + 2];

	switch (cTypeP)
	{
		case 'd':
			snprintf (sDir, MAX_PATHFILE, "png%sdungeon%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s0x%02x.png", sDir, iTile);
			imgd[iTile][1] = IMG_LoadTexture (ascreen, sImage); /*** regular ***/
			snprintf (sDir, MAX_PATHFILE, "png%ssdungeon%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s0x%02x.png", sDir, iTile);
			imgd[iTile][2] = IMG_LoadTexture (ascreen, sImage);
			if ((!imgd[iTile][1]) || (!imgd[iTile][2]))
			{
				printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
				exit (EXIT_ERROR);
			}
			break;
		case 'p':
			snprintf (sDir, MAX_PATHFILE, "png%spalace%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s0x%02x.png", sDir, iTile);
			imgp[iTile][1] = IMG_LoadTexture (ascreen, sImage); /*** regular ***/
			snprintf (sDir, MAX_PATHFILE, "png%sspalace%s", SLASH, SLASH);
			snprintf (sImage, MAX_IMG, "%s0x%02x.png", sDir, iTile);
			imgp[iTile][2] = IMG_LoadTexture (ascreen, sImage);
			if ((!imgp[iTile][1]) || (!imgp[iTile][2]))
			{
				printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
				exit (EXIT_ERROR);
			}
			break;
	}

	iPreLoaded+=2;
}
/*****************************************************************************/
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage)
/*****************************************************************************/
{
	char sImage[MAX_IMG + 2];

	snprintf (sImage, MAX_IMG, "png%s%s%s%s", SLASH, sPath, SLASH, sPNG);
	*imgImage = IMG_LoadTexture (ascreen, sImage);
	if (!*imgImage)
	{
		printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
		exit (EXIT_ERROR);
	}

	iPreLoaded++;
}
/*****************************************************************************/
void ShowScreen (void)
/*****************************************************************************/
{
	int iTile;
	int iLoc;
	int iHorL, iVerL;
	char sLevelBar[MAX_TEXT + 2];
	char sLevelBarF[MAX_TEXT + 2];
	int iUnusedRooms;
	int iX, iY;
	SDL_Texture *imgskeleton[2 + 2];
	SDL_Texture *imgshadow[2 + 2];
	SDL_Texture *imgjaffar[2 + 2];
	SDL_Texture *imgguard[2 + 2];
	int iEventUnused;
	int iTileValue;
	int iToRoom;
	int iUnknown;

	/*** Used for looping. ***/
	int iTileLoop;
	int iRoomLoop;
	int iSideLoop;

	switch (iCurLevel)
	{
		case 1: cCurType = 'd'; break;
		case 2: cCurType = 'd'; break;
		case 3: cCurType = 'd'; break;
		case 4: cCurType = 'p'; break;
		case 5: cCurType = 'p'; break;
		case 6: cCurType = 'p'; break;
		case 7: cCurType = 'd'; break;
		case 8: cCurType = 'd'; break;
		case 9: cCurType = 'd'; break;
		case 10: cCurType = 'p'; break;
		case 11: cCurType = 'p'; break;
		case 12: cCurType = 'd'; break;
		case 13: cCurType = 'd'; break;
		case 14: cCurType = 'p'; break;
		case 15: cCurType = 'd'; break; /*** Also palace. ***/
		case 16: cCurType = 'd'; break;
		case 17: cCurType = 'd'; break;
		default:
			printf ("[FAILED] Invalid level number: %i!\n", iCurLevel);
			exit (EXIT_ERROR);
			break;
	}

	/*** black background ***/
	ShowImage (imgblack, 0, 0, "imgblack");

	if (iScreen == 1)
	{
		/*** above this room ***/
		if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
		{
			for (iTileLoop = 1; iTileLoop <= (TILES / 3); iTileLoop++)
			{
				iTile = arRoomTiles[iCurLevel]
					[arRoomLinks[iCurLevel][iCurRoom][3]][iTileLoop + 20];
				switch (cCurType)
				{
					case 'd':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgd[iTile][1], iHor[iTileLoop], iVer0, sInfo);
						break;
					case 'p':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgp[iTile][1], iHor[iTileLoop], iVer0, sInfo);
						break;
				}
			}
		} else {
			for (iTileLoop = 1; iTileLoop <= (TILES / 3); iTileLoop++)
			{
				iTile = 0x01;
				switch (cCurType)
				{
					case 'd':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgd[iTile][1], iHor[iTileLoop], iVer0, sInfo);
						break;
					case 'p':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgp[iTile][1], iHor[iTileLoop], iVer0, sInfo);
						break;
				}
			}
		}

		/*** under this room ***/
		if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
		{
			for (iTileLoop = 1; iTileLoop <= (TILES / 3); iTileLoop++)
			{
				iTile = arRoomTiles[iCurLevel]
					[arRoomLinks[iCurLevel][iCurRoom][4]][iTileLoop];
				switch (cCurType)
				{
					case 'd':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgd[iTile][1], iHor[iTileLoop], iVer4, sInfo);
						break;
					case 'p':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgp[iTile][1], iHor[iTileLoop], iVer4, sInfo);
						break;
				}
			}
		}

		/*** One tile: top row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			iTile = arRoomTiles[iCurLevel]
				[arRoomLinks[iCurLevel][iCurRoom][1]][10];
		} else {
			iTile = 0x14;
		}
		switch (cCurType)
		{
			case 'd':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgd[iTile][1], iHor[0], iVer1, sInfo);
				break;
			case 'p':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgp[iTile][1], iHor[0], iVer1, sInfo);
				break;
		}
		ShowImage (imgfadeds, iHor[0], iVer1, "imgfadeds");

		/*** One tile: middle row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			iTile = arRoomTiles[iCurLevel]
				[arRoomLinks[iCurLevel][iCurRoom][1]][20];
		} else {
			iTile = 0x14;
		}
		switch (cCurType)
		{
			case 'd':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgd[iTile][1], iHor[0], iVer2, sInfo);
				break;
			case 'p':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgp[iTile][1], iHor[0], iVer2, sInfo);
				break;
		}
		ShowImage (imgfadeds, iHor[0], iVer2, "imgfadeds");

		/*** One tile: 'top' row, room left down. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0) /*** left ***/
		{
			if (arRoomLinks[iCurLevel]
				[arRoomLinks[iCurLevel][iCurRoom][1]][4] != 0) /*** down ***/
			{
				iTile = arRoomTiles[iCurLevel]
					[arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iCurRoom][1]][4]]
					[10];
			} else {
				iTile = 0x14;
			}
		}
		switch (cCurType)
		{
			case 'd':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgd[iTile][1], iHor[0], iVer4, sInfo);
				break;
			case 'p':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgp[iTile][1], iHor[0], iVer4, sInfo);
				break;
		}
		ShowImage (imgfadeds, iHor[0], iVer4, "imgfadeds");

		/*** One tile: bottom row, room left. ***/
		if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
		{
			iTile = arRoomTiles[iCurLevel]
				[arRoomLinks[iCurLevel][iCurRoom][1]][30];
		} else {
			iTile = 0x14;
		}
		switch (cCurType)
		{
			case 'd':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgd[iTile][1], iHor[0], iVer3, sInfo);
				break;
			case 'p':
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgp[iTile][1], iHor[0], iVer3, sInfo);
				break;
		}
		ShowImage (imgfadeds, iHor[0], iVer3, "imgfadeds");

		/*** Tiles. ***/
		for (iTileLoop = 1; iTileLoop <= 30; iTileLoop++)
		{
			iLoc = 0;
			iHorL = iHor[0]; /*** To prevent warnings. ***/
			iVerL = iVer0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
			{
				iLoc = 20 + iTileLoop;
				iHorL = iHor[iTileLoop];
				iVerL = iVer3;
			}
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
			{
				iLoc = iTileLoop;
				iHorL = iHor[iTileLoop - 10];
				iVerL = iVer2;
			}
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
			{
				iLoc = -20 + iTileLoop;
				iHorL = iHor[iTileLoop - 20];
				iVerL = iVer1;
			}
			iTile = arRoomTiles[iCurLevel][iCurRoom][iLoc];
			if (imgd[iTile][1] == NULL) { iUnknown = 1; } else { iUnknown = 0; }
			if (iUnknown == 1)
			{
				snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
				ShowImage (imgunk[1], iHorL, iVerL, sInfo);
			} else {
				switch (cCurType)
				{
					case 'd':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgd[iTile][1], iHorL, iVerL, sInfo);
						break;
					case 'p':
						snprintf (sInfo, MAX_INFO, "tile=%i", iTile);
						ShowImage (imgp[iTile][1], iHorL, iVerL, sInfo);
						break;
				}
			}
			if (iLoc == iSelected)
			{
				if (iUnknown == 1)
				{
					snprintf (sInfo, MAX_INFO, "high=%i", iTile);
					ShowImage (imgunk[2], iHorL, iVerL, sInfo);
				} else {
					switch (cCurType)
					{
						case 'd':
							snprintf (sInfo, MAX_INFO, "high=%i", iTile);
							ShowImage (imgd[iTile][2], iHorL, iVerL, sInfo);
							break;
						case 'p':
							snprintf (sInfo, MAX_INFO, "high=%i", iTile);
							ShowImage (imgp[iTile][2], iHorL, iVerL, sInfo);
							break;
					}
				}
			}

			/*** prince ***/
			if ((iCurRoom == arStartLocation[iCurLevel][1]) &&
				(iLoc == arStartLocation[iCurLevel][2]))
			{
				switch (arStartLocation[iCurLevel][3])
				{
					case 0x00: /*** looks right ***/
						ShowImage (imgprincer[1], iHorL + 20, iVerL + 20, "imgprincer[1]");
						if (iSelected == iLoc)
							{ ShowImage (imgprincer[2], iHorL + 20,
							iVerL + 20, "imgprincer[2]"); }
						break;
					case 0xFF: /*** looks left ***/
						ShowImage (imgprincel[1], iHorL + 24, iVerL + 20, "imgprincel[1]");
						if (iSelected == iLoc)
							{ ShowImage (imgprincel[2], iHorL + 24,
							iVerL + 20, "imgprincel[2]"); }
						break;
					default:
						printf ("[ WARN ] Strange prince direction: 0x%02x!\n",
							arStartLocation[iCurLevel][3]);
				}
			}

			/*** guard ***/
			if (arGuardTile[iCurLevel][iCurRoom] == iLoc)
			{
				switch (arGuardDir[iCurLevel][iCurRoom])
				{
					case 0xFF: /*** l ***/
						imgskeleton[1] = imgskeletonl[1];
						imgshadow[1] = imgshadowl[1];
						imgjaffar[1] = imgjaffarl[1];
						imgguard[1] = imgguardl[1];
						imgskeleton[2] = imgskeletonl[2];
						imgshadow[2] = imgshadowl[2];
						imgjaffar[2] = imgjaffarl[2];
						imgguard[2] = imgguardl[2];
						break;
					case 0x00: /*** r ***/
						imgskeleton[1] = imgskeletonr[1];
						imgshadow[1] = imgshadowr[1];
						imgjaffar[1] = imgjaffarr[1];
						imgguard[1] = imgguardr[1];
						imgskeleton[2] = imgskeletonr[2];
						imgshadow[2] = imgshadowr[2];
						imgjaffar[2] = imgjaffarr[2];
						imgguard[2] = imgguardr[2];
						break;
					default:
						printf ("[FAILED] Incorrect guard direction: 0x%02x!\n",
							arGuardDir[iCurLevel][iCurRoom]);
						exit (EXIT_ERROR);
				}
				switch (iCurLevel)
				{
					case 3:
						ShowImage (imgskeleton[1], iHorL, iVerL + 24, "imgskeleton[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgskeleton[2], iHorL,
							iVerL + 24, "imgskeleton[2]"); }
						break;
					case 12:
						ShowImage (imgshadow[1], iHorL, iVerL + 16, "imgshadow[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgshadow[2], iHorL, iVerL + 16, "imgshadow[2]"); }
						break;
					case 13:
						ShowImage (imgjaffar[1], iHorL, iVerL + 12, "imgjaffar[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgjaffar[2], iHorL, iVerL + 12, "imgjaffar[2]"); }
						break;
					default:
						ShowImage (imgguard[1], iHorL, iVerL + 16, "imgguard[1]");
						if (iLoc == iSelected)
							{ ShowImage (imgguard[2], iHorL, iVerL + 16, "imgguard[2]"); }
						break;
				}
			}
		}

		if (iCurLevel == 15)
		{
			ShowImage (imgvwarning, 40, 472, "imgvwarning");
		}
	}
	if (iScreen == 2) /*** R ***/
	{
		if (arBrokenRoomLinks[iCurLevel] == 0)
		{
			InitRooms();
			/*** room links ***/
			ShowImage (imgrl, 25, 50, "imgrl");
			/*** rooms broken on ***/
			if (iDownAt == 11)
			{
				ShowImage (imgbroomson_1, 629, 63, "imgbroomson_1"); /*** down ***/
			} else {
				ShowImage (imgbroomson_0, 629, 63, "imgbroomson_0"); /*** up ***/
			}
			WhereToStart();
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				arDone[iRoomLoop] = 0;
			}
			ShowRooms (arStartLocation[iCurLevel][1], iStartRoomsX, iStartRoomsY, 1);
			iUnusedRooms = 0;
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				if (arDone[iRoomLoop] != 1)
				{
					iUnusedRooms++;
					ShowRooms (iRoomLoop, 25, iUnusedRooms, 0);

					/* Give unused rooms 0xFF as tile 1. This may not be the best
					 * location in the code to do this, but it works.
					 */
					if (iCurLevel != 15)
					{
						arRoomTiles[iCurLevel][iRoomLoop][1] = 0xFF;
					}
				}
			}
			if (iMovingRoom != 0)
			{
				iX = (iXPos + 10) / iScale;
				iY = (iYPos + 10) / iScale;
				ShowImage (imgroom[iMovingRoom], iX, iY, "imgroom[...]");
				if (iCurRoom == iMovingRoom)
				{
					ShowImage (imgsrc, iX, iY, "imgsrc"); /*** green stripes ***/
				}
				if (arStartLocation[iCurLevel][1] == iMovingRoom)
				{
					ShowImage (imgsrs, iX, iY, "imgsrs"); /*** blue border ***/
				}
				ShowImage (imgsrm, iX, iY, "imgsrm"); /*** red stripes ***/
				ShowRooms (-1, iMovingNewX, iMovingNewY, 0);
			}
		} else {
			/*** broken room links ***/
			ShowImage (imgbrl, 25, 50, "imgbrl");
			for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
			{
				/*** green stripes ***/
				if (iCurRoom == iRoomLoop)
				{
					BrokenRoomChange (iRoomLoop, 0, &iX, &iY);
					ShowImage (imgsrc, iX, iY, "imgsrc");
				}

				/*** blue border ***/
				if (arStartLocation[iCurLevel][1] == iRoomLoop)
				{
					BrokenRoomChange (iRoomLoop, 0, &iX, &iY);
					ShowImage (imgsrs, iX, iY, "imgsrs");
				}

				for (iSideLoop = 1; iSideLoop <= 4; iSideLoop++)
				{
					if (arRoomLinks[iCurLevel][iRoomLoop][iSideLoop] != 0)
					{
						BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
						iToRoom = arRoomLinks[iCurLevel][iRoomLoop][iSideLoop];
						if ((iToRoom < 1) || (iToRoom > ROOMS)) { iToRoom = 25; }
						ShowImage (imgroom[iToRoom], iX, iY, "imgroom[...]");

						/*** blue square ***/
						if (arRoomConnectionsBroken[iCurLevel][iRoomLoop][iSideLoop] == 1)
						{
							BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
							ShowImage (imgsrb, iX, iY, "imgsrb");
						}
					}

					/*** red stripes ***/
					if ((iChangingBrokenRoom == iRoomLoop) &&
						(iChangingBrokenSide == iSideLoop))
					{
						BrokenRoomChange (iRoomLoop, iSideLoop, &iX, &iY);
						ShowImage (imgsrm, iX, iY, "imgsrm");
					}
				}
			}
		}
	}
	if (iScreen == 3) /*** E ***/
	{
		iEventUnused = 0;

		/*** background ***/
		ShowImage (imgevents, 25, 50, "imgevents");

		/*** total no. of events ***/
		CenterNumber (arNrEvents[iCurLevel], 178, 59, color_wh, 0);

		/*** edit this event ***/
		CenterNumber (iChangeEvent, 500, 59, color_wh, 0);

		/*** from room ***/
		if ((arEventsFromRoom[iCurLevel][iChangeEvent] >= 1) &&
			(arEventsFromRoom[iCurLevel][iChangeEvent] <= 24))
		{
			iX = 267 + (arEventsFromRoom[iCurLevel][iChangeEvent] * 15);
			iY = 136;
			ShowImage (imgsele, iX, iY, "imgsele");
		} else { iEventUnused = 1; }

		/*** from tile ***/
		if ((arEventsFromTile[iCurLevel][iChangeEvent] >= 1) &&
			(arEventsFromTile[iCurLevel][iChangeEvent] <= 30))
		{
			iTile = arEventsFromTile[iCurLevel][iChangeEvent];
			if ((iTile >= 1) && (iTile <= 10))
			{
				iX = 267 + (iTile * 15);
				iY = 238;
			}
			if ((iTile >= 11) && (iTile <= 20))
			{
				iX = 267 + ((iTile - 10) * 15);
				iY = 253;
			}
			if ((iTile >= 21) && (iTile <= 30))
			{
				iX = 267 + ((iTile - 20) * 15);
				iY = 268;
			}
			ShowImage (imgsele, iX, iY, "imgsele");
		} else { iEventUnused = 1; }

		/*** from image ***/
		if (iEventUnused == 0)
		{
			iTileValue = arRoomTiles[iCurLevel]
				[arEventsFromRoom[iCurLevel][iChangeEvent]]
				[arEventsFromTile[iCurLevel][iChangeEvent]];
			snprintf (sInfo, MAX_INFO, "tile=%i", iTileValue);
			switch (cCurType)
			{
				case 'd': ShowImage (imgd[iTileValue][1], 128, 112, sInfo); break;
				case 'p': ShowImage (imgp[iTileValue][1], 128, 112, sInfo); break;
			}
		} else {
			ShowImage (imgeventu, 128, 112, "imgeventu");
		}

		/*** open or close ***/
		switch (arEventsOpenClose[iCurLevel][iChangeEvent])
		{
			case 0x00: /*** close ***/
				ShowImage (imgsell, 387, 306, "imgsell"); break;
			case 0x01: /*** open ***/
				ShowImage (imgsell, 282, 306, "imgsell"); break;
		}

		iEventUnused = 0;

		/*** to room ***/
		if ((arEventsToRoom[iCurLevel][iChangeEvent] >= 1) &&
			(arEventsToRoom[iCurLevel][iChangeEvent] <= 24))
		{
			iX = 267 + (arEventsToRoom[iCurLevel][iChangeEvent] * 15);
			iY = 377;
			ShowImage (imgsele, iX, iY, "imgsele");
		} else { iEventUnused = 1; }

		/*** to tile ***/
		if ((arEventsToTile[iCurLevel][iChangeEvent] >= 1) &&
			(arEventsToTile[iCurLevel][iChangeEvent] <= 30))
		{
			iTile = arEventsToTile[iCurLevel][iChangeEvent];
			if ((iTile >= 1) && (iTile <= 10))
			{
				iX = 267 + (iTile * 15);
				iY = 479;
			}
			if ((iTile >= 11) && (iTile <= 20))
			{
				iX = 267 + ((iTile - 10) * 15);
				iY = 494;
			}
			if ((iTile >= 21) && (iTile <= 30))
			{
				iX = 267 + ((iTile - 20) * 15);
				iY = 509;
			}
			ShowImage (imgsele, iX, iY, "imgsele");
		} else { iEventUnused = 1; }

		/*** to image ***/
		if (iEventUnused == 0)
		{
			iTileValue = arRoomTiles[iCurLevel]
				[arEventsToRoom[iCurLevel][iChangeEvent]]
				[arEventsToTile[iCurLevel][iChangeEvent]];
			snprintf (sInfo, MAX_INFO, "tile=%i", iTileValue);
			switch (cCurType)
			{
				case 'd': ShowImage (imgd[iTileValue][1], 128, 353, sInfo); break;
				case 'p': ShowImage (imgp[iTileValue][1], 128, 353, sInfo); break;
			}
		} else {
			ShowImage (imgeventu, 128, 353, "imgeventu");
		}
	}

	/*** left ***/
	if (arRoomLinks[iCurLevel][iCurRoom][1] != 0)
	{
		/*** yes ***/
		if (iDownAt == 1)
		{
			ShowImage (imgleft_1, 0, 50, "imgleft_1"); /*** down ***/
		} else {
			ShowImage (imgleft_0, 0, 50, "imgleft_0"); /*** up ***/
		}
		if (arRoomLinks[iCurLevel][iCurRoom][1] > 24)
			{ ShowImage (imglinkwarnlr, 0, 50, "imglinkwarnlr"); } /*** glow ***/
	} else {
		/*** no ***/
		ShowImage (imglrno, 0, 50, "imglrno");
	}

	/*** right ***/
	if (arRoomLinks[iCurLevel][iCurRoom][2] != 0)
	{
		/*** yes ***/
		if (iDownAt == 2)
		{
			ShowImage (imgright_1, 667, 50, "imgright_1"); /*** down ***/
		} else {
			ShowImage (imgright_0, 667, 50, "imgright_0"); /*** up ***/
		}
		if (arRoomLinks[iCurLevel][iCurRoom][2] > 24)
			{ ShowImage (imglinkwarnlr, 667, 50, "imglinkwarnlr"); } /*** glow ***/
	} else {
		/*** no ***/
		ShowImage (imglrno, 667, 50, "imglrno");
	}

	/*** up ***/
	if (arRoomLinks[iCurLevel][iCurRoom][3] != 0)
	{
		/*** yes ***/
		if (iDownAt == 3)
		{
			ShowImage (imgup_1, 25, 25, "imgup_1"); /*** down ***/
		} else {
			ShowImage (imgup_0, 25, 25, "imgup_0"); /*** up ***/
		}
		if (arRoomLinks[iCurLevel][iCurRoom][3] > 24)
			{ ShowImage (imglinkwarnud, 25, 25, "imglinkwarnud"); } /*** glow ***/
	} else {
		/*** no ***/
		if (iScreen != 1)
		{
			ShowImage (imgudno, 25, 25, "imgudno"); /*** without info ***/
		} else {
			ShowImage (imgudnonfo, 25, 25, "imgudnonfo"); /*** with info ***/
		}
	}

	/*** down ***/
	if (arRoomLinks[iCurLevel][iCurRoom][4] != 0)
	{
		/*** yes ***/
		if (iDownAt == 4)
		{
			ShowImage (imgdown_1, 25, 548, "imgdown_1"); /*** down ***/
		} else {
			ShowImage (imgdown_0, 25, 548, "imgdown_0"); /*** up ***/
		}
		if (arRoomLinks[iCurLevel][iCurRoom][4] > 24)
			{ ShowImage (imglinkwarnud, 25, 548, "imglinkwarnud"); } /*** glow ***/
	} else {
		/*** no ***/
		ShowImage (imgudno, 25, 548, "imgudno");
	}

	switch (iScreen)
	{
		case 1:
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				/*** rooms broken on ***/
				if (iDownAt == 11)
				{
					ShowImage (imgbroomson_1, 0, 25, "imgbroomson_1"); /*** down ***/
				} else {
					ShowImage (imgbroomson_0, 0, 25, "imgbroomson_0"); /*** up ***/
				}
			} else {
				/*** rooms on ***/
				if (iDownAt == 5)
				{
					ShowImage (imgroomson_1, 0, 25, "imgroomson_1"); /*** down ***/
				} else {
					ShowImage (imgroomson_0, 0, 25, "imgroomson_0"); /*** up ***/
				}
			}
			/*** events on ***/
			if (iDownAt == 6)
			{
				ShowImage (imgeventson_1, 667, 25, "imgeventson_1"); /*** down ***/
			} else {
				ShowImage (imgeventson_0, 667, 25, "imgeventson_0"); /*** up ***/
			}
			break;
		case 2:
			/*** rooms off ***/
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				ShowImage (imgbroomsoff, 0, 25, "imgbroomsoff"); /*** broken ***/
			} else {
				ShowImage (imgroomsoff, 0, 25, "imgroomsoff");
			}

			/*** events on ***/
			if (iDownAt == 6)
			{
				ShowImage (imgeventson_1, 667, 25, "imgeventson_1"); /*** down ***/
			} else {
				ShowImage (imgeventson_0, 667, 25, "imgeventson_0"); /*** up ***/
			}
			break;
		case 3:
			if (arBrokenRoomLinks[iCurLevel] == 1)
			{
				/*** rooms broken on ***/
				if (iDownAt == 11)
				{
					ShowImage (imgbroomson_1, 0, 25, "imgbroomson_1"); /*** down ***/
				} else {
					ShowImage (imgbroomson_0, 0, 25, "imgbroomson_0"); /*** up ***/
				}
			} else {
				/*** rooms on ***/
				if (iDownAt == 5)
				{
					ShowImage (imgroomson_1, 0, 25, "imgroomson_1"); /*** down ***/
				} else {
					ShowImage (imgroomson_0, 0, 25, "imgroomson_0"); /*** up ***/
				}
			}
			/*** events off ***/
			ShowImage (imgeventsoff, 667, 25, "imgeventsoff");
			break;
	}

	/*** save ***/
	if (iChanged != 0)
	{
		/*** on ***/
		if (iDownAt == 7)
		{
			ShowImage (imgsaveon_1, 0, 548, "imgsaveon_1"); /*** down ***/
		} else {
			ShowImage (imgsaveon_0, 0, 548, "imgsaveon_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgsaveoff, 0, 548, "imgsaveoff");
	}

	/*** quit ***/
	if (iDownAt == 8)
	{
		ShowImage (imgquit_1, 667, 548, "imgquit_1"); /*** down ***/
	} else {
		ShowImage (imgquit_0, 667, 548, "imgquit_0"); /*** up ***/
	}

	/*** previous ***/
	if (iCurLevel != 1)
	{
		/*** on ***/
		if (iDownAt == 9)
		{
			ShowImage (imgprevon_1, 0, 0, "imgprevon_1"); /*** down ***/
		} else {
			ShowImage (imgprevon_0, 0, 0, "imgprevon_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgprevoff, 0, 0, "imgprevoff");
	}

	/*** next ***/
	if (iCurLevel != LEVELS)
	{
		/*** on ***/
		if (iDownAt == 10)
		{
			ShowImage (imgnexton_1, 667, 0, "imgnexton_1"); /*** down ***/
		} else {
			ShowImage (imgnexton_0, 667, 0, "imgnexton_0"); /*** up ***/
		}
	} else {
		/*** off ***/
		ShowImage (imgnextoff, 667, 0, "imgnextoff");
	}

	/*** level bar ***/
	ShowImage (imgbar, 25, 0, "imgbar");

	/*** Assemble level bar text. ***/
	switch (iCurLevel)
	{
		case 1: snprintf (sLevelBar, MAX_TEXT, "level 1 (prison),"); break;
		case 2: snprintf (sLevelBar, MAX_TEXT, "level 2 (guards),"); break;
		case 3: snprintf (sLevelBar, MAX_TEXT, "level 3 (skeleton),"); break;
		case 4: snprintf (sLevelBar, MAX_TEXT, "level 4 (mirror),"); break;
		case 5: snprintf (sLevelBar, MAX_TEXT, "level 5 (thief),"); break;
		case 6: snprintf (sLevelBar, MAX_TEXT, "level 6 (plunge),"); break;
		case 7: snprintf (sLevelBar, MAX_TEXT, "level 7 (weightless),"); break;
		case 8: snprintf (sLevelBar, MAX_TEXT, "level 8 (mouse),"); break;
		case 9: snprintf (sLevelBar, MAX_TEXT, "level 9 (twisty),"); break;
		case 10: snprintf (sLevelBar, MAX_TEXT, "level 10 (quad),"); break;
		case 11: snprintf (sLevelBar, MAX_TEXT, "level 11 (fragile),"); break;
		case 12: snprintf (sLevelBar, MAX_TEXT, "level 12a (tower),"); break;
		case 13: snprintf (sLevelBar, MAX_TEXT, "level 12b (jaffar),"); break;
		case 14: snprintf (sLevelBar, MAX_TEXT, "level 12c (rescue),"); break;
		case 15: snprintf (sLevelBar, MAX_TEXT, "various d/p,"); break;
		case 16: snprintf (sLevelBar, MAX_TEXT, "training,"); break;
		case 17: snprintf (sLevelBar, MAX_TEXT, "training (cont.),"); break;
	}
	switch (iScreen)
	{
		case 1:
			snprintf (sLevelBarF, MAX_TEXT, "%s room %i", sLevelBar, iCurRoom);
			ShowImage (imgextras[iExtras], 610, 3, "imgextras[...]");
			break;
		case 2:
			snprintf (sLevelBarF, MAX_TEXT, "%s room links", sLevelBar); break;
		case 3:
			snprintf (sLevelBarF, MAX_TEXT, "%s events", sLevelBar); break;
	}

	/*** Mednafen information. ***/
	if (iMednafen == 1) { ShowImage (imgmednafen, 25, 50, "imgmednafen"); }

	/*** Display level bar text. ***/
	message = TTF_RenderText_Shaded (font1, sLevelBarF, color_bl, color_wh);
	messaget = SDL_CreateTextureFromSurface (ascreen, message);
	offset.x = 31;
	offset.y = 5;
	offset.w = message->w; offset.h = message->h;
	CustomRenderCopy (messaget, NULL, &offset, "message");
	SDL_DestroyTexture (messaget); SDL_FreeSurface (message);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void InitPopUp (void)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 1;

	PlaySound ("wav/popup.wav");
	ShowPopUp();
	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iPopUp = 0;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (440, 376, 440 + 85, 376 + 32) == 1) /*** OK ***/
						{
							iOKOn = 1;
							ShowPopUp();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iOKOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (440, 376, 440 + 85, 376 + 32) == 1) /*** OK ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUp(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowScreen(); ShowPopUp(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowPopUp (void)
/*****************************************************************************/
{
	char arText[9 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfadedl, 0, 0, "imgfadedl");

	/*** popup ***/
	ShowImage (imgpopup, 52, 64, "imgpopup");

	/*** OK ***/
	switch (iOKOn)
	{
		case 0: ShowImage (imgok[1], 440, 376, "imgok[1]"); break; /*** off ***/
		case 1: ShowImage (imgok[2], 440, 376, "imgok[2]"); break; /*** on ***/
	}

	snprintf (arText[0], MAX_TEXT, "%s %s", EDITOR_NAME, EDITOR_VERSION);
	snprintf (arText[1], MAX_TEXT, "%s", COPYRIGHT);
	snprintf (arText[2], MAX_TEXT, "%s", "");
	if (iController != 1)
	{
		snprintf (arText[3], MAX_TEXT, "%s", "single tile (change or select)");
		snprintf (arText[4], MAX_TEXT, "%s", "entire room (clear or fill)");
		snprintf (arText[5], MAX_TEXT, "%s", "entire level (randomize or fill)");
	} else {
		snprintf (arText[3], MAX_TEXT, "%s", "The detected game controller:");
		snprintf (arText[4], MAX_TEXT, "%s", sControllerName);
		snprintf (arText[5], MAX_TEXT, "%s", "Have fun using it!");
	}
	snprintf (arText[6], MAX_TEXT, "%s", "");
	snprintf (arText[7], MAX_TEXT, "%s", "You may use one guard per room.");
	snprintf (arText[8], MAX_TEXT, "%s", "The tile behavior may differ per"
		" level.");

	DisplayText (180, 177, FONT_SIZE_15, arText, 9, font1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void Help (void)
/*****************************************************************************/
{
	int iHelp;
	SDL_Event event;

	iHelp = 1;

	PlaySound ("wav/popup.wav");
	ShowHelp();
	while (iHelp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iHelp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iHelp = 0;
						default: break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if (InArea (87, 413, 87 + 517, 413 + 20) == 1)
					{
						SDL_SetCursor (curHand);
					} else {
						SDL_SetCursor (curArrow);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (590, 523, 674, 554) == 1) /*** OK ***/
						{
							iHelpOK = 1;
							ShowHelp();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iHelpOK = 0;
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (590, 523, 674, 554) == 1)
							{ iHelp = 0; }
						if (InArea (87, 413, 87 + 517, 413 + 20) == 1)
							{ OpenURL ("https://github.com/EndeavourAccuracy/legbop"); }
					}
					ShowHelp(); break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowHelp(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowHelp (void)
/*****************************************************************************/
{
	/*** background ***/
	ShowImage (imghelp, 0, 0, "imghelp");

	/*** OK ***/
	switch (iHelpOK)
	{
		case 0: ShowImage (imgok[1], 590, 523, "imgok[1]"); break; /*** off ***/
		case 1: ShowImage (imgok[2], 590, 523, "imgok[2]"); break; /*** on ***/
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void EXE (void)
/*****************************************************************************/
{
	int iEXE;
	SDL_Event event;
	int iValue, iRGBLoop, iY;
	int iOldXPos, iOldYPos;
	char sLineTemp[SLIDES_LINES_CHARS + 2];
	char cAddChar;

	iEXE = 1;
	iN = 0;

	EXELoad();

	PlaySound ("wav/popup.wav");
	ShowEXE();
	while (iEXE == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							EXESave(); iEXE = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							iEXE = 0; break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						/*** Not including SDLK_SPACE here. ***/
						case SDLK_s:
							if (iS == 0) { EXESave(); iEXE = 0; }
							break;
						case SDLK_BACKSPACE:
							if (iS != 0)
							{
								if (strlen (arIntroSlides[iS][iL]) > 0)
								{
									arIntroSlides[iS][iL]
										[strlen (arIntroSlides[iS][iL]) - 1] = '\0';
									IntroSlides();
									PlaySound ("wav/hum_adj.wav");
								}
							}
							break;
						default: break;
					}
					ShowEXE();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					/*** Intro slides. ***/
					iS = 0;
					iL = 0;
					/*** 1 ***/
					if (InArea (34, 209, 34 + 222, 209 + 12) == 1) { iS = 1; iL = 1; }
					if (InArea (34, 225, 34 + 222, 225 + 12) == 1) { iS = 1; iL = 2; }
					if (InArea (34, 241, 34 + 222, 241 + 12) == 1) { iS = 1; iL = 3; }
					if (InArea (34, 257, 34 + 222, 257 + 12) == 1) { iS = 1; iL = 4; }
					/*** 2 ***/
					if (InArea (34, 313, 34 + 222, 313 + 12) == 1) { iS = 2; iL = 1; }
					if (InArea (34, 329, 34 + 222, 329 + 12) == 1) { iS = 2; iL = 2; }
					if (InArea (34, 345, 34 + 222, 345 + 12) == 1) { iS = 2; iL = 3; }
					if (InArea (34, 361, 34 + 222, 361 + 12) == 1) { iS = 2; iL = 4; }
					/*** 3 ***/
					if (InArea (266, 313, 266 + 222, 313 + 12) == 1) { iS = 3; iL = 1; }
					if (InArea (266, 329, 266 + 222, 329 + 12) == 1) { iS = 3; iL = 2; }
					if (InArea (266, 345, 266 + 222, 345 + 12) == 1) { iS = 3; iL = 3; }
					if (InArea (266, 361, 266 + 222, 361 + 12) == 1) { iS = 3; iL = 4; }
					/*** 4 ***/
					if (InArea (34, 417, 34 + 222, 417 + 12) == 1) { iS = 4; iL = 1; }
					if (InArea (34, 433, 34 + 222, 433 + 12) == 1) { iS = 4; iL = 2; }
					if (InArea (34, 449, 34 + 222, 449 + 12) == 1) { iS = 4; iL = 3; }
					if (InArea (34, 465, 34 + 222, 465 + 12) == 1) { iS = 4; iL = 4; }
					/*** 5 ***/
					if (InArea (266, 417, 266 + 222, 417 + 12) == 1) { iS = 5; iL = 1; }
					if (InArea (266, 433, 266 + 222, 433 + 12) == 1) { iS = 5; iL = 2; }
					if (InArea (266, 449, 266 + 222, 449 + 12) == 1) { iS = 5; iL = 3; }
					if (InArea (266, 465, 266 + 222, 465 + 12) == 1) { iS = 5; iL = 4; }
					/*** stop/start input ***/
					if (iS == 0)
					{
						if (iN == 1) { IntroSlides(); iN = 0; }
						SDL_SetCursor (curArrow);
						SDL_StopTextInput();
					} else {
						SDL_SetCursor (curText);
						SDL_StartTextInput();
					}

					ShowEXE();
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (590, 523, 674, 554) == 1) /*** Save ***/
						{
							iEXESave = 1;
							ShowEXE();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iEXESave = 0;
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (590, 523, 674, 554) == 1) /*** Save ***/
						{
							EXESave(); iEXE = 0;
						}

						/*** Starting minutes left. ***/
						PlusMinus (&iEXEMinutesLeft, 219, 34, 0, 255, -10, 0);
						PlusMinus (&iEXEMinutesLeft, 234, 34, 0, 255, -1, 0);
						PlusMinus (&iEXEMinutesLeft, 304, 34, 0, 255, +1, 0);
						PlusMinus (&iEXEMinutesLeft, 319, 34, 0, 255, +10, 0);

						/*** Starting hit points. ***/
						PlusMinus (&iEXEHitPoints, 219, 58, 0, 255, -10, 0);
						PlusMinus (&iEXEHitPoints, 234, 58, 0, 255, -1, 0);
						PlusMinus (&iEXEHitPoints, 304, 58, 0, 255, +1, 0);
						PlusMinus (&iEXEHitPoints, 319, 58, 0, 255, +10, 0);

						/*** Prince colors. ***/
						for (iRGBLoop = 1; iRGBLoop <= 9; iRGBLoop++)
						{
							switch (iRGBLoop)
							{
								case 1: iValue = iEXEHairR; iY = 34; break;
								case 2: iValue = iEXEHairG; iY = 58; break;
								case 3: iValue = iEXEHairB; iY = 82; break;
								case 4: iValue = iEXESkinR; iY = 116; break;
								case 5: iValue = iEXESkinG; iY = 140; break;
								case 6: iValue = iEXESkinB; iY = 164; break;
								case 7: iValue = iEXESuitR; iY = 198; break;
								case 8: iValue = iEXESuitG; iY = 222; break;
								case 9: iValue = iEXESuitB; iY = 246; break;
							}
							PlusMinus (&iValue, 469, iY, 0, 31, -10, 0);
							PlusMinus (&iValue, 484, iY, 0, 31, -1, 0);
							PlusMinus (&iValue, 554, iY, 0, 31, +1, 0);
							PlusMinus (&iValue, 569, iY, 0, 31, +10, 0);
							switch (iRGBLoop)
							{
								case 1: iEXEHairR = iValue; break;
								case 2: iEXEHairG = iValue; break;
								case 3: iEXEHairB = iValue; break;
								case 4: iEXESkinR = iValue; break;
								case 5: iEXESkinG = iValue; break;
								case 6: iEXESkinB = iValue; break;
								case 7: iEXESuitR = iValue; break;
								case 8: iEXESuitG = iValue; break;
								case 9: iEXESuitB = iValue; break;
							}
						}
					}
					ShowEXE();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowEXE(); } break;
				case SDL_QUIT:
					Quit(); break;
				case SDL_TEXTINPUT:
					if (strlen (arIntroSlides[iS][iL]) < SLIDES_LINES_CHARS - 1)
					{
						cAddChar = toupper (event.text.text[0]);
						if (((cAddChar >= 'A') && (cAddChar <= 'Z')) || (cAddChar == ' '))
						{
							snprintf (sLineTemp, SLIDES_LINES_CHARS, "%s",
								arIntroSlides[iS][iL]);
							/*** Added "+1" for \0. ***/
							snprintf (arIntroSlides[iS][iL], SLIDES_LINES_CHARS,
								"%s%c", sLineTemp, cAddChar);
							IntroSlides();
							PlaySound ("wav/hum_adj.wav");
						}
					}
					ShowEXE();
					break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen();
}
/*****************************************************************************/
void ShowEXE (void)
/*****************************************************************************/
{
	SDL_Color clr;
	int iR, iG, iB;
	int iX, iY, iBaseX, iBaseY;
	int iLetter;

	/*** Used for looping. ***/
	int iSlideLoop;
	int iLineLoop;
	int iCharLoop;

	/*** background ***/
	ShowImage (imgexe, 0, 0, "imgexe");

	/*** save button ***/
	switch (iEXESave)
	{
		case 0: /*** off ***/
			ShowImage (imgsave[1], 590, 523, "imgsave[1]"); break;
		case 1: /*** on ***/
			ShowImage (imgsave[2], 590, 523, "imgsave[2]"); break;
	}

	/*** Starting minutes left. ***/
	if (iEXEMinutesLeft == 60) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXEMinutesLeft, 247, 34, clr, 0);

	/*** Starting hit points. ***/
	if (iEXEHitPoints == 3) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXEHitPoints, 247, 58, clr, 0);

	/*** Hair. ***/
	if (iEXEHairR == 24) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXEHairR, 497, 34, clr, 0);
	if (iEXEHairG == 21) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXEHairG, 497, 58, clr, 0);
	if (iEXEHairB == 0) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXEHairB, 497, 82, clr, 0);

	/*** Skin. ***/
	if (iEXESkinR == 20) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESkinR, 497, 116, clr, 0);
	if (iEXESkinG == 13) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESkinG, 497, 140, clr, 0);
	if (iEXESkinB == 11) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESkinB, 497, 164, clr, 0);

	/*** Suit. ***/
	if (iEXESuitR == 31) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESuitR, 497, 198, clr, 0);
	if (iEXESuitG == 31) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESuitG, 497, 222, clr, 0);
	if (iEXESuitB == 31) { clr = color_bl; } else { clr = color_blue; }
	CenterNumber (iEXESuitB, 497, 246, clr, 0);

	/*** Fill the prince with approximate(!) colors. ***/
	/*** Hair. ***/
	iR = (iEXEHairR * 255) / 31;
	iG = (iEXEHairG * 255) / 31;
	iB = (iEXEHairB * 255) / 31;
	ColorRect (614, 49, 12, 8, iR, iG, iB);
	ColorRect (610, 53, 8, 16, iR, iG, iB);
	/*** Skin. ***/
	iR = (iEXESkinR * 255) / 31;
	iG = (iEXESkinG * 255) / 31;
	iB = (iEXESkinB * 255) / 31;
	ColorRect (618, 57, 8, 12, iR, iG, iB);
	ColorRect (614, 69, 8, 4, iR, iG, iB);
	ColorRect (610, 77, 8, 20, iR, iG, iB);
	ColorRect (614, 97, 8, 4, iR, iG, iB);
	ColorRect (618, 101, 8, 4, iR, iG, iB);
	ColorRect (622, 105, 4, 4, iR, iG, iB);
	ColorRect (614, 157, 12, 8, iR, iG, iB);
	ColorRect (626, 161, 4, 4, iR, iG, iB);
	/*** Suit. ***/
	iR = (iEXESuitR * 255) / 31;
	iG = (iEXESuitG * 255) / 31;
	iB = (iEXESuitB * 255) / 31;
	ColorRect (610, 73, 12, 4, iR, iG, iB);
	ColorRect (618, 77, 8, 20, iR, iG, iB);
	ColorRect (622, 97, 4, 4, iR, iG, iB);
	ColorRect (610, 97, 4, 24, iR, iG, iB);
	ColorRect (614, 101, 4, 56, iR, iG, iB);
	ColorRect (618, 105, 4, 52, iR, iG, iB);
	ColorRect (622, 109, 4, 44, iR, iG, iB);
	ColorRect (626, 117, 4, 24, iR, iG, iB);

	/*** Intro slides. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		switch (iSlideLoop)
		{
			case 1: iBaseX = 34; iBaseY = 191; break;
			case 2: iBaseX = 34; iBaseY = 295; break;
			case 3: iBaseX = 266; iBaseY = 295; break;
			case 4: iBaseX = 34; iBaseY = 399; break;
			case 5: iBaseX = 266; iBaseY = 399; break;
		}
		iBaseX+=14;
		iBaseY+=18;
		iY = iBaseY;
		for (iLineLoop = 1; iLineLoop <= SLIDES_LINES; iLineLoop++)
		{
			iX = iBaseX;
			for (iCharLoop = 0; iCharLoop <
				(int)strlen ((char *)arIntroSlides[iSlideLoop]
				[iLineLoop]); iCharLoop++)
			{
				iLetter = arIntroSlides[iSlideLoop][iLineLoop][iCharLoop] - 0x40;
				if (iLetter == -32)
				{
					ShowImage (imgspace, iX, iY, "imgspace");
				} else if ((iLetter >= 1) && (iLetter <= 26)) {
					ShowImage (imgalphabet[iLetter], iX, iY, "imgalphabet[...]");
				} else {
					ShowImage (imgunknown, iX, iY, "imgunknown");
				}
				iX+=16;
			}
			iY+=16;
		}
	}

	/*** Selected intro slide text line. ***/
	switch (iS)
	{
		case 1:
			switch (iL)
			{
				case 1: iX = 34; iY = 209; break;
				case 2: iX = 34; iY = 225; break;
				case 3: iX = 34; iY = 241; break;
				case 4: iX = 34; iY = 257; break;
			}
			break;
		case 2:
			switch (iL)
			{
				case 1: iX = 34; iY = 313; break;
				case 2: iX = 34; iY = 329; break;
				case 3: iX = 34; iY = 345; break;
				case 4: iX = 34; iY = 361; break;
			}
			break;
		case 3:
			switch (iL)
			{
				case 1: iX = 266; iY = 313; break;
				case 2: iX = 266; iY = 329; break;
				case 3: iX = 266; iY = 345; break;
				case 4: iX = 266; iY = 361; break;
			}
			break;
		case 4:
			switch (iL)
			{
				case 1: iX = 34; iY = 417; break;
				case 2: iX = 34; iY = 433; break;
				case 3: iX = 34; iY = 449; break;
				case 4: iX = 34; iY = 465; break;
			}
			break;
		case 5:
			switch (iL)
			{
				case 1: iX = 266; iY = 417; break;
				case 2: iX = 266; iY = 433; break;
				case 3: iX = 266; iY = 449; break;
				case 4: iX = 266; iY = 465; break;
			}
			break;
	}
	if (iS != 0) { ShowImage (imgseltextline, iX, iY, "imgseltextline"); }

	/*** Show the number of free bytes. ***/
	if (iBytesLeft < 0)
	{
		clr = color_red;
		ShowImage (imgexewarning, 265 - 9, 249, "imgexewarning");
	} else {
		clr = color_bl;
	}
	CenterNumber (iBytesLeft, 265, 249, clr, 0);

	/*** If necessary, show warnings about slide sizes. ***/
	if (arSlideSizes[1] != 29)
	{
		ShowImage (imgexewarning, 116 - 9, 265, "imgexewarning");
		CenterNumber (29 - arSlideSizes[1], 116, 265, color_red, 0);
	}
	if (arSlideSizes[2] != 24)
	{
		ShowImage (imgexewarning, 116 - 9, 369, "imgexewarning");
		CenterNumber (24 - arSlideSizes[2], 116, 369, color_red, 0);
	}
	if (arSlideSizes[3] != 37)
	{
		ShowImage (imgexewarning, 348 - 9, 369, "imgexewarning");
		CenterNumber (37 - arSlideSizes[3], 348, 369, color_red, 0);
	}
	if (arSlideSizes[4] != 31)
	{
		ShowImage (imgexewarning, 116 - 9, 473, "imgexewarning");
		CenterNumber (31 - arSlideSizes[4], 116, 473, color_red, 0);
	}
	if (arSlideSizes[5] != 29)
	{
		ShowImage (imgexewarning, 348 - 9, 473, "imgexewarning");
		CenterNumber (29 - arSlideSizes[5], 348, 473, color_red, 0);
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void InitScreenAction (char *sAction)
/*****************************************************************************/
{
	int iEventRoom;
	int iToTile;
	int iFromTile;

	if (strcmp (sAction, "left") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelected--;
				switch (iSelected)
				{
					case 0: iSelected = 10; break;
					case 10: iSelected = 20; break;
					case 20: iSelected = 30; break;
				}
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 1) { iMovingNewX--; }
							else { iMovingNewX = 25; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 1)
					{
						iChangingBrokenSide = 1;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 4; break;
							case 5: iChangingBrokenRoom = 8; break;
							case 9: iChangingBrokenRoom = 12; break;
							case 13: iChangingBrokenRoom = 16; break;
							case 17: iChangingBrokenRoom = 20; break;
							case 21: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom--; break;
						}
					}
				}
				break;
			case 3:
				iToTile = arEventsToTile[iCurLevel][iChangeEvent];
				switch (iToTile)
				{
					case 1: iToTile = 10; break;
					case 11: iToTile = 20; break;
					case 21: iToTile = 30; break;
					default: iToTile--; break;
				}
				if (iToTile > 20) { EventTile (iToTile - 20, 3, 1); }
					else if (iToTile > 10)
						{ EventTile (iToTile - 10, 2, 1); }
					else { EventTile (iToTile, 1, 1); }
				break;
		}
	}

	if (strcmp (sAction, "right") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelected++;
				switch (iSelected)
				{
					case 11: iSelected = 1; break;
					case 21: iSelected = 11; break;
					case 31: iSelected = 21; break;
				}
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 25) { iMovingNewX++; }
							else { iMovingNewX = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 2)
					{
						iChangingBrokenSide = 2;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 4: iChangingBrokenRoom = 1; break;
							case 8: iChangingBrokenRoom = 5; break;
							case 12: iChangingBrokenRoom = 9; break;
							case 16: iChangingBrokenRoom = 13; break;
							case 20: iChangingBrokenRoom = 17; break;
							case 24: iChangingBrokenRoom = 21; break;
							default: iChangingBrokenRoom++; break;
						}
					}
				}
				break;
			case 3:
				iToTile = arEventsToTile[iCurLevel][iChangeEvent];
				switch (iToTile)
				{
					case 10: iToTile = 1; break;
					case 20: iToTile = 11; break;
					case 30: iToTile = 21; break;
					default: iToTile++; break;
				}
				if (iToTile > 20) { EventTile (iToTile - 20, 3, 1); }
					else if (iToTile > 10)
						{ EventTile (iToTile - 10, 2, 1); }
					else { EventTile (iToTile, 1, 1); }
				break;
		}
	}

	if (strcmp (sAction, "up") == 0)
	{
		switch (iScreen)
		{
			case 1:
				if (iSelected > 10) { iSelected-=10; }
					else { iSelected+=20; }
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 1) { iMovingNewY--; }
							else { iMovingNewY = 24; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 3)
					{
						iChangingBrokenSide = 3;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 21; break;
							case 2: iChangingBrokenRoom = 22; break;
							case 3: iChangingBrokenRoom = 23; break;
							case 4: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom -= 4; break;
						}
					}
				}
				break;
			case 3:
				iToTile = arEventsToTile[iCurLevel][iChangeEvent];
				if (iToTile > 10) { iToTile-=10; }
					else { iToTile+=20; }
				if (iToTile > 20) { EventTile (iToTile - 20, 3, 1); }
					else if (iToTile > 10)
						{ EventTile (iToTile - 10, 2, 1); }
					else { EventTile (iToTile, 1, 1); }
				break;
		}
	}

	if (strcmp (sAction, "down") == 0)
	{
		switch (iScreen)
		{
			case 1:
				if (iSelected <= 20) { iSelected+=10; }
					else { iSelected-=20; }
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 24) { iMovingNewY++; }
							else { iMovingNewY = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 4)
					{
						iChangingBrokenSide = 4;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 21: iChangingBrokenRoom = 1; break;
							case 22: iChangingBrokenRoom = 2; break;
							case 23: iChangingBrokenRoom = 3; break;
							case 24: iChangingBrokenRoom = 4; break;
							default: iChangingBrokenRoom += 4; break;
						}
					}
				}
				break;
			case 3:
				iToTile = arEventsToTile[iCurLevel][iChangeEvent];
				if (iToTile <= 20) { iToTile+=10; }
					else { iToTile-=20; }
				if (iToTile > 20) { EventTile (iToTile - 20, 3, 1); }
					else if (iToTile > 10)
						{ EventTile (iToTile - 10, 2, 1); }
					else { EventTile (iToTile, 1, 1); }
				break;
		}
	}

	if (strcmp (sAction, "left bracket") == 0)
	{
		switch (iScreen)
		{
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					iMovingNewBusy = 0;
					switch (iMovingRoom)
					{
						case 0: iMovingRoom = ROOMS; break; /*** If disabled. ***/
						case 1: iMovingRoom = ROOMS; break;
						default: iMovingRoom--; break;
					}
				}
				break;
			case 3:
				iEventRoom = arEventsToRoom[iCurLevel][iChangeEvent];
				if ((iEventRoom >= 2) && (iEventRoom <= 24))
				{
					EventRoom (iEventRoom - 1, 1);
				} else {
					EventRoom (24, 1);
				}
				break;
		}
	}

	if (strcmp (sAction, "right bracket") == 0)
	{
		switch (iScreen)
		{
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					iMovingNewBusy = 0;
					switch (iMovingRoom)
					{
						case 0: iMovingRoom = 1; break; /*** If disabled. ***/
						case 24: iMovingRoom = 1; break;
						default: iMovingRoom++; break;
					}
				}
				break;
			case 3:
				iEventRoom = arEventsToRoom[iCurLevel][iChangeEvent];
				if ((iEventRoom >= 1) && (iEventRoom <= 23))
				{
					EventRoom (iEventRoom + 1, 1);
				} else {
					EventRoom (1, 1);
				}
				break;
		}
	}

	if (strcmp (sAction, "left brace") == 0)
	{
		if (iScreen == 3)
		{
			iEventRoom = arEventsFromRoom[iCurLevel][iChangeEvent];
			if ((iEventRoom >= 2) && (iEventRoom <= 24))
			{
				EventRoom (iEventRoom - 1, 0);
			} else {
				EventRoom (24, 0);
			}
		}
	}

	if (strcmp (sAction, "right brace") == 0)
	{
		if (iScreen == 3)
		{
			iEventRoom = arEventsFromRoom[iCurLevel][iChangeEvent];
			if ((iEventRoom >= 1) && (iEventRoom <= 23))
			{
				EventRoom (iEventRoom + 1, 0);
			} else {
				EventRoom (1, 0);
			}
		}
	}

	if (strcmp (sAction, "enter") == 0)
	{
		switch (iScreen)
		{
			case 1:
				ChangePos();
				break;
			case 2:
				if (arBrokenRoomLinks[iCurLevel] == 0)
				{
					if (iMovingRoom != 0)
					{
						if (arMovingRooms[iMovingNewX][iMovingNewY] == 0)
						{
							RemoveOldRoom();
							AddNewRoom (iMovingNewX, iMovingNewY, iMovingRoom);
							iChanged++;
						}
						iMovingRoom = 0; iMovingNewBusy = 0;
					}
				} else {
					LinkPlus();
				}
				break;
			case 3:
				if (iController == 1) /*** Else it uses brackets. ***/
				{
					iEventRoom = arEventsToRoom[iCurLevel][iChangeEvent];
					if ((iEventRoom >= 1) && (iEventRoom <= 23))
					{
						EventRoom (iEventRoom + 1, 1);
					} else {
						EventRoom (1, 1);
					}
				}
				break;
		}
	}

	if (strcmp (sAction, "env") == 0)
	{
/***
	switch (cCurType)
	{
		case 'd': cCurType = 'p'; break;
		case 'p': cCurType = 'd'; break;
	}
	PlaySound ("wav/extras.wav");
***/
	}

	if (strcmp (sAction, "left from") == 0)
	{
		if (iScreen == 3)
		{
			iFromTile = arEventsFromTile[iCurLevel][iChangeEvent];
			switch (iFromTile)
			{
				case 1: iFromTile = 10; break;
				case 11: iFromTile = 20; break;
				case 21: iFromTile = 30; break;
				default: iFromTile--; break;
			}
			if (iFromTile > 20) { EventTile (iFromTile - 20, 3, 0); }
				else if (iFromTile > 10)
					{ EventTile (iFromTile - 10, 2, 0); }
				else { EventTile (iFromTile, 1, 0); }
		}
	}

	if (strcmp (sAction, "right from") == 0)
	{
		if (iScreen == 3)
		{
			iFromTile = arEventsFromTile[iCurLevel][iChangeEvent];
			switch (iFromTile)
			{
				case 10: iFromTile = 1; break;
				case 20: iFromTile = 11; break;
				case 30: iFromTile = 21; break;
				default: iFromTile++; break;
			}
			if (iFromTile > 20) { EventTile (iFromTile - 20, 3, 0); }
				else if (iFromTile > 10)
					{ EventTile (iFromTile - 10, 2, 0); }
				else { EventTile (iFromTile, 1, 0); }
		}
	}

	if (strcmp (sAction, "up from") == 0)
	{
		if (iScreen == 3)
		{
			iFromTile = arEventsFromTile[iCurLevel][iChangeEvent];
			if (iFromTile > 10) { iFromTile-=10; }
				else { iFromTile+=20; }
			if (iFromTile > 20) { EventTile (iFromTile - 20, 3, 0); }
				else if (iFromTile > 10)
					{ EventTile (iFromTile - 10, 2, 0); }
				else { EventTile (iFromTile, 1, 0); }
		}
	}

	if (strcmp (sAction, "down from") == 0)
	{
		if (iScreen == 3)
		{
			iFromTile = arEventsFromTile[iCurLevel][iChangeEvent];
			if (iFromTile <= 20) { iFromTile+=10; }
				else { iFromTile-=20; }
			if (iFromTile > 20) { EventTile (iFromTile - 20, 3, 0); }
				else if (iFromTile > 10)
					{ EventTile (iFromTile - 10, 2, 0); }
				else { EventTile (iFromTile, 1, 0); }
		}
	}
}
/*****************************************************************************/
void RunLevel (int iLevel)
/*****************************************************************************/
{
	SDL_Thread *princethread;

	if (iDebug == 1)
	{
		printf ("[  OK  ] Starting the game in level %i.\n", iLevel);
	}

	ModifyForMednafen (iLevel);

	princethread = SDL_CreateThread (StartGame, "StartGame", NULL);
	if (princethread == NULL)
	{
		printf ("[FAILED] Could not create thread!\n");
		exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
int StartGame (void *unused)
/*****************************************************************************/
{
	char sSystem[200 + 2];
	char sSound[200 + 2];

	if (unused != NULL) { } /*** To prevent warnings. ***/

	PlaySound ("wav/mednafen.wav");

	switch (iNoAudio)
	{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
		case 0: snprintf (sSound, 200, "%s", " -sound 1"); break;
#else
		case 0: snprintf (sSound, 200, "%s", " -sound 1 -sounddriver sdl"); break;
#endif
		case 1: snprintf (sSound, 200, "%s", " -sound 0"); break;
	}

	snprintf (sSystem, 200, "mednafen %s %s > %s",
		sSound, sPathFile, DEVNULL);
	if (system (sSystem) == -1)
	{
		printf ("[ WARN ] Could not execute mednafen!\n");
	}
	if (iModified == 1) { ModifyBack(); }

	return (EXIT_NORMAL);
}
/*****************************************************************************/
void ClearRoom (void)
/*****************************************************************************/
{
	int iTileLoop;

	/*** Remove tiles. ***/
	for (iTileLoop = 1; iTileLoop <= 30; iTileLoop++)
	{
		SetLocation (iCurRoom, iTileLoop, 0x00);
	}

	/*** Remove guard. ***/
	arGuardTile[iCurLevel][iCurRoom] = TILES + 1;

	PlaySound ("wav/ok_close.wav");
	iChanged++;
}
/*****************************************************************************/
void UseTile (int iTile, int iLocation, int iRoom)
/*****************************************************************************/
{
	/*** Do not use iSelected in this function. ***/

	/*** Random tile. ***/
	if (iTile == -1)
	{
		do {
			iTile = 1 + (int) (60.0 * rand() / (RAND_MAX + 1.0));
		} while (Unused (iTile) == 1);
	}

	/*** Custom tile. ***/
	if (iTile == -2)
	{
		SetLocation (iRoom, iLocation, iCustomTile);
	}

	/*** Make sure the disabled living can't be used. ***/
	if ((iCurLevel != 3) && ((iTile == 65) || (iTile == 66))) { return; }
	if ((iCurLevel != 12) && ((iTile == 67) || (iTile == 68))) { return; }
	if ((iCurLevel != 13) && ((iTile == 69) || (iTile == 70))) { return; }
	if (((iCurLevel == 3) || (iCurLevel == 12) || (iCurLevel == 13)) &&
		((iTile == 63) || (iTile == 64))) { return; }

	switch (iTile)
	{
		/*** Row 1. ***/
		case 1: SetLocation (iRoom, iLocation, 0x00); break;
		case 2: SetLocation (iRoom, iLocation, 0xE0); break;
		case 3: SetLocation (iRoom, iLocation, 0x20); break;
		case 4: SetLocation (iRoom, iLocation, 0x40); break;
		case 5: SetLocation (iRoom, iLocation, 0x60); break;
		case 6: SetLocation (iRoom, iLocation, 0x01); break;
		case 7: SetLocation (iRoom, iLocation, 0xE1); break;
		case 8: SetLocation (iRoom, iLocation, 0x21); break;
		case 9: SetLocation (iRoom, iLocation, 0x1F); break;
		case 10: SetLocation (iRoom, iLocation, 0x0E); break;

		/*** Row 2. ***/
		case 11: SetLocation (iRoom, iLocation, 0x02); break;
		case 12: SetLocation (iRoom, iLocation, 0x12); break;
		/***/
		case 17: SetLocation (iRoom, iLocation, 0x17); break;
		case 18: SetLocation (iRoom, iLocation, 0x0D); break;
		case 19: SetLocation (iRoom, iLocation, 0x05); break;
		/***/

		/*** Row 3. ***/
		case 21: SetLocation (iRoom, iLocation, 0x14); break;
		case 22: SetLocation (iRoom, iLocation, 0x34); break;
		case 23: SetLocation (iRoom, iLocation, 0x03); break;
		case 24: SetLocation (iRoom, iLocation, 0x08); break;
		case 25: SetLocation (iRoom, iLocation, 0x09); break;
		case 26: SetLocation (iRoom, iLocation, 0x10); break;
		case 27: SetLocation (iRoom, iLocation, 0x50); break;
		case 28: SetLocation (iRoom, iLocation, 0x11); break;
		/***/
		case 30: SetLocation (iRoom, iLocation, 0x13); break;

		/*** Row 4. ***/
		case 31: SetLocation (iRoom, iLocation, 0x2A); break;
		case 32: SetLocation (iRoom, iLocation, 0x4A); break;
		case 33: SetLocation (iRoom, iLocation, 0x6A); break;
		case 34: SetLocation (iRoom, iLocation, 0x8A); break;
		case 35: SetLocation (iRoom, iLocation, 0xAA); break;
		case 36: SetLocation (iRoom, iLocation, 0x0A); break;
		/***/
		/***/
		case 39: SetLocation (iRoom, iLocation, 0x0B); break;
		case 40: SetLocation (iRoom, iLocation, 0x2B); break;

		/*** Row 5. ***/
		case 41: SetLocation (iRoom, iLocation, 0x16); break;
		/***/
		case 43: SetLocation (iRoom, iLocation, 0x44); break;
		case 44: SetLocation (iRoom, iLocation, 0x24); break;
		case 45: SetLocation (iRoom, iLocation, 0x0C); break;
		case 46: SetLocation (iRoom, iLocation, 0x2C); break;
		case 47: SetLocation (iRoom, iLocation, 0x4C); break;
		case 48: SetLocation (iRoom, iLocation, 0x27); break;
		case 49: SetLocation (iRoom, iLocation, 0x47); break;
		case 50: SetLocation (iRoom, iLocation, 0x07); break;

		/*** Row 6. ***/
		case 51: SetLocation (iRoom, iLocation, 0x19); break;
		case 52: SetLocation (iRoom, iLocation, 0x1A); break;
		case 53: SetLocation (iRoom, iLocation, 0x1B); break;
		case 54: SetLocation (iRoom, iLocation, 0x1C); break;
		case 55: SetLocation (iRoom, iLocation, 0x1D); break;
		case 56: SetLocation (iRoom, iLocation, 0x18); break;
		case 57: SetLocation (iRoom, iLocation, 0x41); break;
		case 58: SetLocation (iRoom, iLocation, 0x15); break;
		case 59: SetLocation (iRoom, iLocation,
			RaiseDropEvent (0x0F, iChangeEvent, 1)); break;
		case 60: SetLocation (iRoom, iLocation,
			RaiseDropEvent (0x06, iChangeEvent, 1)); break;
		case 61: /*** prince, turned right ***/
			if ((arStartLocation[iCurLevel][1] != iCurRoom) ||
				(arStartLocation[iCurLevel][2] != iLocation) ||
				(arStartLocation[iCurLevel][3] != 0x00))
			{
				arStartLocation[iCurLevel][1] = iCurRoom;
				arStartLocation[iCurLevel][2] = iLocation;
				arStartLocation[iCurLevel][3] = 0x00;
				PlaySound ("wav/hum_adj.wav");
			}
			break;
		case 62: /*** prince, turned left ***/
			if ((arStartLocation[iCurLevel][1] != iCurRoom) ||
				(arStartLocation[iCurLevel][2] != iLocation) ||
				(arStartLocation[iCurLevel][3] != 0xFF))
			{
				arStartLocation[iCurLevel][1] = iCurRoom;
				arStartLocation[iCurLevel][2] = iLocation;
				arStartLocation[iCurLevel][3] = 0xFF;
				PlaySound ("wav/hum_adj.wav");
			}
			break;
		case 63: case 65: case 67: case 69: /*** living, turned right ***/
			if ((arGuardTile[iCurLevel][iCurRoom] == iLocation) &&
				(arGuardDir[iCurLevel][iCurRoom] == 0x00))
			{
				arGuardTile[iCurLevel][iCurRoom] = TILES + 1;
			} else {
				arGuardTile[iCurLevel][iCurRoom] = iLocation;
				arGuardDir[iCurLevel][iCurRoom] = 0x00;
				PlaySound ("wav/hum_adj.wav");
			}
			break;
		case 64: case 66: case 68: case 70: /*** living, turned left ***/
			if ((arGuardTile[iCurLevel][iCurRoom] == iLocation) &&
				(arGuardDir[iCurLevel][iCurRoom] == 0xFF))
			{
				arGuardTile[iCurLevel][iCurRoom] = TILES + 1;
			} else {
				arGuardTile[iCurLevel][iCurRoom] = iLocation;
				arGuardDir[iCurLevel][iCurRoom] = 0xFF;
				PlaySound ("wav/hum_adj.wav");
			}
			break;
	}
}
/*****************************************************************************/
void Zoom (int iToggleFull)
/*****************************************************************************/
{
	if (iToggleFull == 1)
	{
		if (iFullscreen == 0)
		{ iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP; }
			else { iFullscreen = 0; }
	} else {
		if (iFullscreen == SDL_WINDOW_FULLSCREEN_DESKTOP)
		{
			iFullscreen = 0;
			iScale = 1;
		} else if (iScale == 1) {
			iScale = 2;
		} else if (iScale == 2) {
			iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else {
			printf ("[ WARN ] Unknown window state!\n");
		}
	}

	SDL_SetWindowFullscreen (window, iFullscreen);
	SDL_SetWindowSize (window, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED);
	TTF_CloseFont (font1);
	TTF_CloseFont (font2);
	TTF_CloseFont (font3);
	LoadFonts();
}
/*****************************************************************************/
void LinkMinus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide];
	if (iCurrent > ROOMS) /*** "?"; high links ***/
	{
		iNew = 0;
	} else if (iCurrent == 0) {
		iNew = ROOMS;
	} else {
		iNew = iCurrent - 1;
	}
	arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
int BrokenRoomLinks (int iPrint)
/*****************************************************************************/
{
	int iBroken;
	int iRoomLoop;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		arDone[iRoomLoop] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][1] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][2] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][3] = 0;
		arRoomConnectionsBroken[iCurLevel][iRoomLoop][4] = 0;
	}
	CheckSides (arStartLocation[iCurLevel][1], 0, 0);
	iBroken = 0;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		/*** If the room is in use... ***/
		if (arDone[iRoomLoop] == 1)
		{
			/*** check left ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][1] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][1] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][1]][2]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][1] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][1] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The left of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check right ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][2] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][2] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][2]][1]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][2] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][2] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The right of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check up ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][3] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][3] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][3]][4]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][3] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][3] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The top of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
			/*** check down ***/
			if (arRoomLinks[iCurLevel][iRoomLoop][4] != 0)
			{
				if ((arRoomLinks[iCurLevel][iRoomLoop][4] == iRoomLoop) ||
					(arRoomLinks[iCurLevel][arRoomLinks[iCurLevel][iRoomLoop][4]][3]
					!= iRoomLoop) ||
					(arRoomLinks[iCurLevel][iRoomLoop][4] > ROOMS))
				{
					arRoomConnectionsBroken[iCurLevel][iRoomLoop][4] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The bottom of room %i has a broken link.\n",
							iRoomLoop);
					}
				}
			}
		}
	}

	return (iBroken);
}
/*****************************************************************************/
void ChangeEvent (int iAmount, int iChangePos)
/*****************************************************************************/
{
	int iTile;
	int iHighNibble, iLowNibble;

	if (((iAmount > 0) && (iChangeEvent != arNrEvents[iCurLevel])) ||
		((iAmount < 0) && (iChangeEvent > 1)))
	{
		/*** Modify the event number. ***/
		iChangeEvent+=iAmount;
		if (iChangeEvent < 1) { iChangeEvent = 1; }
		if (iChangeEvent > arNrEvents[iCurLevel])
			{ iChangeEvent = arNrEvents[iCurLevel]; }

		/*** Apply the event number to the selected tile. ***/
		if (iChangePos == 1)
		{
			iTile = arRoomTiles[iCurLevel][iCurRoom][iSelected];
			iHighNibble = iTile >> 4;
			iLowNibble = iTile & 0x0F; /*** 0F = 00001111 ***/
			if ((iLowNibble == 0x0F) && (IsEven (iHighNibble))) /*** Raise. ***/
			{
				arRoomTiles[iCurLevel][iCurRoom][iSelected] =
					RaiseDropEvent (0x0F, iChangeEvent, iAmount);
				iChanged++;
			}
			if ((iLowNibble == 0x06) && (IsEven (iHighNibble))) /*** Drop. ***/
			{
				arRoomTiles[iCurLevel][iCurRoom][iSelected] =
					RaiseDropEvent (0x06, iChangeEvent, iAmount);
				iChanged++;
			}
		}

		PlaySound ("wav/plus_minus.wav");
	}
}
/*****************************************************************************/
void ChangeCustom (int iAmount)
/*****************************************************************************/
{
	if (((iAmount > 0) && (iCustomTile < 0xFF)) ||
		((iAmount < 0) && (iCustomTile > 0x00)))
	{
		/*** Modify the custom tile. ***/
		iCustomTile+=iAmount;
		if (iCustomTile < 0x00) { iCustomTile = 0x00; }
		if (iCustomTile > 0xFF) { iCustomTile = 0xFF; }

		PlaySound ("wav/plus_minus.wav");
	}
}
/*****************************************************************************/
void Prev (void)
/*****************************************************************************/
{
	if (iCurLevel != 1)
	{
		iCurLevel--;
		LoadLevels(); iChanged = 0; /*** Both are to discard changes. ***/
		iCurRoom = arStartLocation[iCurLevel][1];
		iChangeEvent = 1;
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
void Next (void)
/*****************************************************************************/
{
	if (iCurLevel != LEVELS)
	{
		iCurLevel++;
		LoadLevels(); iChanged = 0; /*** Both are to discard changes. ***/
		iCurRoom = arStartLocation[iCurLevel][1];
		iChangeEvent = 1;
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
void CallSave (void)
/*****************************************************************************/
{
	CreateBAK();
	SaveLevels();
}
/*****************************************************************************/
void Sprinkle (void)
/*****************************************************************************/
{
	int iRandom;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			/*** space? add wall shadow ***/
			if ((arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] == 0x00) ||
				(arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] == 0xE0))
			{
				/*** 1-4 ***/
				iRandom = 1 + (int) (4.0 * rand() / (RAND_MAX + 1.0));
				switch (iRandom)
				{
					case 1: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x20; break;
					case 2: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x40; break;
				}
			}

			/*** empty floor? add wall shadow, rubble, torch, skeleton ***/
			if ((arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] == 0x01) ||
				(arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] == 0xE1))
			{
				/*** 1-8 ***/
				iRandom = 1 + (int) (8.0 * rand() / (RAND_MAX + 1.0));
				switch (iRandom)
				{
					case 1: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x21; break;
					case 2: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x0E; break;
					case 3: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x13; break;
					case 4: arRoomTiles[iCurLevel][iRoomLoop][iTileLoop] = 0x15; break;
				}
			}
		}
	}
}
/*****************************************************************************/
void SetLocation (int iRoom, int iLocation, int iTile)
/*****************************************************************************/
{
	arRoomTiles[iCurLevel][iRoom][iLocation] = iTile;
	iLastTile = iTile;
}
/*****************************************************************************/
void FlipRoom (int iAxis)
/*****************************************************************************/
{
	unsigned char arRoomTilesTemp[TILES + 2];
	int iTileUse;
	int iTile;

	/*** Used for looping. ***/
	int iTileLoop;

	/*** Storing tiles for later use. ***/
	for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
	{
		arRoomTilesTemp[iTileLoop] = arRoomTiles[iCurLevel][iCurRoom][iTileLoop];
	}

	if (iAxis == 1) /*** horizontal ***/
	{
		/*** tiles ***/
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			iTileUse = 0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
				{ iTileUse = 11 - iTileLoop; }
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
				{ iTileUse = 31 - iTileLoop; }
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
				{ iTileUse = 51 - iTileLoop; }
			arRoomTiles[iCurLevel][iCurRoom][iTileLoop] = arRoomTilesTemp[iTileUse];
		}

		/*** prince ***/
		if (arStartLocation[iCurLevel][1] == iCurRoom)
		{
			/*** direction ***/
			if (arStartLocation[iCurLevel][3] == 0x00)
				{ arStartLocation[iCurLevel][3] = 0xFF; }
					else { arStartLocation[iCurLevel][3] = 0x00; }
			/*** tile ***/
			iTile = arStartLocation[iCurLevel][2];
			if ((iTile >= 1) && (iTile <= 10))
				{ arStartLocation[iCurLevel][2] = 11 - iTile; }
			if ((iTile >= 11) && (iTile <= 20))
				{ arStartLocation[iCurLevel][2] = 31 - iTile; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arStartLocation[iCurLevel][2] = 51 - iTile; }
		}

		/*** guard ***/
		if (arGuardTile[iCurLevel][iCurRoom] <= TILES + 1)
		{
			/*** direction ***/
			if (arGuardDir[iCurLevel][iCurRoom] == 0x00)
				{ arGuardDir[iCurLevel][iCurRoom] = 0xFF; }
					else { arGuardDir[iCurLevel][iCurRoom] = 0x00; }
			/*** tile ***/
			iTile = arGuardTile[iCurLevel][iCurRoom];
			if ((iTile >= 1) && (iTile <= 10))
				{ arGuardTile[iCurLevel][iCurRoom] = 11 - iTile; }
			if ((iTile >= 11) && (iTile <= 20))
				{ arGuardTile[iCurLevel][iCurRoom] = 31 - iTile; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arGuardTile[iCurLevel][iCurRoom] = 51 - iTile; }
		}
	} else { /*** vertical ***/
		/*** tiles ***/
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			iTileUse = 0; /*** To prevent warnings. ***/
			if ((iTileLoop >= 1) && (iTileLoop <= 10))
				{ iTileUse = iTileLoop + 20; }
			if ((iTileLoop >= 11) && (iTileLoop <= 20))
				{ iTileUse = iTileLoop; }
			if ((iTileLoop >= 21) && (iTileLoop <= 30))
				{ iTileUse = iTileLoop - 20; }
			arRoomTiles[iCurLevel][iCurRoom][iTileLoop] =
				arRoomTilesTemp[iTileUse];
		}

		/*** prince ***/
		if (arStartLocation[iCurLevel][1] == iCurRoom)
		{
			/*** tile ***/
			iTile = arStartLocation[iCurLevel][2];
			if ((iTile >= 1) && (iTile <= 10))
				{ arStartLocation[iCurLevel][2] = iTile + 20; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arStartLocation[iCurLevel][2] = iTile - 20; }
		}

		/*** guard ***/
		if (arGuardTile[iCurLevel][iCurRoom] <= TILES + 1)
		{
			/*** tile ***/
			iTile = arGuardTile[iCurLevel][iCurRoom];
			if ((iTile >= 1) && (iTile <= 10))
				{ arGuardTile[iCurLevel][iCurRoom] = iTile + 20; }
			if ((iTile >= 21) && (iTile <= 30))
				{ arGuardTile[iCurLevel][iCurRoom] = iTile - 20; }
		}
	}
}
/*****************************************************************************/
void CopyPaste (int iAction)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iTileLoop;

	if (iAction == 1) /*** copy ***/
	{
		for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
		{
			arCopyPasteTile[iTileLoop] = arRoomTiles[iCurLevel][iCurRoom][iTileLoop];
		}
		cCopyPasteGuardTile = arGuardTile[iCurLevel][iCurRoom];
		cCopyPasteGuardDir = arGuardDir[iCurLevel][iCurRoom];
		iCopied = 1;
	} else { /*** paste ***/
		if (iCopied == 1)
		{
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				arRoomTiles[iCurLevel][iCurRoom][iTileLoop] =
					arCopyPasteTile[iTileLoop];
			}
			arGuardTile[iCurLevel][iCurRoom] = cCopyPasteGuardTile;
			arGuardDir[iCurLevel][iCurRoom] = cCopyPasteGuardDir;
		} else {
			for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
			{
				arRoomTiles[iCurLevel][iCurRoom][iTileLoop] = 0x00;
			}
			arGuardTile[iCurLevel][iCurRoom] = TILES + 1;
		}
	}
}
/*****************************************************************************/
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY)
/*****************************************************************************/
{
	if ((iUpperLeftX * iScale <= iXPos) &&
		(iLowerRightX * iScale >= iXPos) &&
		(iUpperLeftY * iScale <= iYPos) &&
		(iLowerRightY * iScale >= iYPos))
	{
		return (1);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int MouseSelectAdj (void)
/*****************************************************************************/
{
	/* On the broken room links screen, if you click one of the adjacent
	 * rooms, this function sets iOnAdj to 1, and also sets both
	 * iChangingBrokenRoom and iChangingBrokenSide.
	 */

	int iOnAdj;
	int iRoomLoop;
	int iAdjBaseX;
	int iAdjBaseY;

	iOnAdj = 0;
	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		switch (iRoomLoop)
		{
			case 1: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 2: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 3: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 4: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 5: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 6: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 7: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 8: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 9: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 10: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 11: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 12: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 13: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 14: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 15: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 16: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 17: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 18: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 19: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 20: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 21: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 22: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 23: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 24: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			default:
				printf ("[FAILED] iRoomLoop is not in the 1-24 range!\n");
				exit (EXIT_ERROR);
		}
		if (InArea (iAdjBaseX + 1, iAdjBaseY + 16,
			iAdjBaseX + 15, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 1; /*** left ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 31, iAdjBaseY + 16,
			iAdjBaseX + 45, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 2; /*** right ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 1,
			iAdjBaseX + 30, iAdjBaseY + 14) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 3; /*** up ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 31,
			iAdjBaseX + 30, iAdjBaseY + 45) == 1)
		{
			iChangingBrokenRoom = iRoomLoop;
			iChangingBrokenSide = 4; /*** down ***/
			iOnAdj = 1;
		}
	}

	return (iOnAdj);
}
/*****************************************************************************/
int OnLevelBar (void)
/*****************************************************************************/
{
	if (InArea (28, 3, 602, 22) == 1) { return (1); } else { return (0); }
}
/*****************************************************************************/
void ChangePos (void)
/*****************************************************************************/
{
	int iChanging;
	SDL_Event event;
	int iOldXPos, iOldYPos;
	int iUseTile;
	int iNowOn;

	/*** Used for looping. ***/
	int iRoomLoop;
	int iTileLoop;

	iChanging = 1;
	iEventTooltip = 0;
	iCustomHover = 0;

	ShowChange();
	while (iChanging == 1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							if (iOnTile != 0)
							{
								UseTile (iOnTile, iSelected, iCurRoom);
								if (iOnTile <= 60) { iChanging = 0; }
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_B:
							iChanging = 0; break;
						case SDL_CONTROLLER_BUTTON_X:
							UseTile (-2, iSelected, iCurRoom);
							iChanging = 0;
							iChanged++;
							break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							ChangePosAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							ChangePosAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							ChangePosAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							ChangePosAction ("down"); break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
							ChangeCustom (-1); break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
							ChangeCustom (1); break;
					}
					ShowChange();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iYJoy1 = SDL_JoystickGetAxis (joystick, 1);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					iYJoy2 = SDL_JoystickGetAxis (joystick, 4);
					if ((iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((SDL_GetTicks() - joyleft) > 300)
						{
							ChangeEvent (-1, 1);
							joyleft = SDL_GetTicks();
						}
					}
					if ((iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((SDL_GetTicks() - joyright) > 300)
						{
							ChangeEvent (1, 1);
							joyright = SDL_GetTicks();
						}
					}
					if ((iYJoy1 < -30000) || (iYJoy2 < -30000)) /*** up ***/
					{
						if ((SDL_GetTicks() - joyup) > 300)
						{
							ChangeEvent (10, 1);
							joyup = SDL_GetTicks();
						}
					}
					if ((iYJoy1 > 30000) || (iYJoy2 > 30000)) /*** down ***/
					{
						if ((SDL_GetTicks() - joydown) > 300)
						{
							ChangeEvent (-10, 1);
							joydown = SDL_GetTicks();
						}
					}
					ShowChange();
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								if ((iOnTile >= 1) && (iOnTile <= 60))
								{
									for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
									{
										for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
											{ UseTile (iOnTile, iTileLoop, iRoomLoop); }
									}
									iChanging = 0;
									iChanged++;
								}
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								if ((iOnTile >= 1) && (iOnTile <= 60))
								{
									for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
										{ UseTile (iOnTile, iTileLoop, iCurRoom); }
									iChanging = 0;
									iChanged++;
								}
							} else if (iOnTile != 0) {
								UseTile (iOnTile, iSelected, iCurRoom);
								if (iOnTile <= 60) { iChanging = 0; }
								iChanged++;
							}
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
						case SDLK_c:
							iChanging = 0; break;
						case SDLK_LEFT:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								ChangeEvent (-10, 1);
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								ChangeEvent (-1, 1);
							} else {
								ChangePosAction ("left");
							}
							break;
						case SDLK_RIGHT:
							if (event.key.keysym.mod & KMOD_CTRL)
							{
								ChangeEvent (10, 1);
							} else if (event.key.keysym.mod & KMOD_SHIFT) {
								ChangeEvent (1, 1);
							} else {
								ChangePosAction ("right");
							}
							break;
						case SDLK_UP: ChangePosAction ("up"); break;
						case SDLK_DOWN: ChangePosAction ("down"); break;
						default: break;
					}
					ShowChange();
					break;
				case SDL_MOUSEMOTION:
					iOldXPos = iXPos;
					iOldYPos = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iOldXPos == iXPos) && (iOldYPos == iYPos)) { break; }

					/*** event tooltip ***/
					iEventTooltipOld = iEventTooltip;
					if (InArea (525, 539, 525 + 126, 539 + 29) == 1)
						{ iEventTooltip = 1; } else { iEventTooltip = 0; }
					if (iEventTooltip != iEventTooltipOld) { ShowChange(); }

					/*** custom hover ***/
					iCustomHoverOld = iCustomHover;
					if (InArea (395, 491, 395 + 126, 491 + 47) == 1)
						{ iCustomHover = 1; } else { iCustomHover = 0; }
					if (iCustomHover != iCustomHoverOld) { ShowChange(); }

					iNowOn = OnTile();
					if ((iOnTile != iNowOn) && (iNowOn != 0))
					{
						if (IsDisabled (iNowOn) == 0)
						{
							iOnTile = iNowOn;
							ShowChange();
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1) /*** left mouse button ***/
					{
						if (InArea (656, 0, 656 + 36, 0 + 573) == 1) /*** close ***/
						{
							iCloseOn = 1;
							ShowChange();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iCloseOn = 0;

					/*** On tile or living. ***/
					iUseTile = 0;
					if ((InArea (2, 2, 2 + 652, 2 + 488) == 1) ||
						(InArea (2, 488, 2 + 327, 488 + 83) == 1)) { iUseTile = 1; }

					/*** On the custom tile area. ***/
					if (InArea (395, 491, 395 + 126, 491 + 47) == 1)
					{
						iOnTile = -2;
						iUseTile = 1;
					}

					if (event.button.button == 1) /*** left mouse button ***/
					{
						/*** Changing the custom tile. ***/
						if (InArea (402, 543, 402 + 13, 543 + 20) == 1)
							{ ChangeCustom (-16); }
						if (InArea (417, 543, 417 + 13, 543 + 20) == 1)
							{ ChangeCustom (-1); }
						if (InArea (487, 543, 487 + 13, 543 + 20) == 1)
							{ ChangeCustom (1); }
						if (InArea (502, 543, 502 + 13, 543 + 20) == 1)
							{ ChangeCustom (16); }

						/*** Changing the event number. ***/
						if (InArea (532, 543, 532 + 13, 543 + 20) == 1)
							{ ChangeEvent (-10, 1); }
						if (InArea (547, 543, 547 + 13, 543 + 20) == 1)
							{ ChangeEvent (-1, 1); }
						if (InArea (617, 543, 617 + 13, 543 + 20) == 1)
							{ ChangeEvent (1, 1); }
						if (InArea (632, 543, 632 + 13, 543 + 20) == 1)
							{ ChangeEvent (10, 1); }

						/*** On close. ***/
						if (InArea (656, 0, 656 + 36, 0 + 573) == 1) { iChanging = 0; }

						if (iUseTile == 1)
						{
							UseTile (iOnTile, iSelected, iCurRoom);
							if (iOnTile <= 60) { iChanging = 0; }
							iChanged++;
						}
					}

					if (event.button.button == 2)
					{
						if ((iUseTile == 1) && (iOnTile != 0) && (iOnTile <= 60))
						{
							for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
							{
								UseTile (iOnTile, iTileLoop, iCurRoom);
							}
							iChanging = 0;
							iChanged++;
						}
					}

					if (event.button.button == 3)
					{
						if ((iUseTile == 1) && (iOnTile != 0) && (iOnTile <= 60))
						{
							for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
							{
								for (iTileLoop = 1; iTileLoop <= TILES; iTileLoop++)
								{
									UseTile (iOnTile, iTileLoop, iRoomLoop);
								}
							}
							iChanging = 0;
							iChanged++;
						}
					}

					ShowChange();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowChange(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/ok_close.wav");
}
/*****************************************************************************/
void RemoveOldRoom (void)
/*****************************************************************************/
{
	arMovingRooms[iMovingOldX][iMovingOldY] = 0;

	/* Change the links of the rooms around
	 * the removed room.
	 */

	/*** left of removed ***/
	if ((iMovingOldX >= 2) && (iMovingOldX <= 24))
	{
		if (arMovingRooms[iMovingOldX - 1][iMovingOldY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX - 1]
				[iMovingOldY]][2] = 0; /*** remove right ***/
		}
	}

	/*** right of removed ***/
	if ((iMovingOldX >= 1) && (iMovingOldX <= 23))
	{
		if (arMovingRooms[iMovingOldX + 1][iMovingOldY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX + 1]
				[iMovingOldY]][1] = 0; /*** remove left ***/
		}
	}

	/*** above removed ***/
	if ((iMovingOldY >= 2) && (iMovingOldY <= 24))
	{
		if (arMovingRooms[iMovingOldX][iMovingOldY - 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX]
				[iMovingOldY - 1]][4] = 0; /*** remove below ***/
		}
	}

	/*** below removed ***/
	if ((iMovingOldY >= 1) && (iMovingOldY <= 23))
	{
		if (arMovingRooms[iMovingOldX][iMovingOldY + 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iMovingOldX]
				[iMovingOldY + 1]][3] = 0; /*** remove above ***/
		}
	}
}
/*****************************************************************************/
void AddNewRoom (int iX, int iY, int iRoom)
/*****************************************************************************/
{
	arMovingRooms[iX][iY] = iRoom;

	/* Change the links of the rooms around
	 * the new room and the room itself.
	 */

	arRoomLinks[iCurLevel][iRoom][1] = 0;
	arRoomLinks[iCurLevel][iRoom][2] = 0;
	arRoomLinks[iCurLevel][iRoom][3] = 0;
	arRoomLinks[iCurLevel][iRoom][4] = 0;

	if ((iX >= 2) && (iX <= 24)) /*** left of added ***/
	{
		if (arMovingRooms[iX - 1][iY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX - 1]
				[iY]][2] = iRoom; /*** add room right ***/
			arRoomLinks[iCurLevel][iRoom][1] = arMovingRooms[iX - 1][iY];
		}
	}

	if ((iX >= 1) && (iX <= 23)) /*** right of added ***/
	{
		if (arMovingRooms[iX + 1][iY] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX + 1]
				[iY]][1] = iRoom; /*** add room left ***/
			arRoomLinks[iCurLevel][iRoom][2] = arMovingRooms[iX + 1][iY];
		}
	}

	if ((iY >= 2) && (iY <= 24)) /*** above added ***/
	{
		if (arMovingRooms[iX][iY - 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX]
				[iY - 1]][4] = iRoom; /*** add room below ***/
			arRoomLinks[iCurLevel][iRoom][3] = arMovingRooms[iX][iY - 1];
		}
	}

	if ((iY >= 1) && (iY <= 23)) /*** below added ***/
	{
		if (arMovingRooms[iX][iY + 1] != 0)
		{
			arRoomLinks[iCurLevel][arMovingRooms[iX]
				[iY + 1]][3] = iRoom; /*** add room above ***/
			arRoomLinks[iCurLevel][iRoom][4] = arMovingRooms[iX][iY + 1];
		}
	}

	PlaySound ("wav/move_room.wav");
}
/*****************************************************************************/
void LinkPlus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide];
	if (iCurrent > ROOMS) /*** "?"; high links ***/
	{
		iNew = 0;
	} else if (iCurrent == ROOMS) {
		iNew = 0;
	} else {
		iNew = iCurrent + 1;
	}
	arRoomLinks[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	arBrokenRoomLinks[iCurLevel] = BrokenRoomLinks (0);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
void EventRoom (int iRoom, int iFromTo)
/*****************************************************************************/
{
	switch (iFromTo)
	{
		case 0: arEventsFromRoom[iCurLevel][iChangeEvent] = iRoom; break;
		case 1: arEventsToRoom[iCurLevel][iChangeEvent] = iRoom; break;
	}
	PlaySound ("wav/check_box.wav");
	iChanged++;
}
/*****************************************************************************/
void EventTile (int iX, int iY, int iFromTo)
/*****************************************************************************/
{
	int iTile;

	iTile = ((iY - 1) * 10) + iX;
	switch (iFromTo)
	{
		case 0: arEventsFromTile[iCurLevel][iChangeEvent] = iTile; break;
		case 1: arEventsToTile[iCurLevel][iChangeEvent] = iTile; break;
	}
	PlaySound ("wav/check_box.wav");
	iChanged++;
}
/*****************************************************************************/
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo)
/*****************************************************************************/
{
	SDL_Rect dest;
	SDL_Rect loc;
	int iWidth, iHeight;
	int iInfoC;
	char arText[9 + 2][MAX_TEXT + 2];
	int iHighNibble, iLowNibble;
	int iTileValue;
	int iGreen;
	char sTileValue[MAX_OPTION + 2];

	iInfoC = 0;

	iGreen = 1;
	if (strncmp (sImageInfo, "tile=", 5) == 0)
	{
		GetOptionValue (sImageInfo, sTileValue);
		iTileValue = atoi (sTileValue);
	} else if (strncmp (sImageInfo, "high=", 5) == 0) {
		iGreen = 2;
		GetOptionValue (sImageInfo, sTileValue);
		iTileValue = atoi (sTileValue);
	} else {
		iTileValue = -1;
	}

	if (iInfo == 1)
	{
		snprintf (arText[0], MAX_TEXT, "0x%02x", iTileValue);
	}

	/*** Raise and drop buttons. ***/
	if (iTileValue != -1)
	{
		iHighNibble = iTileValue >> 4;
		iLowNibble = iTileValue & 0x0F; /*** 0F = 00001111 ***/

		if ((iLowNibble == 0x0F) && (IsEven (iHighNibble))) /*** Raise. ***/
		{
			switch (cCurType)
			{
				case 'd': img = imgd[0x0F][iGreen]; break;
				case 'p': img = imgp[0x0F][iGreen]; break;
			}
			if (iInfo != 1)
				{ snprintf (arText[0], MAX_TEXT, "N:%i", iHighNibble); }
			iInfoC = 1;
		}

		if ((iLowNibble == 0x06) && (IsEven (iHighNibble))) /*** Drop. ***/
		{
			switch (cCurType)
			{
				case 'd': img = imgd[0x06][iGreen]; break;
				case 'p': img = imgp[0x06][iGreen]; break;
			}
			if (iInfo != 1)
				{ snprintf (arText[0], MAX_TEXT, "N:%i", iHighNibble); }
			iInfoC = 1;
		}
	}

	/*** Custom tile. ***/
	if ((iInfoC == 0) && (imgd[iTileValue][1] == NULL))
	{
		snprintf (arText[0], MAX_TEXT, "0x%02X", iTileValue);
		iInfoC = 1;
	}

	if ((iNoAnim == 0) && (strcmp (sImageInfo, "tile=19") == 0))
	{
		switch (cCurType)
		{
			case 'd': img = imgspriteflamed; break;
			case 'p': img = imgspriteflamep; break;
		}
	}
	SDL_QueryTexture (img, NULL, NULL, &iWidth, &iHeight);
	loc.x = 0;
	loc.y = 0;
	loc.w = iWidth;
	loc.h = iHeight;
	dest.x = iX;
	dest.y = iY;
	dest.w = iWidth;
	dest.h = iHeight;
	if ((iNoAnim == 0) && (strcmp (sImageInfo, "tile=19") == 0))
	{
		loc.x = (iFlameFrame - 1) * 130;
		loc.w = loc.w / 4;
		dest.w = dest.w / 4;
	}
	CustomRenderCopy (img, &loc, &dest, sImageInfo);

	/*** Info ("i"). ***/
	if ((iTileValue != -1) && ((iInfo == 1) || (iInfoC == 1)))
	{
		DisplayText (dest.x, dest.y + 162 - 12 - FONT_SIZE_11,
			FONT_SIZE_11, arText, 1, font2);
	}
}
/*****************************************************************************/
void CustomRenderCopy (SDL_Texture* src, SDL_Rect* srcrect,
	SDL_Rect *dstrect, char *sImageInfo)
/*****************************************************************************/
{
	SDL_Rect stuff;

	stuff.x = dstrect->x * iScale;
	stuff.y = dstrect->y * iScale;
	if (srcrect != NULL) /*** image ***/
	{
		stuff.w = dstrect->w * iScale;
		stuff.h = dstrect->h * iScale;
	} else { /*** font ***/
		stuff.w = dstrect->w;
		stuff.h = dstrect->h;
	}
	if (SDL_RenderCopy (ascreen, src, srcrect, &stuff) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (%s): %s!\n",
			sImageInfo, SDL_GetError());
	}
}
/*****************************************************************************/
void CreateBAK (void)
/*****************************************************************************/
{
	FILE *fDAT;
	FILE *fBAK;
	int iData;

	fDAT = fopen (sPathFile, "rb");
	if (fDAT == NULL)
		{ printf ("[FAILED] Could not open %s: %s!\n",
			sPathFile, strerror (errno)); }

	fBAK = fopen (BACKUP, "wb");
	if (fBAK == NULL)
		{ printf ("[FAILED] Could not open %s: %s!\n",
			BACKUP, strerror (errno)); }

	while (1)
	{
		iData = fgetc (fDAT);
		if (iData == EOF) { break; }
			else { putc (iData, fBAK); }
	}

	fclose (fDAT);
	fclose (fBAK);
}
/*****************************************************************************/
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font)
/*****************************************************************************/
{
	int iTemp;

	for (iTemp = 0; iTemp <= (iLines - 1); iTemp++)
	{
		if (strcmp (arText[iTemp], "") != 0)
		{
			message = TTF_RenderText_Shaded (font,
				arText[iTemp], color_bl, color_wh);
			messaget = SDL_CreateTextureFromSurface (ascreen, message);
			if ((strcmp (arText[iTemp], "single tile (change or select)") == 0) ||
				(strcmp (arText[iTemp], "entire room (clear or fill)") == 0) ||
				(strcmp (arText[iTemp], "entire level (randomize or fill)") == 0))
			{
				offset.x = iStartX + 20;
			} else {
				offset.x = iStartX;
			}
			offset.y = iStartY + (iTemp * (iFontSize + 4));
			offset.w = message->w; offset.h = message->h;
			CustomRenderCopy (messaget, NULL, &offset, "message");
			SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
		}
	}
}
/*****************************************************************************/
void InitRooms (void)
/*****************************************************************************/
{
	int iRoomLoop;
	int iRoomLoop2;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS + 1; iRoomLoop++) /*** x ***/
	{
		for (iRoomLoop2 = 1; iRoomLoop2 <= ROOMS; iRoomLoop2++) /*** y ***/
		{
			arMovingRooms[iRoomLoop][iRoomLoop2] = 0;
		}
	}
}
/*****************************************************************************/
void WhereToStart (void)
/*****************************************************************************/
{
	int iRoomLoop;

	iMinX = 0;
	iMaxX = 0;
	iMinY = 0;
	iMaxY = 0;

	for (iRoomLoop = 1; iRoomLoop <= ROOMS; iRoomLoop++)
	{
		arDone[iRoomLoop] = 0;
	}
	CheckSides (arStartLocation[iCurLevel][1], 0, 0);

	iStartRoomsX = round (12 - (((float)iMinX + (float)iMaxX) / 2));
	iStartRoomsY = round (12 - (((float)iMinY + (float)iMaxY) / 2));
}
/*****************************************************************************/
void CheckSides (int iRoom, int iX, int iY)
/*****************************************************************************/
{
	if (iX < iMinX) { iMinX = iX; }
	if (iY < iMinY) { iMinY = iY; }
	if (iX > iMaxX) { iMaxX = iX; }
	if (iY > iMaxY) { iMaxY = iY; }

	arDone[iRoom] = 1;

	if ((arRoomLinks[iCurLevel][iRoom][1] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][1]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][1], iX - 1, iY); }

	if ((arRoomLinks[iCurLevel][iRoom][2] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][2]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][2], iX + 1, iY); }

	if ((arRoomLinks[iCurLevel][iRoom][3] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][3]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][3], iX, iY - 1); }

	if ((arRoomLinks[iCurLevel][iRoom][4] != 0) &&
		(arDone[arRoomLinks[iCurLevel][iRoom][4]] != 1))
		{ CheckSides (arRoomLinks[iCurLevel][iRoom][4], iX, iY + 1); }
}
/*****************************************************************************/
void ShowRooms (int iRoom, int iX, int iY, int iNext)
/*****************************************************************************/
{
	int iShowX, iShowY;

	if (iX == 25) /*** side pane ***/
	{
		iShowX = 272;
	} else {
		iShowX = 282 + (iX * 15); /*** grid, 24x24 ***/
	}
	iShowY = 46 + (iY * 15); /*** grid, 24x24 & pane ***/

	if (iRoom != -1)
	{
		ShowImage (imgroom[iRoom], iShowX, iShowY, "imgroom[...]");
		arMovingRooms[iX][iY] = iRoom; /*** save room location ***/
		if (iCurRoom == iRoom)
		{
			ShowImage (imgsrc, iShowX, iShowY, "imgsrc"); /*** green stripes ***/
		}
		if (arStartLocation[iCurLevel][1] == iRoom)
		{
			ShowImage (imgsrs, iShowX, iShowY, "imgsrs"); /*** blue border ***/
		}
		if (iMovingRoom == iRoom)
		{
			ShowImage (imgsrm, iShowX, iShowY, "imgsrm"); /*** red stripes ***/
		}
	} else {
		ShowImage (imgsrp, iShowX, iShowY, "imgsrp"); /*** white cross ***/
	}
	if (iRoom == iMovingRoom)
	{
		iMovingOldX = iX;
		iMovingOldY = iY;
		if (iMovingNewBusy == 0)
		{
			iMovingNewX = iMovingOldX;
			iMovingNewY = iMovingOldY;
			iMovingNewBusy = 1;
		}
	}

	arDone[iRoom] = 1;

	if (iNext == 1)
	{
		if ((arRoomLinks[iCurLevel][iRoom][1] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][1]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][1], iX - 1, iY, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][2] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][2]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][2], iX + 1, iY, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][3] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][3]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][3], iX, iY - 1, 1); }

		if ((arRoomLinks[iCurLevel][iRoom][4] != 0) &&
			(arDone[arRoomLinks[iCurLevel][iRoom][4]] != 1))
			{ ShowRooms (arRoomLinks[iCurLevel][iRoom][4], iX, iY + 1, 1); }
	}
}
/*****************************************************************************/
void BrokenRoomChange (int iRoom, int iSide, int *iX, int *iY)
/*****************************************************************************/
{
	switch (iSide)
	{
		case 0:
			*iX = BROKEN_ROOM_X;
			*iY = BROKEN_ROOM_Y;
			break;
		case 1:
			*iX = BROKEN_LEFT_X;
			*iY = BROKEN_LEFT_Y;
			break;
		case 2:
			*iX = BROKEN_RIGHT_X;
			*iY = BROKEN_RIGHT_Y;
			break;
		case 3:
			*iX = BROKEN_UP_X;
			*iY = BROKEN_UP_Y;
			break;
		case 4:
			*iX = BROKEN_DOWN_X;
			*iY = BROKEN_DOWN_Y;
			break;
	}

	switch (iRoom)
	{
		case 1: *iX += (63 * 0); *iY += (63 * 0); break;
		case 2: *iX += (63 * 1); *iY += (63 * 0); break;
		case 3: *iX += (63 * 2); *iY += (63 * 0); break;
		case 4: *iX += (63 * 3); *iY += (63 * 0); break;
		case 5: *iX += (63 * 0); *iY += (63 * 1); break;
		case 6: *iX += (63 * 1); *iY += (63 * 1); break;
		case 7: *iX += (63 * 2); *iY += (63 * 1); break;
		case 8: *iX += (63 * 3); *iY += (63 * 1); break;
		case 9: *iX += (63 * 0); *iY += (63 * 2); break;
		case 10: *iX += (63 * 1); *iY += (63 * 2); break;
		case 11: *iX += (63 * 2); *iY += (63 * 2); break;
		case 12: *iX += (63 * 3); *iY += (63 * 2); break;
		case 13: *iX += (63 * 0); *iY += (63 * 3); break;
		case 14: *iX += (63 * 1); *iY += (63 * 3); break;
		case 15: *iX += (63 * 2); *iY += (63 * 3); break;
		case 16: *iX += (63 * 3); *iY += (63 * 3); break;
		case 17: *iX += (63 * 0); *iY += (63 * 4); break;
		case 18: *iX += (63 * 1); *iY += (63 * 4); break;
		case 19: *iX += (63 * 2); *iY += (63 * 4); break;
		case 20: *iX += (63 * 3); *iY += (63 * 4); break;
		case 21: *iX += (63 * 0); *iY += (63 * 5); break;
		case 22: *iX += (63 * 1); *iY += (63 * 5); break;
		case 23: *iX += (63 * 2); *iY += (63 * 5); break;
		case 24: *iX += (63 * 3); *iY += (63 * 5); break;
	}
}
/*****************************************************************************/
void ShowChange (void)
/*****************************************************************************/
{
	int iX, iY;
	int iOldTile;
	int iHighNibble, iLowNibble;

	/*** background ***/
	switch (cCurType)
	{
		case 'd': ShowImage (imgdungeon, 0, 0, "imgdungeon"); break;
		case 'p': ShowImage (imgpalace, 0, 0, "imgpalace"); break;
	}

	/*** close button ***/
	switch (iCloseOn)
	{
		case 0: /*** off ***/
			ShowImage (imgclosebig_0, 656, 0, "imgclosebig_0"); break;
		case 1: /*** on ***/
			ShowImage (imgclosebig_1, 656, 0, "imgclosebig_1"); break;
	}

	DisableSome();

	/*** old tile ***/
	iOldTile = arRoomTiles[iCurLevel][iCurRoom][iSelected];
	iHighNibble = iOldTile >> 4;
	iLowNibble = iOldTile & 0x0F; /*** 0F = 00001111 ***/
	if ((iLowNibble == 0x0F) && (IsEven (iHighNibble))) /*** Raise. ***/
	{
		iOldTile = 0x0F;
		iChangeEvent = iHighNibble;
	} else if ((iLowNibble == 0x06) && (IsEven (iHighNibble))) { /*** Drop. ***/
		iOldTile = 0x06;
		iChangeEvent = iHighNibble;
	}
	iX = -1; iY = -1;
	switch (iOldTile)
	{
		/*** Row 1. ***/
		case 0x00: iX = TILESX1; iY = TILESY1; break;
		case 0xE0: iX = TILESX2; iY = TILESY1; break;
		case 0x20: iX = TILESX3; iY = TILESY1; break;
		case 0x40: iX = TILESX4; iY = TILESY1; break;
		case 0x60: iX = TILESX5; iY = TILESY1; break;
		case 0x01: iX = TILESX6; iY = TILESY1; break;
		case 0xE1: iX = TILESX7; iY = TILESY1; break;
		case 0x21: iX = TILESX8; iY = TILESY1; break;
		case 0x1F: iX = TILESX9; iY = TILESY1; break;
		case 0x0E: iX = TILESX10; iY = TILESY1; break;

		/*** Row 2. ***/
		case 0x02: iX = TILESX1; iY = TILESY2; break;
		case 0x12: iX = TILESX2; iY = TILESY2; break;
		/***/
		case 0x17: iX = TILESX7; iY = TILESY2; break;
		case 0x0D: iX = TILESX8; iY = TILESY2; break;
		case 0x05: iX = TILESX9; iY = TILESY2; break;
		/***/

		/*** Row 3. ***/
		case 0x14: iX = TILESX1; iY = TILESY3; break;
		case 0x34: iX = TILESX2; iY = TILESY3; break;
		case 0x03: iX = TILESX3; iY = TILESY3; break;
		case 0x08: iX = TILESX4; iY = TILESY3; break;
		case 0x09: iX = TILESX5; iY = TILESY3; break;
		case 0x10: iX = TILESX6; iY = TILESY3; break;
		case 0x50: iX = TILESX7; iY = TILESY3; break;
		case 0x11: iX = TILESX8; iY = TILESY3; break;
		/***/
		case 0x13: iX = TILESX10; iY = TILESY3; break;

		/*** Row 4. ***/
		case 0x2A: iX = TILESX1; iY = TILESY4; break;
		case 0x4A: iX = TILESX2; iY = TILESY4; break;
		case 0x6A: iX = TILESX3; iY = TILESY4; break;
		case 0x8A: iX = TILESX4; iY = TILESY4; break;
		case 0xAA: iX = TILESX5; iY = TILESY4; break;
		case 0x0A: iX = TILESX6; iY = TILESY4; break;
		/***/
		/***/
		case 0x0B: iX = TILESX9; iY = TILESY4; break;
		case 0x2B: iX = TILESX10; iY = TILESY4; break;

		/*** Row 5. ***/
		case 0x16: iX = TILESX1; iY = TILESY5; break;
		/***/
		case 0x44: iX = TILESX3; iY = TILESY5; break;
		case 0x24: iX = TILESX4; iY = TILESY5; break;
		case 0x0C: iX = TILESX5; iY = TILESY5; break;
		case 0x2C: iX = TILESX6; iY = TILESY5; break;
		case 0x4C: iX = TILESX7; iY = TILESY5; break;
		case 0x27: iX = TILESX8; iY = TILESY5; break;
		case 0x47: iX = TILESX9; iY = TILESY5; break;
		case 0x07: iX = TILESX10; iY = TILESY5; break;

		/*** Row 6. ***/
		case 0x19: iX = TILESX1; iY = TILESY6; break;
		case 0x1A: iX = TILESX2; iY = TILESY6; break;
		case 0x1B: iX = TILESX3; iY = TILESY6; break;
		case 0x1C: iX = TILESX4; iY = TILESY6; break;
		case 0x1D: iX = TILESX5; iY = TILESY6; break;
		case 0x18: iX = TILESX6; iY = TILESY6; break;
		case 0x41: iX = TILESX7; iY = TILESY6; break;
		case 0x15: iX = TILESX8; iY = TILESY6; break;
		case 0x0F: iX = TILESX9; iY = TILESY6; break;
		case 0x06: iX = TILESX10; iY = TILESY6; break;
	}
	if ((iX != -1) && (iY != -1))
	{
		ShowImage (imgborderbl, iX, iY, "imgborderbl");
	}

	/*** prince ***/
	if ((iCurRoom == arStartLocation[iCurLevel][1]) &&
		(iSelected == arStartLocation[iCurLevel][2]))
	{
		switch (arStartLocation[iCurLevel][3])
		{
			case 0x00: /*** r ***/
				ShowImage (imgbordersl, 2, 488, "imgbordersl"); break;
			case 0xFF: /*** l ***/
				ShowImage (imgbordersl, 35, 488, "imgbordersl"); break;
		}
	}

	/*** guard ***/
	if (iSelected == arGuardTile[iCurLevel][iCurRoom])
	{
		iY = 488;
		switch (arGuardDir[iCurLevel][iCurRoom])
		{
			case 0x00: /*** r ***/
				if (iCurLevel == 3)
				{
					iX = 132;
				} else if (iCurLevel == 12) {
					iX = 197;
				} else if (iCurLevel == 13) {
					iX = 262;
				} else {
					iX = 67;
				}
				break;
			case 0xFF: /*** l ***/
				if (iCurLevel == 3)
				{
					iX = 165;
				} else if (iCurLevel == 12) {
					iX = 230;
				} else if (iCurLevel == 13) {
					iX = 295;
				} else {
					iX = 100;
				}
				break;
		}
		ShowImage (imgbordersl, iX, iY, "imgbordersl");
	}

	/*** selected (new) tile ***/
	if ((iOnTile != 0) && (IsDisabled (iOnTile) == 0))
	{
		if (iOnTile <= 60) /*** (large) tiles ***/
		{
			iX = 2 + (((iOnTile - 1) % 10) * (TILEWIDTH + 2));
			iY = 2 + (((int)((iOnTile - 1) / 10)) * (TILEHEIGHT + 2));
			ShowImage (imgborderb, iX, iY, "imgborderb");
		} else { /*** living ***/
			switch (iOnTile)
			{
				case 61: iX = 2; break;
				case 62: iX = 35; break;
				case 63: iX = 67; break;
				case 64: iX = 100; break;
				case 65: iX = 132; break;
				case 66: iX = 165; break;
				case 67: iX = 197; break;
				case 68: iX = 230; break;
				case 69: iX = 262; break;
				case 70: iX = 295; break;
			}
			iY = 488;
			ShowImage (imgborders, iX, iY, "imgborders");
		}
	}

	/*** custom tile ***/
	CenterNumber (iCustomTile, 430, 543, color_bl, 1);

	/*** event number ***/
	CenterNumber (iChangeEvent, 560, 542, color_bl, 0);

	if (iEventTooltip == 1)
		{ ShowImage (imgetooltip, 525, 491, "imgetooltip"); }

	if (iCustomHover == 1)
		{ ShowImage (imgchover, 395, 491, "imgchover"); }

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
int OnTile (void)
/*****************************************************************************/
{
	int iTempX;
	int iTempY;
	int iTempOn;

	/*** (large) tiles ***/
	for (iTempY = 0; iTempY < 6; iTempY++)
	{
		for (iTempX = 0; iTempX < 10; iTempX++)
		{
			if (InArea (4 + (iTempX * (TILEWIDTH + 2)),
				4 + (iTempY * (TILEHEIGHT + 2)),
				2 + ((iTempX + 1) * (TILEWIDTH + 2)),
				2 + ((iTempY + 1) * (TILEHEIGHT + 2))) == 1)
			{
				iTempOn = (iTempY * 10) + iTempX + 1;
				if ((iTempOn >= 1) && (iTempOn <= 60)) { return (iTempOn); }
			}
		}
	}

	/*** living ***/
	iTempY = 490;
	if (InArea (4, iTempY, 4 + 30, iTempY + 79) == 1) { return (61); }
	if (InArea (37, iTempY, 37 + 30, iTempY + 79) == 1) { return (62); }
	if (InArea (69, iTempY, 69 + 30, iTempY + 79) == 1) { return (63); }
	if (InArea (102, iTempY, 102 + 30, iTempY + 79) == 1) { return (64); }
	if (InArea (134, iTempY, 134 + 30, iTempY + 79) == 1) { return (65); }
	if (InArea (167, iTempY, 167 + 30, iTempY + 79) == 1) { return (66); }
	if (InArea (199, iTempY, 199 + 30, iTempY + 79) == 1) { return (67); }
	if (InArea (232, iTempY, 232 + 30, iTempY + 79) == 1) { return (68); }
	if (InArea (264, iTempY, 264 + 30, iTempY + 79) == 1) { return (69); }
	if (InArea (297, iTempY, 297 + 30, iTempY + 79) == 1) { return (70); }

	return (0);
}
/*****************************************************************************/
void ChangePosAction (char *sAction)
/*****************************************************************************/
{
	if (strcmp (sAction, "left") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 1: iOnTile = 10; break;
				case 11: iOnTile = 20; break;
				case 21: iOnTile = 30; break;
				case 31: iOnTile = 40; break;
				case 41: iOnTile = 50; break;
				case 51: iOnTile = 60; break;
				case 61: iOnTile = 70; break;
				default: iOnTile--; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "right") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 10: iOnTile = 1; break;
				case 20: iOnTile = 11; break;
				case 30: iOnTile = 21; break;
				case 40: iOnTile = 31; break;
				case 50: iOnTile = 41; break;
				case 60: iOnTile = 51; break;
				case 70: iOnTile = 61; break;
				default: iOnTile++; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "up") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 1: iOnTile = 61; break;
				case 2: iOnTile = 63; break;
				case 3: iOnTile = 65; break;
				case 4: iOnTile = 67; break;
				case 5: iOnTile = 69; break;
				case 6: iOnTile = 56; break;
				case 7: iOnTile = 57; break;
				case 8: iOnTile = 58; break;
				case 9: iOnTile = 59; break;
				case 10: iOnTile = 60; break;
				case 61: case 62: iOnTile = 51; break;
				case 63: case 64: iOnTile = 52; break;
				case 65: case 66: iOnTile = 53; break;
				case 67: case 68: iOnTile = 54; break;
				case 69: case 70: iOnTile = 55; break;
				default: iOnTile-=10; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}

	if (strcmp (sAction, "down") == 0)
	{
		do {
			switch (iOnTile)
			{
				case 51: iOnTile = 61; break;
				case 52: iOnTile = 63; break;
				case 53: iOnTile = 65; break;
				case 54: iOnTile = 67; break;
				case 55: iOnTile = 69; break;
				case 56: iOnTile = 6; break;
				case 57: iOnTile = 7; break;
				case 58: iOnTile = 8; break;
				case 59: iOnTile = 9; break;
				case 60: iOnTile = 10; break;
				case 61: case 62: iOnTile = 1; break;
				case 63: case 64: iOnTile = 2; break;
				case 65: case 66: iOnTile = 3; break;
				case 67: case 68: iOnTile = 4; break;
				case 69: case 70: iOnTile = 5; break;
				default: iOnTile+=10; break;
			}
		} while (IsDisabled (iOnTile) == 1);
	}
}
/*****************************************************************************/
void DisableSome (void)
/*****************************************************************************/
{
	if (iCurLevel != 3)
	{
		/*** disable skeleton ***/
		ShowImage (imgdisabled, 132, 488, "imgdisabled");
	}

	if (iCurLevel != 12)
	{
		/*** disable shadow ***/
		ShowImage (imgdisabled, 197, 488, "imgdisabled");
	}

	if (iCurLevel != 13)
	{
		/*** disable Jaffar ***/
		ShowImage (imgdisabled, 262, 488, "imgdisabled");
	}

	if ((iCurLevel == 3) || (iCurLevel == 12) || (iCurLevel == 13))
	{
		/*** disable guard ***/
		ShowImage (imgdisabled, 67, 488, "imgdisabled");
	}
}
/*****************************************************************************/
int IsDisabled (int iTile)
/*****************************************************************************/
{
	/*** skeleton ***/
	if ((iCurLevel != 3) && ((iTile == 65) || (iTile == 66)))
		{ return (1); }

	/*** shadow ***/
	if ((iCurLevel != 12) && ((iTile == 67) || (iTile == 68)))
		{ return (1); }

	/*** Jaffar ***/
	if ((iCurLevel != 13) && ((iTile == 69) || (iTile == 70)))
		{ return (1); }

	/*** guard ***/
	if (((iCurLevel == 3) || (iCurLevel == 12) || (iCurLevel == 13)) &&
		((iTile == 63) || (iTile == 64))) { return (1); }

	if (Unused (iTile) == 1) { return (1); }

	return (0);
}
/*****************************************************************************/
void CenterNumber (int iNumber, int iX, int iY,
	SDL_Color fore, int iHex)
/*****************************************************************************/
{
	char sText[MAX_TEXT + 2];

	if (iHex == 0)
	{
		snprintf (sText, MAX_TEXT, "%i", iNumber);
	} else {
		snprintf (sText, MAX_TEXT, "%02X", iNumber);
	}
	/* The 100000 is a workaround for 0 being broken. SDL devs have fixed that
	 * see e.g. https://hg.libsdl.org/SDL_ttf/rev/72b8861dbc01 but
	 * Ubuntu et al. still ship an sdl-ttf that is >10 years(!) old.
	 */
	message = TTF_RenderText_Blended_Wrapped (font3, sText, fore, 100000);
	messaget = SDL_CreateTextureFromSurface (ascreen, message);
	if (iHex == 0)
	{
		if ((iNumber >= -9) && (iNumber <= -1))
		{
			offset.x = iX + 16;
		} else if ((iNumber >= 0) && (iNumber <= 9)) {
			offset.x = iX + 21;
		} else if ((iNumber >= 10) && (iNumber <= 99)) {
			offset.x = iX + 14;
		} else {
			offset.x = iX + 7;
		}
	} else {
		offset.x = iX + 14;
	}
	offset.y = iY - 1;
	offset.w = message->w; offset.h = message->h;
	CustomRenderCopy (messaget, NULL, &offset, "message");
	SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
}
/*****************************************************************************/
int Unused (int iTile)
/*****************************************************************************/
{
	if ((iTile >= 13) && (iTile <= 16)) { return (1); }
	if (iTile == 20) { return (1); }
	if (iTile == 29) { return (1); }
	if ((iTile == 37) || (iTile == 38)) { return (1); }
	if (iTile == 42) { return (1); }

	return (0);
}
/*****************************************************************************/
int RaiseDropEvent (int iTile, int iEvent, int iAmount)
/*****************************************************************************/
{
	int iUseEvent;

	/*** Button numbers must be even. ***/
	if (IsEven (iEvent))
	{
		iUseEvent = iEvent;
	} else {
		if (iAmount < 0)
		{
			iUseEvent = iEvent - 1;
			if (iUseEvent == 0) { iUseEvent = 2; }
		} else {
			iUseEvent = iEvent + 1;
			if (iUseEvent == 16) { iUseEvent = 14; }
		}
	}

	return ((iUseEvent << 4) + iTile);
}
/*****************************************************************************/
void OpenURL (char *sURL)
/*****************************************************************************/
{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
ShellExecute (NULL, "open", sURL, NULL, NULL, SW_SHOWNORMAL);
#else
pid_t pid;
pid = fork();
if (pid == 0)
{
	execl ("/usr/bin/xdg-open", "xdg-open", sURL, (char *)NULL);
	exit (EXIT_NORMAL);
}
#endif
}
/*****************************************************************************/
void EXELoad (void)
/*****************************************************************************/
{
	int iFdEXE;
	unsigned char sRead[2 + 2];
	char sReadW[10 + 2];
	int iSlideNr, iSlideLine, iSlideChar;

	/*** Used for looping. ***/
	int iSlidesLoop;
	int iSlideLoop, iLineLoop, iCharLoop;

	iFdEXE = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Starting minutes left. ***/
	lseek (iFdEXE, 0x191, SEEK_SET);
	read (iFdEXE, sRead, 1);
	iEXEMinutesLeft = sRead[0];

	/*** Starting hit points. ***/
	lseek (iFdEXE, 0x80D, SEEK_SET);
	read (iFdEXE, sRead, 1);
	iEXEHitPoints = sRead[0];

	/*** Hair. ***/
	lseek (iFdEXE, 0xB1A2, SEEK_SET);
	read (iFdEXE, sRead, 2);
	snprintf (sReadW, 10, "%02x%02x", sRead[1], sRead[0]);
	iEXEHair = strtoul (sReadW, NULL, 16);
	ToFiveBitRGB (iEXEHair, &iEXEHairR, &iEXEHairG, &iEXEHairB);

	/*** Skin. ***/
	lseek (iFdEXE, 0xB1A4, SEEK_SET);
	read (iFdEXE, sRead, 2);
	snprintf (sReadW, 10, "%02x%02x", sRead[1], sRead[0]);
	iEXESkin = strtoul (sReadW, NULL, 16);
	ToFiveBitRGB (iEXESkin, &iEXESkinR, &iEXESkinG, &iEXESkinB);

	/*** Suit. ***/
	lseek (iFdEXE, 0xB1A6, SEEK_SET);
	read (iFdEXE, sRead, 2);
	snprintf (sReadW, 10, "%02x%02x", sRead[1], sRead[0]);
	iEXESuit = strtoul (sReadW, NULL, 16);
	ToFiveBitRGB (iEXESuit, &iEXESuitR, &iEXESuitG, &iEXESuitB);

	/*** Intro slides. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		for (iLineLoop = 1; iLineLoop <= SLIDES_LINES; iLineLoop++)
		{
			for (iCharLoop = 0; iCharLoop < SLIDES_LINES_CHARS; iCharLoop++)
			{
				arIntroSlides[iSlideLoop][iLineLoop][iCharLoop] = '\0';
			}
		}
	}
	lseek (iFdEXE, SLIDES_OFFSET, SEEK_SET);
	read (iFdEXE, sIntroSlides, SLIDES_BYTES);
	iSlideNr = 1;
	iSlideLine = 1;
	iSlideChar = 0;
	for (iSlidesLoop = 0; iSlidesLoop < SLIDES_BYTES; iSlidesLoop++)
	{
		if (sIntroSlides[iSlidesLoop] == 0x60) /*** space + newline ***/
		{
			/*** space ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] = ' ';
			iSlideChar++;
			/*** newline ***/
			iSlideLine++;
			iSlideChar = 0;
		} else if (sIntroSlides[iSlidesLoop] == 0x20) { /*** space ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] = ' ';
			iSlideChar++;
		} else if (sIntroSlides[iSlidesLoop] < 0x41) { /*** char + slide end ***/
			/*** char ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] =
				sIntroSlides[iSlidesLoop] + 0x40;
			/*** slide end ***/
			iSlideNr++;
			iSlideLine = 1;
			iSlideChar = 0;
		} else if (sIntroSlides[iSlidesLoop] >= 0xC1) { /*** char + space ***/
			/*** char ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] =
				sIntroSlides[iSlidesLoop] - 0x80;
			iSlideChar++;
			/*** space ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] = ' ';
			iSlideChar++;
		} else if (sIntroSlides[iSlidesLoop] >= 0x81) { /*** char + newline ***/
			/*** char ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] =
				sIntroSlides[iSlidesLoop] - 0x40;
			/*** newline ***/
			iSlideLine++;
			iSlideChar = 0;
		} else { /*** char ***/
			arIntroSlides[iSlideNr][iSlideLine][iSlideChar] =
				sIntroSlides[iSlidesLoop];
			iSlideChar++;
		}
	}
	IntroSlides();

	close (iFdEXE);
}
/*****************************************************************************/
void EXESave (void)
/*****************************************************************************/
{
	int iFdEXE;
	char sToWrite[MAX_TOWRITE + 2];

	/*** Used for looping. ***/
	int iNrFFLoop;

	iFdEXE = open (sPathFile, O_RDWR|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Starting minutes left. ***/
	lseek (iFdEXE, 0x191, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEMinutesLeft);
	write (iFdEXE, sToWrite, 1);

	/*** Starting hit points. ***/
	lseek (iFdEXE, 0x80D, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c", iEXEHitPoints);
	write (iFdEXE, sToWrite, 1);

	/*** Hair. ***/
	lseek (iFdEXE, 0xB1A2, SEEK_SET); /*** 1,2,3,4,8,9,12a,12b,v,t1,t2 ***/
	iEXEHair = iEXEHairB << 10;
	iEXEHair += iEXEHairG << 5;
	iEXEHair += iEXEHairR;
	sToWrite[0] = (iEXEHair >> 0) & 0xFF;
	sToWrite[1] = (iEXEHair >> 8) & 0xFF;
	write (iFdEXE, sToWrite, 2);
	/***/
	lseek (iFdEXE, 0xB1E2, SEEK_SET); /*** 6,7 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB222, SEEK_SET); /*** 5 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB262, SEEK_SET); /*** 10 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2A2, SEEK_SET); /*** 11 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2E2, SEEK_SET); /*** 12c ***/
	write (iFdEXE, sToWrite, 2);

	/*** Skin. ***/
	lseek (iFdEXE, 0xB1A4, SEEK_SET);
	iEXESkin = iEXESkinB << 10;
	iEXESkin += iEXESkinG << 5;
	iEXESkin += iEXESkinR;
	sToWrite[0] = (iEXESkin >> 0) & 0xFF;
	sToWrite[1] = (iEXESkin >> 8) & 0xFF;
	write (iFdEXE, sToWrite, 2);
	/***/
	lseek (iFdEXE, 0xB1E4, SEEK_SET); /*** 6,7 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB224, SEEK_SET); /*** 5 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB264, SEEK_SET); /*** 10 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2A4, SEEK_SET); /*** 11 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2E4, SEEK_SET); /*** 12c ***/
	write (iFdEXE, sToWrite, 2);

	/*** Suit. ***/
	lseek (iFdEXE, 0xB1A6, SEEK_SET);
	iEXESuit = iEXESuitB << 10;
	iEXESuit += iEXESuitG << 5;
	iEXESuit += iEXESuitR;
	sToWrite[0] = (iEXESuit >> 0) & 0xFF;
	sToWrite[1] = (iEXESuit >> 8) & 0xFF;
	write (iFdEXE, sToWrite, 2);
	/***/
	lseek (iFdEXE, 0xB1E6, SEEK_SET); /*** 6,7 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB226, SEEK_SET); /*** 5 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB266, SEEK_SET); /*** 10 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2A6, SEEK_SET); /*** 11 ***/
	write (iFdEXE, sToWrite, 2);
	lseek (iFdEXE, 0xB2E6, SEEK_SET); /*** 12c ***/
	write (iFdEXE, sToWrite, 2);

	/*** Intro slides. ***/
	IntroSlides();
	if (iBytesLeft >= 0)
	{
		if ((arSlideSizes[1] == 29) && (arSlideSizes[2] == 24) &&
			(arSlideSizes[3] == 37) && (arSlideSizes[4] == 31) &&
			(arSlideSizes[5] == 29))
		{
			lseek (iFdEXE, SLIDES_OFFSET, SEEK_SET);
			write (iFdEXE, sIntroSlides, strlen ((char *)sIntroSlides));

			/* Fill the rest with 0xFF. Currently, this never happens because of
			 * the if-check above.
			 */
			if (iBytesLeft > 0)
			{
				snprintf (sToWrite, MAX_TOWRITE, "%c", 0xFF);
				for (iNrFFLoop = 1; iNrFFLoop <= iBytesLeft; iNrFFLoop++)
					{ write (iFdEXE, sToWrite, 1); }
			}
		} else {
			printf ("[ WARN ] One or more incorrect slide sizes!\n");
		}
	} else {
		printf ("[ WARN ] Not enough free bytes!\n");
	}

	close (iFdEXE);

	PlaySound ("wav/save.wav");
}
/*****************************************************************************/
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged)
/*****************************************************************************/
{
	if ((InArea (iX, iY, iX + 13, iY + 20) == 1) &&
		(((iChange < 0) && (*iWhat > iMin)) ||
		((iChange > 0) && (*iWhat < iMax))))
	{
		*iWhat = *iWhat + iChange;
		if ((iChange < 0) && (*iWhat < iMin)) { *iWhat = iMin; }
		if ((iChange > 0) && (*iWhat > iMax)) { *iWhat = iMax; }
		if (iAddChanged == 1) { iChanged++; }
		PlaySound ("wav/plus_minus.wav");
		return (1);
	} else { return (0); }
}
/*****************************************************************************/
void ColorRect (int iX, int iY, int iW, int iH, int iR, int iG, int iB)
/*****************************************************************************/
{
	SDL_Rect rect;

	rect.x = iX * iScale;
	rect.y = iY * iScale;
	rect.w = iW * iScale;
	rect.h = iH * iScale;
	SDL_SetRenderDrawColor (ascreen, iR, iG, iB, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect (ascreen, &rect);
}
/*****************************************************************************/
void ToFiveBitRGB (int iIn, int *iR, int *iG, int *iB)
/*****************************************************************************/
{
	int iValue;

	iValue = iIn;

	*iR = 0; *iG = 0; *iB = 0;
	*iR = iValue & 0x1F;
	*iG = (iValue >> 5) & 0x1F;
	*iB = iValue >> 10;
	if (iDebug == 1)
	{
		printf ("[ INFO ] %i -> R:%i G:%i B:%i\n", iIn, *iR, *iG, *iB);
	}
}
/*****************************************************************************/
void TotalEvents (int iAmount)
/*****************************************************************************/
{
	if (((iAmount > 0) && (arNrEvents[iCurLevel] != 255)) ||
		((iAmount < 0) && (arNrEvents[iCurLevel] != 0)))
	{
		/*** Modify the event number. ***/
		arNrEvents[iCurLevel]+=iAmount;
		if (arNrEvents[iCurLevel] < 0) { arNrEvents[iCurLevel] = 0; }
		if (arNrEvents[iCurLevel] > 255) { arNrEvents[iCurLevel] = 255; }

		/*** If necessary, modify iChangeEvent. ***/
		if ((iChangeEvent == 0) && (arNrEvents[iCurLevel] != 0))
		{
			iChangeEvent = 1;
		}
		if (iChangeEvent > arNrEvents[iCurLevel])
		{
			iChangeEvent = arNrEvents[iCurLevel];
		}

		PlaySound ("wav/plus_minus.wav");
		iChanged++;
	}
}
/*****************************************************************************/
void GetOptionValue (char *sArgv, char *sValue)
/*****************************************************************************/
{
	int iTemp;
	char sTemp[MAX_OPTION + 2];

	iTemp = strlen (sArgv) - 1;
	snprintf (sValue, MAX_OPTION, "%s", "");
	while (sArgv[iTemp] != '=')
	{
		snprintf (sTemp, MAX_OPTION, "%c%s", sArgv[iTemp], sValue);
		snprintf (sValue, MAX_OPTION, "%s", sTemp);
		iTemp--;
	}
}
/*****************************************************************************/
int IsEven (int iValue)
/*****************************************************************************/
{
	return (iValue % 2 == 0);
}
/*****************************************************************************/
void IntroSlides (void)
/*****************************************************************************/
{
	int iSize;
	char cCharPrev, cChar, cCharNext;
	int arLength[SLIDES_LINES + 2];
	int iCheckAgain;
	int iSizeCheck;

	/*** Used for looping. ***/
	int iSlidesLoop;
	int iSlideLoop, iLineLoop, iCharLoop;

	iSize = 0;

	/*** Clear sIntroSlides. ***/
	for (iSlidesLoop = 0; iSlidesLoop < SLIDES_BYTES_MAX; iSlidesLoop++)
		{ sIntroSlides[iSlidesLoop] = '\0'; }

	/*** Disallow space as the last slide character. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		do {
			iCheckAgain = 0;
			arLength[1] = strlen (arIntroSlides[iSlideLoop][1]);
			arLength[2] = strlen (arIntroSlides[iSlideLoop][2]);
			arLength[3] = strlen (arIntroSlides[iSlideLoop][3]);
			arLength[4] = strlen (arIntroSlides[iSlideLoop][4]);
			if (arLength[4] != 0)
			{
				if (arIntroSlides[iSlideLoop][4][arLength[4] - 1] == ' ')
				{
					if (iS != iSlideLoop)
					{
						arIntroSlides[iSlideLoop][4][arLength[4] - 1] = '\0';
						iCheckAgain = 1;
					} else {
						iN = 1;
					}
				}
			} else if (arLength[3] != 0) {
				if (arIntroSlides[iSlideLoop][3][arLength[3] - 1] == ' ')
				{
					if (iS != iSlideLoop)
					{
						arIntroSlides[iSlideLoop][3][arLength[3] - 1] = '\0';
						iCheckAgain = 1;
					} else {
						iN = 1;
					}
				}
			} else if (arLength[2] != 0) {
				if (arIntroSlides[iSlideLoop][2][arLength[2] - 1] == ' ')
				{
					if (iS != iSlideLoop)
					{
						arIntroSlides[iSlideLoop][2][arLength[2] - 1] = '\0';
						iCheckAgain = 1;
					} else {
						iN = 1;
					}
				}
			} else if (arLength[1] != 0) {
				if (arIntroSlides[iSlideLoop][1][arLength[1] - 1] == ' ')
				{
					if (iS != iSlideLoop)
					{
						arIntroSlides[iSlideLoop][1][arLength[1] - 1] = '\0';
						iCheckAgain = 1;
					} else {
						iN = 1;
					}
				}
			}
		} while (iCheckAgain != 0);
	}

	/*** Disallow empty slides. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		if ((strlen (arIntroSlides[iSlideLoop][1]) == 0) &&
			(strlen (arIntroSlides[iSlideLoop][2]) == 0) &&
			(strlen (arIntroSlides[iSlideLoop][3]) == 0) &&
			(strlen (arIntroSlides[iSlideLoop][4]) == 0))
		{
			if (iS != iSlideLoop)
			{
				snprintf (arIntroSlides[iSlideLoop][1], SLIDES_LINES_CHARS,
					"%s", "REQUIRES TEXT");
			} else {
				iN = 1;
			}
		}
	}

	/*** Add a space on empty lines if the next line has content. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		for (iLineLoop = 3; iLineLoop >= 1; iLineLoop--)
		{
			if ((strlen (arIntroSlides[iSlideLoop][iLineLoop + 1]) != 0) &&
				(strlen (arIntroSlides[iSlideLoop][iLineLoop]) == 0))
			{
				if (iL != iLineLoop)
				{
					arIntroSlides[iSlideLoop][iLineLoop][0] = ' ';
				} else {
					iN = 1;
				}
			}
		}
	}

	/*** Create sIntroSlides and update iSize and arSlideSizes[]. ***/
	for (iSlideLoop = 1; iSlideLoop <= SLIDES; iSlideLoop++)
	{
		arSlideSizes[iSlideLoop] = 0;
		for (iLineLoop = 1; iLineLoop <= SLIDES_LINES; iLineLoop++)
		{
			for (iCharLoop = 0; iCharLoop < SLIDES_LINES_CHARS; iCharLoop++)
			{
				cCharPrev = arIntroSlides[iSlideLoop][iLineLoop][iCharLoop - 1];
				cChar = arIntroSlides[iSlideLoop][iLineLoop][iCharLoop];
				cCharNext = arIntroSlides[iSlideLoop][iLineLoop][iCharLoop + 1];
				if (cChar != '\0')
				{
					if ((cChar == ' ') && (cCharPrev != ' ') &&
						(cCharPrev != '\0') && (cCharNext != '\0'))
					{
						sIntroSlides[iSize - 1]+=0x80;
					} else {
						sIntroSlides[iSize] = cChar;
						iSize++;
						arSlideSizes[iSlideLoop]++;
					}
				} else if ((iLineLoop == 4) ||
					((iLineLoop == 3) && (arIntroSlides[iSlideLoop][4][0] == '\0')) ||
					((iLineLoop == 2) && (arIntroSlides[iSlideLoop][4][0] == '\0') &&
					(arIntroSlides[iSlideLoop][3][0] == '\0')) ||
					((iLineLoop == 1) && (arIntroSlides[iSlideLoop][4][0] == '\0') &&
					(arIntroSlides[iSlideLoop][3][0] == '\0') &&
					(arIntroSlides[iSlideLoop][2][0] == '\0'))) { /*** slide end ***/
					iCharLoop = SLIDES_LINES_CHARS;
					iLineLoop = SLIDES_LINES;
					sIntroSlides[iSize - 1]-=0x40;
				} else { /*** newline ***/
					iCharLoop = SLIDES_LINES_CHARS;
					sIntroSlides[iSize - 1]+=0x40;
				}
			}
		}
	}

	iSizeCheck = (int)strlen ((char *)sIntroSlides);
	if (iSize != iSizeCheck)
	{
		printf ("[ WARN ] Non-matching total slide sizes: %i and %i!\n",
			iSize, iSizeCheck);
	}

	iBytesLeft = SLIDES_BYTES - iSize;
}
/*****************************************************************************/
void ModifyForMednafen (int iLevel)
/*****************************************************************************/
{
	int iFd;
	char sToWrite[MAX_TOWRITE + 2];
	int iToLevel;

	/*** Open file. ***/
	iFd = open (sPathFile, O_RDWR|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Make training the active in-editor level. ***/
	switch (iLevel)
	{
		case 15: iToLevel = 15; break;
		case 16: iToLevel = 16; break;
		case 17: iToLevel = 17; break; /*** Does not work. ***/
		default: iToLevel = iLevel - 1; break;
	}
	lseek (iFd, OFFSET_TRAINING, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c", iToLevel);
	write (iFd, sToWrite, 1);

	/*** Start training. ***/
	lseek (iFd, OFFSET_START, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c%c%c", 0xC3, 0x63, 0x06);
	write (iFd, sToWrite, 3);

	/*** Skip Red Orb screen. ***/
	lseek (iFd, OFFSET_REDORB, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c%c", 0x18, 0x19);
	write (iFd, sToWrite, 2);

	close (iFd);

	iModified = 1;
}
/*****************************************************************************/
void ModifyBack (void)
/*****************************************************************************/
{
	int iFd;
	char sToWrite[MAX_TOWRITE + 2];

	/*** Open file. ***/
	iFd = open (sPathFile, O_RDWR|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** [Undo] Make training the active in-editor level. ***/
	lseek (iFd, OFFSET_TRAINING, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c", 0x10);
	write (iFd, sToWrite, 1);

	/*** [Undo] Start training. ***/
	lseek (iFd, OFFSET_START, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c%c%c", 0xCD, 0x5F, 0x20);
	write (iFd, sToWrite, 3);

	/*** [Undo] Skip Red Orb screen. ***/
	lseek (iFd, OFFSET_REDORB, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%c%c", 0x38, 0xF7);
	write (iFd, sToWrite, 2);

	close (iFd);

	iModified = 0;
}
/*****************************************************************************/
