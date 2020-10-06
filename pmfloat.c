/*
 Copyright (c) 1994 Paul Floyd.
All rights reserved.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
advertising materials, and other materials related to such
distribution and use acknowledge that the software was developed
by the <organization>. The name of the
<organization> may not be used to endorse or promote products derived
from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINSWITCHLIST
#define INCL_WINSYS
#define INCL_WINDIALOGS
#define INCL_WINBUTTONS
#define INCL_WINPOINTERS
#define INCL_WINENTRYFIELDS
#define INCL_WINMENUS
#define INCL_WININPUT
#define INCL_WINACCELERATORS

/* Short = 2 bytes = 4 chars + 1 space = 5 chars long */
#define ShortChars 5
/* Long = 4 bytes = 8 chars + 3 spaces = 11 chars long */
#define LongChars 11
/* Float = 4 bytes = 8 chars + 3 spaces = 11 chars long */
#define FloatChars 11
/* Double = 8 bytes = 16 chars + 7 spaces = 23 chars long */
#define DoubleChars 23
/* Long Double = 10 bytes = 20 chars + 9 spaces = 29 chars long */
#define LDoubleChars 29

/* Short = +/-32768 i.e. 6 spaces */
#define ShortPrecision 6
/* Long = +/-2147483648 i.e. 11 spaces */
#define LongPrecision 11
/* Float = +/-0.(9 digits precision)e+/-(38 max) i.e. 15 spaces */
#define FloatPrecision 16
#define FloatDigits 9
/* Double = +/-0.(16 digits precision)e+/-(308 max) i.e. 24 spaces */
#define DoublePrecision 24
#define DoubleDigits 16
/* Long Double = +/-.(20 digits of precision)e+/-(4932 max) i.e. 27 spaces */
#define LDoublePrecision 29
#define LDoubleDigits 20

#define AlphaHexOffset 87

/* defines for the mode we are in */
#define ViewUShort 1
#define ViewShort 2
#define ViewULong 4
#define ViewLong 8
#define ViewFloat 16
#define ViewDouble 32
#define ViewLDouble 64

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pmfloat.h"

/* This is the storage for whatever mode we are in */
union Num
{
   unsigned char ucNum[16];
   char cNum[16];
   unsigned short usNum;
   signed short sNum;
   unsigned long ulNum;
   signed long lNum;
   float fNum;
   double dNum;
   long double ldNum;
} Number;

