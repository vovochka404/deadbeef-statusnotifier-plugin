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

GtkWidget *menu;
GtkWidget *quit_item;
GtkWidget *play_item;
GtkWidget *stop_item;
GtkWidget *next_item;
GtkWidget *prev_item;
GtkWidget *pref_item;
GtkWidget *random_item;

GtkWidget *pb_menu;
GtkWidget *pb_order_linear;
GtkWidget *pb_order_shuffle_tracks;
GtkWidget *pb_order_random;
GtkWidget *pb_order_shuffle_albums;
GtkWidget *pb_loop_all;
GtkWidget *pb_loop_none;
GtkWidget *pb_loop_single;

GSList *pb_order;
GSList *pb_loop;

GtkWidget*
create_menu_item (gchar *label, gchar *icon_name, int is_checkbox, gboolean is_separator);

void
on_quit_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef->sendmessage(DB_EV_TERMINATE, 0, 0, 0);
}

void
on_play_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef_toggle_play_pause();
}

void
on_stop_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
}

void
on_next_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
}

void
on_prev_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
}

void
on_random_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef->sendmessage(DB_EV_PLAY_RANDOM, 0, 0, 0);
}

void
on_pref_activate(GtkMenuItem *menuitem,
               gpointer     user_data) {
    deadbeef_preferences_activate();
}

void
on_playback_order_activate(GtkMenuItem *menuitem) {
    guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (menuitem), "pb_data"));
    deadbeef->conf_set_int ("playback.order", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_playback_loop_activate(GtkMenuItem *menuitem) {
    guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (menuitem), "pb_data"));
    deadbeef->conf_set_int ("playback.loop", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
update_playback_controls() {
    guint32 order = deadbeef-> conf_get_int("playback.order", 0);
    guint32 loop  = deadbeef-> conf_get_int("playback.loop", 0);

    void check_list(gpointer item, gpointer data) {
        guint32 val = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (item), "pb_data"));
        if (val == GPOINTER_TO_UINT (data)) {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
        }
    }

    g_slist_foreach(pb_order, check_list, GUINT_TO_POINTER(order));
    g_slist_foreach(pb_loop, check_list, GUINT_TO_POINTER(loop));
}

