/*
 *  win32dep.c
 *
 *  Author: Herman Bloggs <hermanator12002@yahoo.com>
 *  Date: June, 2002
 *  Description: Windows dependant code for Gaim
 */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winuser.h>
#include <glib.h>
#include <gdk/gdkwin32.h>

#include "gaim.h"
#include "stdafx.h"
#include "resource.h"
#include "MinimizeToTray.h"
#include "systray.h"
#include "winuser_extra.h"
#include "idletrack.h"
#include "zlib.h"
#include "untar.h"

/*
 *  DEFINES & MACROS
 */

/*
 * DATA STRUCTS
 */
struct _WGAIM_FLASH_INFO {
	guint t_handle;
	guint sig_handler;
};
typedef struct _WGAIM_FLASH_INFO WGAIM_FLASH_INFO;

/*
 * LOCALS
 */
static char install_dir[MAXPATHLEN];
static char lib_dir[MAXPATHLEN];
static char locale_dir[MAXPATHLEN];

/*
 *  GLOBALS
 */
HINSTANCE gaimexe_hInstance = 0;
HINSTANCE gaimdll_hInstance = 0;

/*
 *  PROTOS
 */

BOOL (*MyFlashWindowEx)(PFLASHWINFO pfwi)=NULL;
FARPROC wgaim_find_and_loadproc(char*, char*);
extern void wgaim_gtkspell_init();

/*
 *  STATIC CODE
 */

/* Window flasher */
static gboolean flash_window_cb(gpointer data) {
	FlashWindow((HWND)data, TRUE);
	return TRUE;
}

static void halt_flash_filter(GtkWidget *widget, GdkEventFocus *event, WGAIM_FLASH_INFO *finfo) {
	/* Stop flashing and remove filter */
	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "Removing timeout\n");
	g_source_remove(finfo->t_handle);
	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "Disconnecting signal handler\n");
	g_signal_handler_disconnect(G_OBJECT(widget),finfo->sig_handler);
	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "done\n");
}

static void load_winver_specific_procs(void) {
	/* Used for Win98+ and WinNT5+ */
	MyFlashWindowEx = (void*)wgaim_find_and_loadproc("user32.dll", "FlashWindowEx" );
}

/*
 *  PUBLIC CODE
 */

HINSTANCE wgaim_hinstance(void) {
	return gaimexe_hInstance;
}

/* Escape windows dir separators.  This is needed when paths are saved,
   and on being read back have their '\' chars used as an escape char.
   Returns an allocated string which needs to be freed.
*/
char* wgaim_escape_dirsep( char* filename ) {
	int sepcount=0;
	char* ret=NULL;
	int cnt=0;

	ret = filename;
	while(*ret) {
		if(*ret == '\\')
			sepcount++;
		ret++;
	}
	ret = g_malloc0(strlen(filename) + sepcount + 1);
	while(*filename) {
		ret[cnt] = *filename;
		if(*filename == '\\')
			ret[++cnt] = '\\';
		filename++;
		cnt++;
	}
	ret[cnt] = '\0';
	return ret;
}

/*
 * This is a hack to circumvent the conflict between the
 * windows behaviour of gtk_window_get_pos and gtk_window_move, which
 * exists in GTK+ v2.2.0.  GTK+ documentation explains the following
 * should be true for gtk_window_get_pos:
 *   This function returns the position you need to pass to
 *   gtk_window_move() to keep window in its current position.
 * This is false (for windows). gtk_window_get_pos returns
 * client coords, whereas gtk_window_move accepts non-client coords.
 * Our solution, until this is fixed, is to anticipate the offset and
 * adjust the coordinates passed to gtk_window_move.
 */
void wgaim_gtk_window_move(GtkWindow *window, gint x, gint y) {
	LONG style,  extended_style;
	RECT trect;
	HWND hWnd;

	if(!GTK_WIDGET_REALIZED(GTK_WIDGET(window)))
		gtk_widget_realize(GTK_WIDGET(window));
	hWnd = GDK_WINDOW_HWND(GTK_WIDGET(window)->window);
	style = GetWindowLong(hWnd, GWL_STYLE);
	extended_style = GetWindowLong (hWnd, GWL_EXSTYLE);
	GetClientRect (hWnd, &trect);
	AdjustWindowRectEx (&trect, style, FALSE, extended_style);
	gtk_window_move(window, x + (-1 * trect.left), y + (-1 * trect.top));
}