/* define functions used */
MRESULT EXPENTRY DialogProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY AboutProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY HelpProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY HexProc(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY HexProcSwap(HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY NumericProc   (HWND, ULONG, MPARAM, MPARAM);
char *HexToStringBS(union Num, char *, int);
char *HexToString(union Num, char *, int);
void StringToHexBS(const char *, int, union Num *);
void StringToHex(const char *, int, union Num *);
int CheckIt(unsigned char *, int);
int CheckValue(unsigned char *);

/* pointers to functions used for subclassing */
PFNWP  EntryFieldProc;
PFNWP  EntryFieldProc2;
PFNWP  EntryFieldProc3;

/* flags */
ULONG fDisplayType = ViewDouble; /* type to display */
ULONG fFocus = ID_HEX; /* entry box currently with focus */

INT main(VOID)
{
   HAB  hab;
   HMQ  hmq;
 
   hab = WinInitialize(0);
   hmq = WinCreateMsgQueue(hab, 0);

   /* Launch the dialog box */
   WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, DialogProc, (HMODULE)NULL, ID_MAIN, 0);

   WinDestroyMsgQueue(hmq);
   WinTerminate(hab);
   return 0;
} /* end main */

/* this is the function that does the biz */
MRESULT EXPENTRY DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   static HWND hDlgBoxIcon, hMenu, hwndFrame; /* handles for the menu and icon */
   HACCEL haccelAccel; /* handle for the accelerator table */
   HAB habAccel; /* anchor block for the accelerator table */
   SWCNTRL PgmEntry; /* for registering the program with the OS/2 switch list */
   CHAR szBuffer[35]; /* a text string */
   CHAR szOutputString[35]; /* another text string */
   USHORT usItemId; /* menu item identifier */
   long double temp; /* for converting up to ld */

   switch (msg)
   {
   case WM_INITDLG: /* 1st time setup
      /* subclass the entry fields to filter unwanted keystrokes */
      EntryFieldProc = WinSubclassWindow(WinWindowFromID(hwndDlg, ID_HEX), (PFNWP)HexProc);
      EntryFieldProc2 = WinSubclassWindow(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PFNWP)HexProcSwap);
      EntryFieldProc3 = WinSubclassWindow(WinWindowFromID(hwndDlg, ID_VALUE), (PFNWP)NumericProc);

      /* load the icon, otherwise the program will not minimize */
      hDlgBoxIcon = WinLoadPointer(HWND_DESKTOP, (HMODULE)NULL, ID_ICON);
      WinSendMsg(hwndDlg, WM_SETICON, (MPARAM)hDlgBoxIcon, 0);

      /* limit the number of characters in the hex dialog boxes */
      WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(DoubleChars), 0);
      WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(DoubleChars), 0);
      /* And limit those for the value box */
      WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(DoublePrecision), 0);

      /* this is the default value (2.781828....) */
      Number.dNum = exp(1.0);

      /* output this to the VALUE entry field */
      WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.dNum, DoubleDigits, szBuffer));

      /* output the hex representation */
      WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(double)));

      /* output the Intel endian format */
      WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(double)));

      /* set the input focus to the hex (top) window */
      WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwndDlg, ID_HEX));

      /* load up the user menu - not normally done on a dialog box */
      WinLoadMenu(hwndDlg, (HMODULE)NULL, ID_MENU);

      /* ditto, load the accelerator table */
      haccelAccel = WinLoadAccelTable(habAccel, (HMODULE)NULL, ID_MENU); /* load the accelerator table */
      WinSetAccelTable(habAccel, haccelAccel, hwndDlg);

      /* update the dialog box window to show the menu bar */
      WinSendMsg(hwndDlg, WM_UPDATEFRAME, (MPARAM)FID_MENU, 0);

      /* register the program */
      PgmEntry.hwnd = hwndDlg;
      PgmEntry.hwndIcon = hDlgBoxIcon;
      PgmEntry.hprog = (HPROGRAM)NULL;
      PgmEntry.idProcess = (PID)NULL;
      PgmEntry.idSession = (ULONG)NULL;
      PgmEntry.uchVisibility = SWL_VISIBLE;
      PgmEntry.fbJump = SWL_JUMPABLE;
      strcpy(PgmEntry.szSwtitle, "PM Float");
      WinAddSwitchEntry(&PgmEntry);
 
      return (MRESULT)TRUE;
 
   /* focus changes */
   case WM_FOCUSCHANGE:
      if (SHORT1FROMMP(mp2) == TRUE)
      {
         /*********************************************/
         /*this is the message we're not interested in*/
         /*focus change causes two messages to be sent*/
         /*one to the window losing focus             */
         /*one to the window gaining focus            */
         /*and this is the one for the focus loss     */
         /*in which I have no interest                */
         /*********************************************/
         return WinDefDlgProc(hwndDlg, msg, mp1, mp2);
      } /* endif */

      if (HWNDFROMMP(mp1) == WinWindowFromID(hwndDlg, ID_HEX))
      {
         /* gained focus */
         fFocus = ID_HEX;
      } /* endif */

      if (HWNDFROMMP(mp1) == WinWindowFromID(hwndDlg, ID_HEXSWAP))
      {
         fFocus = ID_HEXSWAP;
      } /* endif */

      if (HWNDFROMMP(mp1) == WinWindowFromID(hwndDlg, ID_VALUE))
      {
         fFocus = ID_VALUE;
      } /* endif */
      return WinDefDlgProc(hwndDlg, msg, mp1, mp2);

   /* this message is sent when a menu is about to appear */
   case WM_INITMENU:
      switch (SHORT1FROMMP(mp1))
      {
      case ID_VIEW:
         /* the current display type is shown with a tick */
         WinCheckMenuItem(HWNDFROMMP(mp2), ID_SHORT, (fDisplayType & ViewShort) ? TRUE : FALSE);
         WinCheckMenuItem(HWNDFROMMP(mp2), ID_LONG, (fDisplayType & ViewLong) ? TRUE : FALSE);
         WinCheckMenuItem(HWNDFROMMP(mp2), ID_FLOAT, (fDisplayType & ViewFloat) ? TRUE : FALSE);
         WinCheckMenuItem(HWNDFROMMP(mp2), ID_DOUBLE, (fDisplayType & ViewDouble) ? TRUE : FALSE);
         WinCheckMenuItem(HWNDFROMMP(mp2), ID_LDOUBLE, (fDisplayType & ViewLDouble) ? TRUE : FALSE);
         break;
      default:
        break;
      } /* endswitch */
      break;

   /* Menu options and pushbuttons */
   case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
         /* F3 pressed, Quit button pressed or ESC, or Exit from the menu bar */
         case DID_CANCEL:
         case ID_EXIT:
            /* exit */
            WinDestroyPointer(hDlgBoxIcon);
            WinRemoveSwitchEntry(WinQuerySwitchHandle(hwndDlg, 0));
            WinPostMsg(hwndDlg, WM_QUIT, 0, 0);
            break;
         case ID_SHORT:
            /* change to showing short integers */
            /* whatever crap happens to be in the union gets dispalyed */
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(ShortChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(ShortChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(ShortPrecision), 0);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_itoa((int)Number.sNum, szBuffer, 10));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(short)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(short)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_STAT), (PSZ)"Short");
            fDisplayType = ViewShort;
            return FALSE;
         case ID_LONG:
            /* change to showing long integers */
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(LongChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(LongChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(LongPrecision), 0);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_ltoa(Number.lNum, szBuffer, 10));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(long)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(long)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_STAT), (PSZ)"Long");
            fDisplayType = ViewLong;
            return FALSE;
         case ID_FLOAT:
            /* change to showing floats */
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(FloatChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(FloatChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(FloatPrecision), 0);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.fNum, FloatDigits, szBuffer));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(float)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(float)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_STAT), (PSZ)"Float");
            fDisplayType = ViewFloat;
            return FALSE;
         case ID_DOUBLE:
            /* change to showing doubles */
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(DoubleChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(DoubleChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(DoublePrecision), 0);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.dNum, DoubleDigits, szBuffer));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.dNum, DoubleDigits, szBuffer));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(double)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(double)));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_STAT), (PSZ)"Double");
            fDisplayType = ViewDouble;
            return FALSE;
         case ID_LDOUBLE:
            /* change to showing long doubles */
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEX), EM_SETTEXTLIMIT, MRFROMSHORT(LDoubleChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_HEXSWAP), EM_SETTEXTLIMIT, MRFROMSHORT(LDoubleChars), 0);
            WinSendMsg(WinWindowFromID(hwndDlg, ID_VALUE), EM_SETTEXTLIMIT, MRFROMSHORT(LDoublePrecision), 0);
            temp = (long double)Number.dNum;
            Number.ldNum = temp;
            /* no direct way to convert ld to string */
            sprintf(szBuffer, "%-.20Lg\0", Number.ldNum);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)szBuffer);
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, 10));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, 10));
            WinSetWindowText(WinWindowFromID(hwndDlg, ID_STAT), (PSZ)"Long Double");
            fDisplayType = ViewLDouble;
            return FALSE;
         case DID_OK:
            /* what was the last used entry box??? */
            switch (fFocus)
            {
               case ID_HEX:
                  WinQueryDlgItemText(hwndDlg, ID_HEX, sizeof(szBuffer), szBuffer);
                  switch (fDisplayType)
                  {
                  case ViewShort:
                     if (!CheckIt(szBuffer, sizeof(short)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, sizeof(short), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _itoa((int)Number.sNum, szBuffer, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(short)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(short)));
                     } /* endif */
                     break;
                  case ViewLong:
                     if (!CheckIt(szBuffer, sizeof(long)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, sizeof(long), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _itoa(Number.lNum, szBuffer, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(long)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(long)));
                     } /* endif */
                     break;
                  case ViewFloat:
                     if (!CheckIt(szBuffer, sizeof(float)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, sizeof(float), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _gcvt(Number.fNum, FloatDigits, szBuffer));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(float)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(float)));
                     } /* endif */
                     break;
                  case ViewDouble:
                     if (!CheckIt(szBuffer, sizeof(double)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, sizeof(double), &Number);

                        /* output this to the VALUE entry field */
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.dNum, DoubleDigits, szBuffer));

                        /* output the hex representation */
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(double)));

                        /* output the Intel endian format */
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(double)));
                     } /* endif */
                     break;
                  case ViewLDouble:
                     if (!CheckIt(szBuffer, 10)) /* sizeof gives 16 i.e. nearest power of 2, instead of 10 */
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, 10, &Number);
                        sprintf(szBuffer, "%Lg\0", szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, 10));
                      } /* endif */
                     break;
                  default:
                     break;
                  } /* endswitch */
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwndDlg, ID_HEX));
                  break;

               case ID_HEXSWAP:
                  WinQueryDlgItemText(hwndDlg, ID_HEXSWAP, sizeof(szBuffer), szBuffer);

                  switch (fDisplayType)
                  {
                  case ViewShort:
                     if (!CheckIt(szBuffer, sizeof(short)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHexBS(szBuffer, sizeof(short), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _itoa((int)Number.sNum, szBuffer, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(short)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(short)));
                     } /* endif */
                     break;
                  case ViewLong:
                     if (!CheckIt(szBuffer, sizeof(long)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHexBS(szBuffer, sizeof(long), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _itoa(Number.lNum, szBuffer, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(long)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(long)));
                     } /* endif */
                     break;
                  case ViewFloat:
                     if (!CheckIt(szBuffer, sizeof(float)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHexBS(szBuffer, sizeof(float), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), _gcvt(Number.fNum, FloatDigits, szBuffer));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(float)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(float)));
                     } /* endif */
                     break;
                  case ViewDouble:
                     if (!CheckIt(szBuffer, sizeof(double)))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHexBS(szBuffer, sizeof(double), &Number);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), (PSZ)_gcvt(Number.dNum, DoubleDigits, szBuffer));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(double)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(double)));
                     } /* endif */
                     break;
                  case ViewLDouble:
                     if (!CheckIt(szBuffer, 10))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        StringToHex(szBuffer, 10, &Number);
                        sprintf(szBuffer, "%Lg", szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_VALUE), szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, 10));
                     } /* endif */
                     break;
                  default:
                    break;
                  } /* endswitch fDisplayType */
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwndDlg, ID_HEXSWAP));
                  break;

               case ID_VALUE:
                  WinQueryDlgItemText(hwndDlg, ID_VALUE, sizeof(szBuffer), szBuffer);
                  switch (fDisplayType)
                  {
                  case ViewShort:
                     if (!CheckValue(szBuffer))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        Number.sNum = (short)atoi(szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(short)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(short)));
                     } /* endif */
                     break;
                  case ViewLong:
                     if (!CheckValue(szBuffer))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        Number.lNum = atoi(szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(long)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(long)));
                     } /* endif */
                     break;
                  case ViewFloat:
                     if (!CheckValue(szBuffer))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        Number.fNum = (float)atof(szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(float)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(float)));
                     } /* endif */
                     break;
                  case ViewDouble:
                     if (!CheckValue(szBuffer))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        Number.dNum = atof(szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, sizeof(double)));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, sizeof(double)));
                     } /* endif */
                     break;
                  case ViewLDouble:
                     if (!CheckValue(szBuffer))
                     {
                        WinMessageBox(HWND_DESKTOP, hwndDlg, szBuffer, "You wrongly entered...", 0, MB_INFORMATION);
                     }
                     else
                     {
                        /* go ahead with the processing */
                        Number.ldNum = _atold(szBuffer);
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEXSWAP), (PSZ)HexToStringBS(Number, szOutputString, 10));
                        WinSetWindowText(WinWindowFromID(hwndDlg, ID_HEX), (PSZ)HexToString(Number, szOutputString, 10));
                      } /* endif */
                     break;
                  default:
                     break;
                  } /* endswitch */

                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwndDlg, ID_VALUE));                  
                  break;

               default:
                  break;
               } /* endswitch fFocus */
            return FALSE;
         case ID_HELPOPTION:
            WinDlgBox(HWND_DESKTOP, hwndDlg, HelpProc, 0, ID_HELP, 0);
            return FALSE;
         case ID_ABOUTOPTION:
            WinDlgBox(HWND_DESKTOP, hwndDlg, AboutProc, 0, ID_ABOUT, 0);
            return FALSE;
         default:
            return FALSE;
      } /* endswitch SHORT1FROMMP(mp1) */
      WinDismissDlg(hwndDlg, TRUE);
      break;

   default: 
      return WinDefDlgProc(hwndDlg, msg, mp1, mp2);
   } /* endswitch msg */
   return FALSE;
} /* end function DialogProc */

