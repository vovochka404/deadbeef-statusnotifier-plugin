#include "sni.h"

typedef struct {
    StatusNotifier *icon;

    guint timer;
    uintptr_t lock;
} sni_state_t;

static sni_state_t *sni_state = NULL;

void
sni_update_info(sni_state_t *st, int state) {
    // TODO FIXME Reduce more changes
    sni_update_status(state);
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

        sni_state->lock = deadbeef->mutex_create();
        if (sni_state->lock == 0) {
            sni_free_null(sni_state);
            return -1;
        }

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
        deadbeef->mutex_free(sni_state->lock);
        sni_free_null(sni_state);
    }
}
