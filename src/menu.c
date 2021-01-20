/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * menu.c
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

#include "sni.h"

typedef struct {
    DbusmenuMenuitem *menu;
    DbusmenuMenuitem *item_quit;
    DbusmenuMenuitem *item_play;
    DbusmenuMenuitem *item_stop;
    DbusmenuMenuitem *item_next;
    DbusmenuMenuitem *item_prev;
    DbusmenuMenuitem *item_pref;
    DbusmenuMenuitem *item_random;

    DbusmenuMenuitem *pb_menu;
    struct {
        DbusmenuMenuitem *item_linear;
        DbusmenuMenuitem *item_shuffle_tracks;
        DbusmenuMenuitem *item_random;
        DbusmenuMenuitem *item_shuffle_albums;

        GSList *list;
    } pb_order;
    struct {
        DbusmenuMenuitem *item_all;
        DbusmenuMenuitem *item_none;
        DbusmenuMenuitem *item_single;

        GSList *list;
    } pb_loop;

    uintptr_t pb_lock;
} sni_menu_t;

static sni_menu_t *sm = NULL;
static DB_functions_t *deadbeef;

/* Generate actoion procedure name */
#define SNI_CALLBACK_NAME(item) on_##item##_activate
/* Menu item callback declaration */
#define SNI_MENU_ITEM_CALLBACK(item) void SNI_CALLBACK_NAME(item)(DbusmenuMenuitem * menuitem)

/* Menu item simple messaging function macro */
static inline void
sni_callback_message(unsigned msg) {
    if (deadbeef == NULL)
        return;
    deadbeef->sendmessage(msg, 0, 0, 0);
}
#define SNI_MENU_ITEM_MESSAGE(item, MSG)                                                           \
    SNI_MENU_ITEM_CALLBACK(item) {                                                                 \
        (void *)menuitem; /* unused messages cleanup */                                            \
        sni_callback_message(MSG);                                                                 \
    }

SNI_MENU_ITEM_MESSAGE(quit, DB_EV_TERMINATE);
SNI_MENU_ITEM_MESSAGE(play, DB_EV_TOGGLE_PAUSE);
SNI_MENU_ITEM_MESSAGE(stop, DB_EV_STOP);
SNI_MENU_ITEM_MESSAGE(next, DB_EV_NEXT);
SNI_MENU_ITEM_MESSAGE(prev, DB_EV_PREV);
SNI_MENU_ITEM_MESSAGE(random, DB_EV_PLAY_RANDOM);

SNI_MENU_ITEM_CALLBACK(pref) { deadbeef_preferences_activate(); }

SNI_MENU_ITEM_CALLBACK(playback_order) {
    DB_functions_t *deadbeef = deadbeef_get_instance();
    if (deadbeef == NULL)
        return;
    guint32 val = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "pb_data"));
    deadbeef->conf_set_int("playback.order", val);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

SNI_MENU_ITEM_CALLBACK(playback_loop) {
    DB_functions_t *deadbeef = deadbeef_get_instance();
    if (deadbeef == NULL)
        return;
    guint32 val = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "pb_data"));
    deadbeef->conf_set_int("playback.loop", val);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
check_list(gpointer item, gpointer data) {
    guint32 val = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(item), "pb_data"));
    dbusmenu_menuitem_property_set_int(DBUSMENU_MENUITEM(item), DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                       val == GPOINTER_TO_UINT(data)
                                           ? DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED
                                           : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
}

void
update_playback_controls(void) {
    if (sm == NULL)
        return;

    deadbeef->mutex_lock(sm->pb_lock);

    guint32 order = deadbeef->conf_get_int("playback.order", 0);
    guint32 loop = deadbeef->conf_get_int("playback.loop", 0);

    g_slist_foreach(sm->pb_order.list, check_list, GUINT_TO_POINTER(order));
    g_slist_foreach(sm->pb_loop.list, check_list, GUINT_TO_POINTER(loop));

    deadbeef->mutex_unlock(sm->pb_lock);
}

static DbusmenuMenuitem *
create_menu_item(gchar *label, gchar *icon_name, SNIContextMenuItemType item_type) {
    DbusmenuMenuitem *item;

    item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, label);

    switch (item_type) {
    case SNI_MENU_ITEM_TYPE_SEPARATOR:
        dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TYPE, "separator");
        break;
    case SNI_MENU_ITEM_TYPE_RADIO:
        dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                       DBUSMENU_MENUITEM_TOGGLE_RADIO);
        break;
    case SNI_MENU_ITEM_TYPE_CHECKBOX:
        dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                       DBUSMENU_MENUITEM_TOGGLE_CHECK);
        break;
    case SNI_MENU_ITEM_TYPE_COMMON:
    default:
        break;
    }

    if (icon_name)
        dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_ICON_NAME, icon_name);

    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);

    return item;
}