/* subclass function - only passes on hexadecimal charaters 0 to 9, a to f */
/* and A to F and virtual keys (cursor etc). Other ASCII characters are */
/* discarded */
MRESULT EXPENTRY HexProc(HWND hwndHex, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   if (msg == WM_CHAR)
   { 
      /* is it a funny key that I can't tell the ASCII value? */
      if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
      {
         return (*EntryFieldProc)(hwndHex, msg, mp1, mp2);
      } /* endif virtualkey */

      if (!(SHORT1FROMMP(mp1) & KC_KEYUP))
      {
         if ((SHORT1FROMMP(mp2) < 0x30 || SHORT1FROMMP(mp2) > 0x39) && /* 0 to 9 */
             (SHORT1FROMMP(mp2) != 9) && /* tab */
             (SHORT1FROMMP(mp2) != 0xd) && /* carriage return */
             (SHORT1FROMMP(mp2) < 0x61 || SHORT1FROMMP(mp2) > 0x66) && /* A to Z */
             (SHORT1FROMMP(mp2) < 0x41 || SHORT1FROMMP(mp2) > 0x46) && /* a to z */
             (SHORT1FROMMP(mp2) != 0x20)) /* <space> */
         {
            WinAlarm(HWND_DESKTOP, WA_WARNING);
            return (MRESULT)TRUE;
         } /* endif */
      } /* endif */
   } /* endif */
   return (*EntryFieldProc)(hwndHex, msg, mp1, mp2);
} /* end HexProc */