/* Determine whether the specified dll contains the specified procedure.
   If so, load it (if not already loaded). */
FARPROC wgaim_find_and_loadproc( char* dllname, char* procedure ) {
	HMODULE hmod;
	int did_load=0;
	FARPROC proc = 0;

	if(!(hmod=GetModuleHandle(dllname))) {
		gaim_debug(GAIM_DEBUG_WARNING, "wgaim", "%s not found. Loading it..\n", dllname);
		if(!(hmod = LoadLibrary(dllname))) {
			gaim_debug(GAIM_DEBUG_ERROR, "wgaim", "Could not load: %s\n", dllname);
			return NULL;
		}
		else
			did_load = 1;
 	}

	if((proc=GetProcAddress(hmod, procedure))) {
		gaim_debug(GAIM_DEBUG_INFO, "wgaim", "This version of %s contains %s\n",
			     dllname, procedure);
		return proc;
	}
	else {
		gaim_debug(GAIM_DEBUG_WARNING, "wgaim", "Function %s not found in dll %s\n", 
			     procedure, dllname);
		if(did_load) {
			/* unload dll */
			FreeLibrary(hmod);
		}
		return NULL;
	}
}

/* Determine Gaim Paths during Runtime */

char* wgaim_install_dir(void) {
	HMODULE hmod;
	char* buf;

	hmod = GetModuleHandle(NULL);
	if( hmod == 0 ) {
		buf = g_win32_error_message( GetLastError() );
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim", "GetModuleHandle error: %s\n", buf);
		free(buf);
		return NULL;
	}
	if(GetModuleFileName( hmod, (char*)&install_dir, MAXPATHLEN ) == 0) {
		buf = g_win32_error_message( GetLastError() );
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim", "GetModuleFileName error: %s\n", buf);
		free(buf);
		return NULL;
	}
	buf = g_path_get_dirname( install_dir );
	strcpy( (char*)&install_dir, buf );
	free( buf );

	return (char*)&install_dir;
}

char* wgaim_lib_dir(void) {
	strcpy(lib_dir, wgaim_install_dir());
	strcat(lib_dir, G_DIR_SEPARATOR_S "plugins");
	return (char*)&lib_dir;
}

char* wgaim_locale_dir(void) {
	strcpy(locale_dir, wgaim_install_dir());
	strcat(locale_dir, G_DIR_SEPARATOR_S "locale");
	return (char*)&locale_dir;
}

/* Miscellaneous */

/* FlashWindowEx is only supported by Win98+ and WinNT5+. If its
   not supported we do it our own way */
void wgaim_im_blink(GtkWidget *window) {
	if(MyFlashWindowEx) {
		FLASHWINFO info;

		info.cbSize = sizeof(FLASHWINFO);
		info.hwnd = GDK_WINDOW_HWND(window->window);
		info.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		info.dwTimeout = 0;
		MyFlashWindowEx(&info);
	}
	else {
		WGAIM_FLASH_INFO *finfo = g_new0(WGAIM_FLASH_INFO, 1);

		/* Start Flashing window */
		finfo->t_handle = g_timeout_add(1000, 
						flash_window_cb, 
						GDK_WINDOW_HWND(window->window));
		finfo->sig_handler = g_signal_connect(G_OBJECT(window), 
						      "focus-in-event", 
						      G_CALLBACK(halt_flash_filter), finfo);
	}
}

