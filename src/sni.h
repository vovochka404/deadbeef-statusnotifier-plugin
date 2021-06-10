/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni.h
 * Copyright (C) 2014 Vladimir Perepechin <vovochka13@gmail.com>
 *
 * This file is part of deadbeef-statusnotifier-plugin.
 *
 * deadbeef-statusnotifier-plugin is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * deadbeef-statusnotifier-plugin is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * deadbeef-statusnotifier-plugin. If not, see http://www.gnu.org/licenses/
 */

#ifndef SNI_H
#define SNI_H

#if ENABLE_NLS
    #include <libintl.h>
    #define PACKAGE "deadbeef"
    #define _(String) dgettext(PACKAGE, String)
#else
    #define _(String) (String)
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
    #define SNI_EXPORT_FUNC __attribute__((visibility("default")))
#else
    #define SNI_EXPORT_FUNC
#endif

#include <stdlib.h>

#include <gtk/gtk.h>
#include <libdbusmenu-glib/menuitem.h>

#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

#include <extras/statusnotifier/src/statusnotifier.h>

DB_functions_t *
deadbeef_get_instance(void);

typedef enum {
    SNI_MENU_ITEM_PLAY,
    SNI_MENU_ITEM_STOP,
    SNI_MENU_ITEM_NEXT,
    SNI_MENU_ITEM_PREV,
    SNI_MENU_ITEM_RANDOM,
    SNI_MENU_ITEM_QUIT
} SNIContextMenuItem;

typedef enum {
    SNI_MENU_ITEM_TYPE_COMMON,
    SNI_MENU_ITEM_TYPE_CHECKBOX,
    SNI_MENU_ITEM_TYPE_RADIO,
    SNI_MENU_ITEM_TYPE_SEPARATOR
} SNIContextMenuItemType;

typedef enum { SNI_FLAG_ENABLED = 0, SNI_FLAG_LOADED = 1, SNI_FLAG_HIDDEN = 2 } SNIFlags;

enum {
    SNI_STATE_TOOGLE_PLAY = 0,
    SNI_STATE_TOOGLE_PAUSE = 1,
    SNI_STATE_TOOGLE_STOP = 2,
};

#define SNI_OPTION_ENABLE "sni.enabled"
#define SNI_OPTION_ICON_MINIMIZE "sni.icon_minimize_action"
#define SNI_OPTION_ICON_OVERLAY "sni.icon_overlay"
#define SNI_OPTION_ICON_REPLACE "sni.icon_replace"
#define SNI_OPTION_TOOLTIP_ENABLE "sni.tooltip_enable"
#define SNI_OPTION_TOOLTIP_ISTEXT "sni.tooltip_istext"
#define SNI_OPTION_TOOLTIP_ICON "sni.tooltip_icon"
#define SNI_OPTION_MENU_PLAYBACK "sni.menu_playback"
#define SNI_OPTION_MENU_TOGGLE "sni.menu_wmtoggle"
#define SNI_OPTION_VOLUME_HORIZONTAL "sni.volume_hdirect"
#define SNI_OPTION_VOLUME_INVERSE "sni.volume_inverse"
#define SNI_OPTION_TIMEOUT "sni.timeout"

DbusmenuMenuitem *
get_context_menu(void);

DbusmenuMenuitem *
get_context_menu_item(SNIContextMenuItem item);

gboolean
deadbeef_window_is_visible(void);

void
deadbeef_toogle_window(void);

void
update_window_controls(void);

void
update_playback_controls(void);

void
update_play_controls(int play);

gboolean
deadbeef_preferences_available(void);

void
deadbeef_preferences_activate(void);

gboolean
deadbeef_help_available(void);

void
deadbeef_help_activate(void);

int
sni_context_menu_create(void);

void
sni_context_menu_release(void);

gboolean
sni_flag_get(uint32_t code);

void
sni_flag_set(uint32_t code);

void
sni_flag_unset(uint32_t code);

#define sni_free_null(X)                                                                           \
    do {                                                                                           \
        free(X);                                                                                   \
        X = NULL;                                                                                  \
    } while (0);

#endif