/* Another copy of the previous function for the second dialog box */
MRESULT EXPENTRY HexProcSwap(HWND hwndHex2, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   if (msg == WM_CHAR)
   {
      /* is it a funny key that I can't tell the ASCII value? */
      if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
      {
         return (*EntryFieldProc2)(hwndHex2, msg, mp1, mp2);
      } /* endif virtualkey */

      if (!(SHORT1FROMMP(mp1) & KC_KEYUP))
      {
         if ((SHORT1FROMMP(mp2) < 0x30 || SHORT1FROMMP(mp2) > 0x39) && /* 0 to 9
             (SHORT1FROMMP(mp2) != 9) && /* tab */
             (SHORT1FROMMP(mp2) != 0xd) && /* carriage return */
             (SHORT1FROMMP(mp2) < 0x61 || SHORT1FROMMP(mp2) > 0x66) && /* A to Z */
             (SHORT1FROMMP(mp2) < 0x41 || SHORT1FROMMP(mp2) > 0x46) && /* a to z */
             (SHORT1FROMMP(mp2) != 0x20)) /* space */
         {
            WinAlarm(HWND_DESKTOP, WA_WARNING);
            return (MRESULT)TRUE;
         } /* endif */
      } /* endif */
   } /* endif */
   return (*EntryFieldProc2)(hwndHex2, msg, mp1, mp2);
} /* end HexProcSwap */