int wgaim_gz_decompress(const char* in, const char* out) {
	gzFile fin;
	FILE *fout;
	char buf[1024];
	int ret;

	if((fin = gzopen(in, "rb"))) {
		if(!(fout = fopen(out, "wb"))) {
			gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_decompress", "Error opening file: %s\n", out);
			gzclose(fin);
			return 0;
		}
	}
	else {
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_decompress", "gzopen failed to open: %s\n", in);
		return 0;
	}

	while((ret=gzread(fin, buf, 1024))) {
		if(fwrite(buf, 1, ret, fout) < ret) {
			gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_decompress", "Error writing %d bytes to file\n", ret);
			gzclose(fin);
			fclose(fout);
			return 0;
		}
	}
	fclose(fout);
	gzclose(fin);

	if(ret < 0) {
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_decompress", "gzread failed while reading: %s\n", in);
		return 0;
	}

	return 1;
}

int wgaim_gz_untar(const char* filename, const char* destdir) {
	char tmpfile[_MAX_PATH];
	char template[]="wgaimXXXXXX";

	sprintf(tmpfile, "%s%s%s", g_get_tmp_dir(), G_DIR_SEPARATOR_S, _mktemp(template));
	if(wgaim_gz_decompress(filename, tmpfile)) {
		int ret;
		if(untar(tmpfile, destdir, UNTAR_FORCE | UNTAR_QUIET))
			ret=1;
		else {
			gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_untar", "Failure untaring %s\n", tmpfile);
			ret=0;
		}
		unlink(tmpfile);
		return ret;
	}
	else {
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim_gz_untar", "Failed to gz decompress %s\n", filename);
		return 0;
	}
}

/* Windows Initializations */

void wgaim_init(void) {
	WORD wVersionRequested;
	WSADATA wsaData;
	char* locale=0;
	char newenv[128];
	char* drmingw;

	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "wgaim_init\n");

	/* Load exception handler if we have it */
	drmingw = g_build_filename(wgaim_install_dir(), "exchndl.dll", NULL);
	LoadLibrary(drmingw);
	g_free(drmingw);

	load_winver_specific_procs();

	/* Initialize Wingaim systray icon */
	wgaim_systray_init();

	/*
	 *  Winsock init
	 */
	wVersionRequested = MAKEWORD( 2, 2 );

	WSAStartup( wVersionRequested, &wsaData );

	/* Confirm that the winsock DLL supports 2.2 */
	/* Note that if the DLL supports versions greater than
	   2.2 in addition to 2.2, it will still return 2.2 in 
	   wVersion since that is the version we requested. */

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) {
		gaim_debug(GAIM_DEBUG_WARNING, "wgaim", "Could not find a usable WinSock DLL.  Oh well.\n");
		WSACleanup( );
	}

	/* get default locale */
	locale = g_win32_getlocale();
	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "Language profile used: %s\n", locale);

	/* Aspell config */
	sprintf(newenv, "LANG=%s", locale);
	if(putenv(newenv)<0)
		gaim_debug(GAIM_DEBUG_WARNING, "wgaim", "putenv failed\n");
	g_free(locale);

	/* Disable PANGO UNISCRIBE (for GTK 2.2.0). This may not be necessary in the
	   future because there will most likely be a check to see if we need this,
	   but for now we need to set this in order to avoid poor performance for some 
	   windows machines.
	*/
	sprintf(newenv, "PANGO_WIN32_NO_UNISCRIBE=1");
	if(putenv(newenv)<0)
		gaim_debug(GAIM_DEBUG_WARNING, "wgaim", "putenv failed\n");

	/*
	 *  IdleTracker Initialization
	 */
	if(!wgaim_set_idlehooks())
		gaim_debug(GAIM_DEBUG_ERROR, "wgaim", "Failed to initialize idle tracker\n");

	wgaim_gtkspell_init();
}

/* Windows Cleanup */

void wgaim_cleanup(void) {
	gaim_debug(GAIM_DEBUG_INFO, "wgaim", "wgaim_cleanup\n");

	/* winsock cleanup */
	WSACleanup( );

	/* Idle tracker cleanup */
	wgaim_remove_idlehooks();
	
	/* Remove systray icon */
	wgaim_systray_cleanup();
}

/* DLL initializer */
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
	gaimdll_hInstance = hinstDLL;
	return TRUE;
}
