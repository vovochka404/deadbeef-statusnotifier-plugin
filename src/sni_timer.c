/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni_timer.c
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
    StatusNotifier *icon;

    unsigned playback;
    DB_playItem_t *item;

    guint timer;
} sni_state_t;

static sni_state_t *sni_state = NULL;

void
sni_update_info(sni_state_t *st, unsigned state) {

    DB_playItem_t *tmp = deadbeef->streamer_get_playing_track();

    if ((st->playback != state) || (st->item != tmp)) {
        st->playback = state;
        if (st->item)
            deadbeef->pl_item_unref(st->item);
        st->item = tmp;

        g_debug("Update status. State: %u\tItem:%p\n", st->playback, st->item);
        sni_update_status(state);
    } else {
        if (tmp)
            deadbeef->pl_item_unref(tmp);
    }
}

static gboolean
callback_timer_status_update(gpointer ctx) {

    if (sni_flag_get(SNI_FLAG_LOADED) == FALSE)
        return TRUE;

    sni_state_t *sni_ctx = (sni_state_t *)ctx;
    if (sni_ctx == NULL)
        return FALSE;

    int state = playback_state_active_waiting();
    if (state < 0) {
        return FALSE;
    }

    sni_update_info(sni_ctx, state);

    return TRUE;
}

gboolean
sni_timer_enabled(void) {
    return (sni_state) ? TRUE : FALSE;
}

int
sni_timer_init(StatusNotifier *icon, guint interval) {
    if (sni_state == NULL) {

        sni_state = calloc(1, sizeof(sni_state_t));
        if (sni_state == NULL)
            return -1;

        sni_state->icon = icon;
        // clang-format off
        sni_state->timer = g_timeout_add_full( G_PRIORITY_DEFAULT, interval,
                                               callback_timer_status_update,
                                               (gpointer)sni_state, NULL);
        // clang-format on
    }
    return 0;
}

void
sni_timer_free(void) {
    if (sni_state) {
        g_source_remove(sni_state->timer);

        if (sni_state->item)
            deadbeef->pl_item_unref(sni_state->item);

        sni_free_null(sni_state);
    }
}