/* third and last subclass function */
/* this one only allows the letters e and E (for ^10), '.', '+' and '-' */
/* otherwise the same as the previous */
MRESULT EXPENTRY NumericProc(HWND hwndNum, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   if (msg == WM_CHAR)
   { 
      /* is it a funny key that I can't tell the ASCII value? */
      if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
      {
         return (*EntryFieldProc3)(hwndNum, msg, mp1, mp2);
      } /* endif virtualkey */
      if (!(SHORT1FROMMP(mp1) & KC_KEYUP))
      {
         if ((SHORT1FROMMP(mp2) < 0x30 || SHORT1FROMMP(mp2) > 0x39) && /* 0 to 9 */
             (SHORT1FROMMP(mp2) != 9) && /* tab */
             (SHORT1FROMMP(mp2) != 0xd) && /* carriage return */
             (SHORT1FROMMP(mp2) != 0x2b) && /* + */
             (SHORT1FROMMP(mp2) != 0x2d) && /* - */
             (SHORT1FROMMP(mp2) != 0x2e) && /* . */
             (SHORT1FROMMP(mp2) != 0x65) && /* e */
             (SHORT1FROMMP(mp2) != 0x45)) /* E */
         {
            WinAlarm(HWND_DESKTOP, WA_WARNING);
            return (MRESULT)TRUE;
         } /* endif */
      } /* endif */
   } /* endif */
   return (*EntryFieldProc3)(hwndNum, msg, mp1, mp2);
} /* end NumericProc */