GtkWidget *
get_context_menu (void) {
    if (menu)
        return menu;

    menu = gtk_menu_new ();
    pb_menu = gtk_menu_new ();
    g_object_ref (menu);

    /** Common media controls **/

    quit_item = create_menu_item (_("Quit"), "application-exit", 0, FALSE);
    play_item = create_menu_item (_("Play"), "media-playback-start", 0, FALSE);
    stop_item = create_menu_item (_("Stop"), "media-playback-stop", 0, FALSE);
    prev_item = create_menu_item (_("Previous"), "media-skip-backward", 0, FALSE);
    next_item = create_menu_item (_("Next"), "media-skip-forward", 0, FALSE);
    random_item = create_menu_item (_("Play Random"), NULL, 0, FALSE);

    g_signal_connect (quit_item, "activate", G_CALLBACK (on_quit_activate), NULL);
    g_signal_connect (play_item, "activate", G_CALLBACK (on_play_activate), NULL);
    g_signal_connect (stop_item, "activate", G_CALLBACK (on_stop_activate), NULL);
    g_signal_connect (prev_item, "activate", G_CALLBACK (on_prev_activate), NULL);
    g_signal_connect (next_item, "activate", G_CALLBACK (on_next_activate), NULL);
    g_signal_connect (random_item, "activate", G_CALLBACK (on_random_activate), NULL);

    gtk_container_add (GTK_CONTAINER (menu), play_item);
    gtk_container_add (GTK_CONTAINER (menu), stop_item);
    gtk_container_add (GTK_CONTAINER (menu), prev_item);
    gtk_container_add (GTK_CONTAINER (menu), next_item);
    gtk_container_add (GTK_CONTAINER (menu), random_item);
    gtk_container_add (GTK_CONTAINER (menu), create_menu_item (NULL, NULL, 0, TRUE));

    /** Playback settings controls **/
    /** Playback order **/

    pb_order_linear         = create_menu_item (_("Linear"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM (pb_order_linear), pb_order);
    pb_order = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (pb_order_linear));
    g_object_set_data (G_OBJECT (pb_order_linear), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_LINEAR));

    pb_order_shuffle_tracks = create_menu_item (_("Shuffle tracks"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM(pb_order_shuffle_tracks), pb_order);
    pb_order = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(pb_order_shuffle_tracks));
    g_object_set_data (G_OBJECT (pb_order_shuffle_tracks), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_SHUFFLE_TRACKS));

    pb_order_shuffle_albums = create_menu_item (_("Shuffle albums"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM(pb_order_shuffle_albums), pb_order);
    pb_order = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(pb_order_shuffle_albums));
    g_object_set_data (G_OBJECT (pb_order_shuffle_albums), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_SHUFFLE_ALBUMS));

    pb_order_random         = create_menu_item (_("Random"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM(pb_order_random), pb_order);
    pb_order = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(pb_order_random));
    g_object_set_data (G_OBJECT (pb_order_random), "pb_data", GUINT_TO_POINTER (PLAYBACK_ORDER_RANDOM));

    /** Playback loop **/

    pb_loop_all         = create_menu_item (_("Loop all"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM (pb_loop_all), pb_loop);
    pb_loop = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (pb_loop_all));
    g_object_set_data (G_OBJECT (pb_loop_all), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_LOOP_ALL));

    pb_loop_single      = create_menu_item (_("Loop single song"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM (pb_loop_single), pb_loop);
    pb_loop = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (pb_loop_single));
    g_object_set_data (G_OBJECT (pb_loop_single), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_LOOP_SINGLE));

    pb_loop_none        = create_menu_item (_("Don't loop"), NULL, 2, FALSE);
    gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM (pb_loop_none), pb_loop);
    pb_loop = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (pb_loop_none));
    g_object_set_data (G_OBJECT (pb_loop_none), "pb_data", GUINT_TO_POINTER (PLAYBACK_MODE_NOLOOP));

    update_playback_controls();

    g_signal_connect (pb_order_linear, "activate", G_CALLBACK (on_playback_order_activate), NULL);
    g_signal_connect (pb_order_shuffle_tracks, "activate", G_CALLBACK (on_playback_order_activate), NULL);
    g_signal_connect (pb_order_shuffle_albums, "activate", G_CALLBACK (on_playback_order_activate), NULL);
    g_signal_connect (pb_order_random, "activate", G_CALLBACK (on_playback_order_activate), NULL);
    g_signal_connect (pb_loop_all, "activate", G_CALLBACK (on_playback_loop_activate), NULL);
    g_signal_connect (pb_loop_single, "activate", G_CALLBACK (on_playback_loop_activate), NULL);
    g_signal_connect (pb_loop_none, "activate", G_CALLBACK (on_playback_loop_activate), NULL);

    gtk_container_add (GTK_CONTAINER (pb_menu), pb_order_linear);
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_order_shuffle_tracks);
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_order_shuffle_albums);
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_order_random);
    gtk_container_add (GTK_CONTAINER (pb_menu), create_menu_item (NULL, NULL, 0, TRUE));
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_loop_all);
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_loop_single);
    gtk_container_add (GTK_CONTAINER (pb_menu), pb_loop_none);

    GtkWidget *playback_item = create_menu_item (_("Playback"), NULL, 0, FALSE);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM(playback_item), pb_menu);

    gtk_container_add (GTK_CONTAINER (menu), playback_item);
    gtk_container_add (GTK_CONTAINER (menu), create_menu_item (NULL, NULL, 0, TRUE));

    if (deadbeef_preferences_available()) {
        pref_item = create_menu_item(_("Preferences"), "configure", 0, FALSE);
        g_signal_connect(pref_item, "activate", G_CALLBACK(on_pref_activate), NULL);
        gtk_container_add (GTK_CONTAINER (menu), pref_item);
    }
    gtk_container_add (GTK_CONTAINER (menu), quit_item);

    return menu;
}

GtkWidget*
create_menu_item (gchar *label, gchar *icon_name, int is_checkbox, gboolean is_separator) {
    GtkWidget *item;
    GtkWidget *icon;

    if (is_separator) {
        item = gtk_separator_menu_item_new ();
        gtk_widget_show(item);
        return item;
    }

    if (is_checkbox && is_checkbox == 2)
        item = gtk_radio_menu_item_new_with_mnemonic (NULL, label);
    else if (is_checkbox)
        item = gtk_check_menu_item_new_with_mnemonic (label);
    else if(icon_name)
        item = gtk_image_menu_item_new_with_mnemonic (label);
    else
        item = gtk_menu_item_new_with_mnemonic (label);

    gtk_widget_show(item);

    if (icon_name) {
        icon = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
        gtk_widget_show (icon);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
    }

    return item;
}

GtkWidget *
get_context_menu_item (SNIContextMenuItem item) {
    get_context_menu();

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