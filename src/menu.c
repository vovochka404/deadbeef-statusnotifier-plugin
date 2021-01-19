/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * menu.c
 * Copyright (C) 2014 Vladimir Perepechin <vovochka13@gmail.com>
 *
 * This file is part of deadbeef-statusnotifier-plugin.
 *
 * deadbeef-statusnotifier-plugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * deadbeef-statusnotifier-plugin is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * deadbeef-statusnotifier-plugin. If not, see http://www.gnu.org/licenses/
 */


#include "sni.h"

DbusmenuMenuitem *menu;
DbusmenuMenuitem *quit_item;
DbusmenuMenuitem *play_item;
DbusmenuMenuitem *stop_item;
DbusmenuMenuitem *next_item;
DbusmenuMenuitem *prev_item;
DbusmenuMenuitem *pref_item;
DbusmenuMenuitem *random_item;

DbusmenuMenuitem *pb_menu;
DbusmenuMenuitem *pb_order_linear;
DbusmenuMenuitem *pb_order_shuffle_tracks;
DbusmenuMenuitem *pb_order_random;
DbusmenuMenuitem *pb_order_shuffle_albums;
DbusmenuMenuitem *pb_loop_all;
DbusmenuMenuitem *pb_loop_none;
DbusmenuMenuitem *pb_loop_single;

GSList *pb_order;
GSList *pb_loop;

DbusmenuMenuitem*
create_menu_item (gchar *label, gchar *icon_name, SNIContextMenuItemType item_type);

/* Generate actoion procedure name */
#define SNI_CALLBACK_NAME(item) on_##item##_activate
/* Menu item callback declaration */
#define SNI_MENU_ITEM_CALLBACK(item)  \
    void SNI_CALLBACK_NAME(item) (DbusmenuMenuitem *menuitem)

/* Menu item simple messaging function macro */
static inline void
sni_callback_message (unsigned msg) {
    DB_functions_t *deadbeef = deadbeef_get_instance ();
    if (deadbeef == NULL)
        return;
    deadbeef->sendmessage (msg, 0, 0, 0);
}
#define SNI_MENU_ITEM_MESSAGE(item, MSG)                 \
    SNI_MENU_ITEM_CALLBACK (item) {                      \
        (void*) menuitem; /* unused messages cleanup */  \
        sni_callback_message(MSG);                       \
    }

SNI_MENU_ITEM_MESSAGE (quit, DB_EV_TERMINATE);
SNI_MENU_ITEM_MESSAGE (play, DB_EV_TOGGLE_PAUSE);
SNI_MENU_ITEM_MESSAGE (stop, DB_EV_STOP);
SNI_MENU_ITEM_MESSAGE (next, DB_EV_NEXT);
SNI_MENU_ITEM_MESSAGE (prev, DB_EV_PREV);
SNI_MENU_ITEM_MESSAGE (random, DB_EV_PLAY_RANDOM);

SNI_MENU_ITEM_CALLBACK (pref) {
    deadbeef_preferences_activate ();
}

SNI_MENU_ITEM_CALLBACK (playback_order) {
    DB_functions_t *deadbeef = deadbeef_get_instance ();
    if (deadbeef == NULL)
        return;
    guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (menuitem), "pb_data"));
    deadbeef->conf_set_int ("playback.order", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

SNI_MENU_ITEM_CALLBACK (playback_loop) {
    DB_functions_t *deadbeef = deadbeef_get_instance ();
    if (deadbeef == NULL)
        return;
    guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (menuitem), "pb_data"));
    deadbeef->conf_set_int ("playback.loop", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void check_list (gpointer item, gpointer data) {
    guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (item), "pb_data"));
    dbusmenu_menuitem_property_set_int (DBUSMENU_MENUITEM (item), DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                val == GPOINTER_TO_UINT (data) ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED :
                                                 DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
}

void
update_playback_controls (void) {
    DB_functions_t *deadbeef = deadbeef_get_instance ();
    if (deadbeef == NULL)
        return;
    guint32 order = deadbeef->conf_get_int ("playback.order", 0);
    guint32 loop  = deadbeef->conf_get_int ("playback.loop", 0);

    g_slist_foreach (pb_order, check_list, GUINT_TO_POINTER (order));
    g_slist_foreach (pb_loop, check_list, GUINT_TO_POINTER (loop));
}