/* function to convert a double to a string representing */
/* the bytes in the order that they should be */
char *HexToStringBS(union Num chUnion, char *pszString, int bytes)
{
   BYTE *pChop;
   BYTE Send;
   char szConvBuf[10];
   int count;

   /* bytes hex chars */
   pChop = &chUnion.cNum[0];
   Send = *pChop;
   strcpy(pszString, "\0"); /* make the string null */

   for (count = 0; count < bytes; count++)
   {
      Send = *pChop; /* make a copy */
      if (Send < 0x10)
      {
         /* put in a leading 0 */
         strcat(pszString, "0\0");
      } /* endif */
      _itoa((int)Send, szConvBuf, 16);
      strcat(pszString, szConvBuf);
      strcat(pszString, " \0");
      pChop++; /* point to next byte in double */
   } /* endfor count */

   return pszString;
 
} /* end function HexToStringBS */

/* function to convert a double to a string representing */
/* the bytes in the order that they appear in memory */
char *HexToString(union Num chUnion, char *pszString, int bytes)
{
   BYTE *pChop;
   BYTE Send;
   char szConvBuf[10];
   int count;

   /* bytes hex chars */
   pChop = &chUnion.cNum[0];
   pChop += bytes - 1; /* make it point to end of the current type */
   Send = *pChop;
   strcpy(pszString, "\0"); /* make the string null */

   for (count = 0; count < bytes - 1; count++)
   {
      Send = *pChop; /* make a copy */
      if (Send < 0x10)
      {
         /* put in a leading 0 */
         strcat(pszString, "0\0");
      } /* endif */
      _itoa((int)Send, szConvBuf, 16);
      strcat(pszString, szConvBuf);
      strcat(pszString, " \0");
      pChop--; /* point to next byte in double */
   } /* endfor count */

   /* one more time, but without the trailing space */
   Send = *pChop; /* make a copy */
   if (Send < 0x10)
   {
      /* put in a leading 0 */
      strcat(pszString, "0\0");
   } /* endif */
   _itoa((int)Send, szConvBuf, 16);
   strcat(pszString, szConvBuf);

   return pszString;
 
} /* end function HexToString */

