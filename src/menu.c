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
GtkWidget *random_item;

GtkWidget*
create_menu_item (gchar *label, gchar *icon_name, gboolean is_checkbox, gboolean is_separator);

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

GtkWidget *
get_context_menu (void) {
    if (menu)
        return menu;

    menu = gtk_menu_new ();
    g_object_ref (menu);

    quit_item = create_menu_item (_("Quit"), "application-exit", FALSE, FALSE);
    play_item = create_menu_item (_("Play"), "media-playback-start", FALSE, FALSE);
    stop_item = create_menu_item (_("Stop"), "media-playback-stop", FALSE, FALSE);
    prev_item = create_menu_item (_("Previous"), "media-skip-backward", FALSE, FALSE);
    next_item = create_menu_item (_("Next"), "media-skip-forward", FALSE, FALSE);
    random_item = create_menu_item (_("Play Random"), "", FALSE, FALSE);

    g_signal_connect(quit_item, "activate", G_CALLBACK (on_quit_activate), NULL);
    g_signal_connect(play_item, "activate", G_CALLBACK (on_play_activate), NULL);
    g_signal_connect(stop_item, "activate", G_CALLBACK (on_stop_activate), NULL);
    g_signal_connect(prev_item, "activate", G_CALLBACK (on_prev_activate), NULL);
    g_signal_connect(next_item, "activate", G_CALLBACK (on_next_activate), NULL);
    g_signal_connect(random_item, "activate", G_CALLBACK (on_random_activate), NULL);

    gtk_container_add (GTK_CONTAINER (menu), play_item);
    gtk_container_add (GTK_CONTAINER (menu), stop_item);
    gtk_container_add (GTK_CONTAINER (menu), prev_item);
    gtk_container_add (GTK_CONTAINER (menu), next_item);
    gtk_container_add (GTK_CONTAINER (menu), random_item);
    gtk_container_add (GTK_CONTAINER (menu), create_menu_item (NULL, NULL, FALSE, TRUE));
    gtk_container_add (GTK_CONTAINER (menu), quit_item);

    return menu;
}

GtkWidget*
create_menu_item (gchar *label, gchar *icon_name, gboolean is_checkbox, gboolean is_separator) {
    GtkWidget *item;
    GtkWidget *icon;

    if (is_separator) {
        item = gtk_separator_menu_item_new ();
        gtk_widget_show(item);
        return item;
    }

    if (is_checkbox)
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