DbusmenuMenuitem *
get_context_menu (void) {
    if (menu)
        return menu;

    menu = create_menu_item (_("Deadbeef"), NULL, SNI_MENU_ITEM_TYPE_COMMON);
    dbusmenu_menuitem_set_root (menu, TRUE);

    pb_menu = create_menu_item (_("Playback"), NULL, SNI_MENU_ITEM_TYPE_COMMON);

//    g_object_ref (menu);

    /** Common media controls **/

    quit_item = create_menu_item (_("Quit"), "application-exit", SNI_MENU_ITEM_TYPE_COMMON);
    play_item = create_menu_item (_("Play"), "media-playback-start", SNI_MENU_ITEM_TYPE_COMMON);
    stop_item = create_menu_item (_("Stop"), "media-playback-stop", SNI_MENU_ITEM_TYPE_COMMON);
    prev_item = create_menu_item (_("Previous"), "media-skip-backward", SNI_MENU_ITEM_TYPE_COMMON);
    next_item = create_menu_item (_("Next"), "media-skip-forward", SNI_MENU_ITEM_TYPE_COMMON);
    random_item = create_menu_item (_("Play Random"), NULL, SNI_MENU_ITEM_TYPE_COMMON);

    g_signal_connect (quit_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,   G_CALLBACK (SNI_CALLBACK_NAME(quit)), NULL);
    g_signal_connect (play_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,   G_CALLBACK (SNI_CALLBACK_NAME(play)), NULL);
    g_signal_connect (stop_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,   G_CALLBACK (SNI_CALLBACK_NAME(stop)), NULL);
    g_signal_connect (prev_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,   G_CALLBACK (SNI_CALLBACK_NAME(prev)), NULL);
    g_signal_connect (next_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,   G_CALLBACK (SNI_CALLBACK_NAME(next)), NULL);
    g_signal_connect (random_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(random)), NULL);

    dbusmenu_menuitem_child_append (menu, play_item);
    dbusmenu_menuitem_child_append (menu, stop_item);
    dbusmenu_menuitem_child_append (menu, prev_item);
    dbusmenu_menuitem_child_append (menu, next_item);
    dbusmenu_menuitem_child_append (menu, random_item);
    dbusmenu_menuitem_child_append (menu, create_menu_item (NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));

    /** Playback settings controls **/
    /** Playback order **/

    pb_order_linear         = create_menu_item (_("Linear"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_order_linear), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_LINEAR));
    pb_order = g_slist_append (pb_order, pb_order_linear);

    pb_order_shuffle_tracks = create_menu_item (_("Shuffle tracks"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_order_shuffle_tracks), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_SHUFFLE_TRACKS));
    pb_order = g_slist_append (pb_order, pb_order_shuffle_tracks);

    pb_order_shuffle_albums = create_menu_item (_("Shuffle albums"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_order_shuffle_albums), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_SHUFFLE_ALBUMS));
    pb_order = g_slist_append (pb_order, pb_order_shuffle_albums);

    pb_order_random         = create_menu_item (_("Random"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_order_random), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_RANDOM));
    pb_order = g_slist_append (pb_order, pb_order_random);

    /** Playback loop **/

    pb_loop_all         = create_menu_item (_("Loop all"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_loop_all), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_LOOP_ALL));
    pb_loop = g_slist_append (pb_loop, pb_loop_all);

    pb_loop_single      = create_menu_item (_("Loop single song"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_loop_single), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_LOOP_SINGLE));
    pb_loop = g_slist_append (pb_loop, pb_loop_single);

    pb_loop_none        = create_menu_item (_("Don't loop"), NULL, SNI_MENU_ITEM_TYPE_RADIO);
    g_object_set_data (G_OBJECT (pb_loop_none), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_NOLOOP));
    pb_loop = g_slist_append (pb_loop, pb_loop_none);

    update_playback_controls ();

    g_signal_connect (pb_order_linear, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_order)), NULL);
    g_signal_connect (pb_order_shuffle_tracks, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_order)), NULL);
    g_signal_connect (pb_order_shuffle_albums, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_order)), NULL);
    g_signal_connect (pb_order_random, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_order)), NULL);
    g_signal_connect (pb_loop_all, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_loop)), NULL);
    g_signal_connect (pb_loop_single, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_loop)), NULL);
    g_signal_connect (pb_loop_none, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (SNI_CALLBACK_NAME(playback_loop)), NULL);

    dbusmenu_menuitem_child_append (pb_menu, pb_order_linear);
    dbusmenu_menuitem_child_append (pb_menu, pb_order_shuffle_tracks);
    dbusmenu_menuitem_child_append (pb_menu, pb_order_shuffle_albums);
    dbusmenu_menuitem_child_append (pb_menu, pb_order_random);
    dbusmenu_menuitem_child_append (pb_menu, create_menu_item (NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));
    dbusmenu_menuitem_child_append (pb_menu, pb_loop_all);
    dbusmenu_menuitem_child_append (pb_menu, pb_loop_single);
    dbusmenu_menuitem_child_append (pb_menu, pb_loop_none);

    dbusmenu_menuitem_child_append (menu, pb_menu);
    dbusmenu_menuitem_child_append (menu, create_menu_item (NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));

    if (deadbeef_preferences_available ()) {
        pref_item = create_menu_item (_("Preferences"), "configure", SNI_MENU_ITEM_TYPE_COMMON);
        g_signal_connect (pref_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, G_CALLBACK (on_pref_activate), NULL);
        dbusmenu_menuitem_child_append (menu, pref_item);
    }
    dbusmenu_menuitem_child_append (menu, quit_item);

    return menu;
}

DbusmenuMenuitem*
create_menu_item (gchar *label, gchar *icon_name, SNIContextMenuItemType item_type) {
    DbusmenuMenuitem *item;

    item = dbusmenu_menuitem_new ();
    dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_LABEL, label);

    if (item_type == SNI_MENU_ITEM_TYPE_SEPARATOR)
        dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_TYPE, "separator");
    else if (item_type == SNI_MENU_ITEM_TYPE_RADIO)
        dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
    else if (item_type == SNI_MENU_ITEM_TYPE_CHECKBOX)
        dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);

    if (icon_name)
        dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_ICON_NAME, icon_name);

    dbusmenu_menuitem_property_set_bool (item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool (item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);


    return item;
}

DbusmenuMenuitem *
get_context_menu_item (SNIContextMenuItem item) {
    get_context_menu ();

    switch (item) {
        case SNI_MENU_ITEM_PLAY:
            return play_item;
        case SNI_MENU_ITEM_STOP:
            return stop_item;
        case SNI_MENU_ITEM_NEXT:
            return next_item;
        case SNI_MENU_ITEM_PREV:
            return prev_item;
        case SNI_MENU_ITEM_RANDOM:
            return random_item;
        case SNI_MENU_ITEM_QUIT:
            return quit_item;
    }
    return NULL;
}