int CheckIt(unsigned char *szTest, int CheckCount)
{
   int count = 0;
   unsigned char *cptr = szTest;

   while (cptr != '\0')
   {
      while (isspace(*cptr))
      {
         cptr++;
      } /* endwhile */

      if (*cptr == '\0')
      {
         break;
      } /* endif */

      if (isxdigit(*cptr) && isspace(*(cptr+1)))
      {
         cptr++;
         count++;
         continue;
      } /* endif */

      if (isxdigit(*cptr) && *(cptr+1) == '\0')
      {
         count++;
         break;
      } /* endif */

      if ((isxdigit(*cptr) && isxdigit(*(cptr+1))) && *(cptr+2) == '\0')
      {
         count++;
         break;
      } /* endif */

      if ((isxdigit(*cptr) && isxdigit(*(cptr+1))) && isspace(*(cptr+2)))
      {
         count++;
         cptr+=2;
         continue;
      }
      else
      {
         count = 0;
         break;
      } /* endif */
   } /* endwhile */
   if (count == CheckCount)
   {
      return 1;
   }
   else
   {
      return 0;
   } /* endif */

} /* end function CheckIt */

int CheckValue(unsigned char *szString)
{
   unsigned char *chpoint = szString;

   int DecimalPointCount = 0;
   int ECount = 0;
   int PlusOrMinusCount = 0;

   while (*chpoint != '\0')
   {
      switch (*chpoint)
      {
      case '.':
         DecimalPointCount++;
         break;
      case '+':
      case '-':
         PlusOrMinusCount++;
         break;
      case 'e':
      case 'E':
         ECount++;
         break;
      default:
        break;
      } /* endswitch */
      chpoint++;
   } /* endwhile */
   if ((ECount <= 1 && PlusOrMinusCount <= 2) && DecimalPointCount <= 1)
   {
      return 1;
   }
   else
   {
      return 0;
   } /* endif */

} /* end function CheckValue */

void StringToHexBS(const char *szString, int digits, union Num *chUnion)
{
   const char *index = szString;
   char *end;
   int i;

   for (i = 0; i < digits; i++)
   {
      while (isspace(*index))
      {
         index++;
      } /* endwhile */
 
      chUnion->cNum[i] = (char)strtol(index, &end, 16);
      index = end;
   } /* endfor */
} /* end function StringToHexBS */

void StringToHex(const char *szString, int digits, union Num *chUnion)
{
   const char *index = szString;
   char *end;
   int i;

   for (i = 0; i < digits; i++)
   {
      while (isspace(*index))
      {
         index++;
      } /* endwhile */

      chUnion->cNum[digits - i - 1] = (char)strtol(index, &end, 16);
      index = end;
   } /* endfor */
} /* end function StringToHex */

MRESULT EXPENTRY AboutProc(HWND hwndAbout, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
   case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
      case ID_ABOUT_OK:
         WinDismissDlg(hwndAbout, ID_ABOUT_OK);
         break;
      default: 
        break;
      } /* endswitch */
      break;
   default: 
      return  WinDefDlgProc(hwndAbout, msg, mp1, mp2);
      break;
   } /* endswitch */
   return (MRESULT)0;
} /* end function AboutProc */

MRESULT EXPENTRY HelpProc(HWND hwndHelp, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
   case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      {
      case ID_HELP_OK:
         WinDismissDlg(hwndHelp, ID_ABOUT_OK);
         break;
      default: 
        break;
      } /* endswitch */
      break;
   default: 
      return  WinDefDlgProc(hwndHelp, msg, mp1, mp2);
      break;
   } /* endswitch */
   return (MRESULT)0;
} /* end function HelpProc */