#define CREATE_PLAYBACK_ITEM(menu, name, label, mode, callback)                                    \
    do {                                                                                           \
        sm->pb_##menu.item_##name = create_menu_item(label, NULL, SNI_MENU_ITEM_TYPE_RADIO);       \
        g_object_set_data(G_OBJECT(sm->pb_##menu.item_##name), "pb_data", GUINT_TO_POINTER(mode)); \
        sm->pb_##menu.list = g_slist_append(sm->pb_##menu.list, sm->pb_##menu.item_##name);        \
                                                                                                   \
        g_signal_connect(sm->pb_##menu.item_##name, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,       \
                         G_CALLBACK(callback), NULL);                                              \
                                                                                                   \
        dbusmenu_menuitem_child_append(sm->pb_menu, sm->pb_##menu.item_##name);                    \
    } while (0)

static inline DbusmenuMenuitem *
create_menu_playback(void) {
    sm->pb_menu = create_menu_item(_("Playback"), NULL, SNI_MENU_ITEM_TYPE_COMMON);

    CREATE_PLAYBACK_ITEM(order, linear, _("Linear"), PLAYBACK_ORDER_LINEAR,
                         SNI_CALLBACK_NAME(playback_order));
    CREATE_PLAYBACK_ITEM(order, shuffle_tracks, _("Shuffle tracks"), PLAYBACK_ORDER_SHUFFLE_TRACKS,
                         SNI_CALLBACK_NAME(playback_order));
    CREATE_PLAYBACK_ITEM(order, shuffle_albums, _("Shuffle albums"), PLAYBACK_ORDER_SHUFFLE_ALBUMS,
                         SNI_CALLBACK_NAME(playback_order));
    CREATE_PLAYBACK_ITEM(order, random, _("Random"), PLAYBACK_ORDER_RANDOM,
                         SNI_CALLBACK_NAME(playback_order));

    dbusmenu_menuitem_child_append(sm->pb_menu,
                                   create_menu_item(NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));

    CREATE_PLAYBACK_ITEM(loop, all, _("Loop all"), PLAYBACK_MODE_LOOP_ALL,
                         SNI_CALLBACK_NAME(playback_loop));
    CREATE_PLAYBACK_ITEM(loop, single, _("Loop single song"), PLAYBACK_MODE_LOOP_SINGLE,
                         SNI_CALLBACK_NAME(playback_loop));
    CREATE_PLAYBACK_ITEM(loop, none, _("Don't loop"), PLAYBACK_MODE_NOLOOP,
                         SNI_CALLBACK_NAME(playback_loop));

    update_playback_controls();

    return sm->pb_menu;
}

#undef create_playback_item

#define CREATE_CONTEXT_ITEM(name, label, icon, callback)                                           \
    do {                                                                                           \
        sm->item_##name = create_menu_item(label, icon, SNI_MENU_ITEM_TYPE_COMMON);                \
        g_signal_connect(sm->item_##name, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,                 \
                         G_CALLBACK(callback), NULL);                                              \
        dbusmenu_menuitem_child_append(sm->menu, sm->item_##name);                                 \
    } while (0)

static inline DbusmenuMenuitem *
create_context_menu(void) {

    sm->menu = create_menu_item(_("Deadbeef"), NULL, SNI_MENU_ITEM_TYPE_COMMON);
    dbusmenu_menuitem_set_root(sm->menu, TRUE);

    /** Common media controls **/

    CREATE_CONTEXT_ITEM(play, _("Play"), "media-playback-start", SNI_CALLBACK_NAME(play));
    CREATE_CONTEXT_ITEM(stop, _("Stop"), "media-playback-stop", SNI_CALLBACK_NAME(stop));
    CREATE_CONTEXT_ITEM(prev, _("Previous"), "media-skip-backward", SNI_CALLBACK_NAME(prev));
    CREATE_CONTEXT_ITEM(next, _("Next"), "media-skip-forward", SNI_CALLBACK_NAME(next));
    CREATE_CONTEXT_ITEM(random, _("Play Random"), NULL, SNI_CALLBACK_NAME(random));

    dbusmenu_menuitem_child_append(sm->menu,
                                   create_menu_item(NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));
    /** Playback settings controls **/
    dbusmenu_menuitem_child_append(sm->menu, create_menu_playback());
    dbusmenu_menuitem_child_append(sm->menu,
                                   create_menu_item(NULL, NULL, SNI_MENU_ITEM_TYPE_SEPARATOR));

    if (deadbeef_preferences_available())
        CREATE_CONTEXT_ITEM(pref, _("Preferences"), "configure", SNI_CALLBACK_NAME(pref));

    CREATE_CONTEXT_ITEM(quit, _("Quit"), "application-exit", SNI_CALLBACK_NAME(quit));

    return sm->menu;
}

#undef create_context_menu

DbusmenuMenuitem *
get_context_menu_item(SNIContextMenuItem item) {
    if (sm) {
        switch (item) {
        case SNI_MENU_ITEM_PLAY:
            return sm->item_play;
        case SNI_MENU_ITEM_STOP:
            return sm->item_stop;
        case SNI_MENU_ITEM_NEXT:
            return sm->item_next;
        case SNI_MENU_ITEM_PREV:
            return sm->item_prev;
        case SNI_MENU_ITEM_RANDOM:
            return sm->item_random;
        case SNI_MENU_ITEM_QUIT:
            return sm->item_quit;
        }
    }
    return NULL;
}

DbusmenuMenuitem *
get_context_menu(void) {
    return (sm) ? sm->menu : NULL;
}

int
sni_context_menu_create(void) {
    deadbeef = deadbeef_get_instance();
    if (deadbeef) {
        sm = calloc(1, sizeof(sni_menu_t));
        if (sm) {
            sm->pb_lock = deadbeef->mutex_create();
            if (sm->pb_lock) {
                create_context_menu();
                return 0;
            } else {
                sni_free_null(sm);
            }
        }
    }
    return -1;
}

void
sni_context_menu_release(void) {
    if (sm) {
        deadbeef->mutex_free(sm->pb_lock);
        sni_free_null(sm);
    }
}
