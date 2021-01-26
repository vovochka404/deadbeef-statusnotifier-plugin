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

typedef enum {
    SNI_FLAG_AUTOED = 1 << 0,
    SNI_FLAG_LOADED = 1 << 1,
} SNIFlags;

DbusmenuMenuitem *
get_context_menu(void);

DbusmenuMenuitem *
get_context_menu_item(SNIContextMenuItem item);

void
update_playback_controls(void);

void
deadbeef_toggle_play_pause(void);

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

#define sni_free_null(X)                                                                           \
    do {                                                                                           \
        free(X);                                                                                   \
        X = NULL;                                                                                  \
    } while (0);

#endif
