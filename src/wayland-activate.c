/*
 * wayland-activate.c
 * Copyright (C) 2026 Vladimir Perepechin <vovochka13@gmail.com>
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

#include "wayland-activate.h"

#if defined(GDK_WINDOWING_WAYLAND)

#include <gdk/gdkwayland.h>
#include <wayland-client.h>
#include "xdg-activation-v1-client-protocol.h"

#include <string.h>
#include <stdlib.h>

/*
 * Per-activation context.
 * Heap-allocated, freed in done callback.
 */

struct activation_ctx {
    struct xdg_activation_v1 *activation_manager;
    struct wl_seat *seat;
    struct wl_surface *target_surface;
};

/*
 * wl_registry listener.
 * Discovers globals for this call only (no caching).
 */

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version) {
    struct activation_ctx *ctx = (struct activation_ctx *)data;
    (void)registry;
    (void)version;

    if (strcmp(interface, "xdg_activation_v1") == 0 && !ctx->activation_manager) {
        ctx->activation_manager = wl_registry_bind(registry, name,
                                                   &xdg_activation_v1_interface, 1);
    } else if (strcmp(interface, "wl_seat") == 0 && !ctx->seat) {
        ctx->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

/*
 * xdg_activation_token_v1 listener.
 */

static void
activation_token_handle_done(void *data,
                             struct xdg_activation_token_v1 *token,
                             const char *str) {
    struct activation_ctx *ctx = (struct activation_ctx *)data;

    if (ctx->activation_manager && ctx->target_surface && str) {
        xdg_activation_v1_activate(ctx->activation_manager, str,
                                   ctx->target_surface);
    }

    xdg_activation_token_v1_destroy(token);
    xdg_activation_v1_destroy(ctx->activation_manager);
    if (ctx->seat)
        wl_seat_destroy(ctx->seat);
    free(ctx);
}

static const struct xdg_activation_token_v1_listener activation_token_listener = {
    .done = activation_token_handle_done,
};

/*
 * Public API.
 */

void
gdk_wayland_window_activate(GdkWindow *window, guint32 timestamp) {
    struct activation_ctx *ctx;
    struct wl_display *wl_dpy;
    struct wl_registry *registry;
    struct xdg_activation_token_v1 *token;

    g_return_if_fail(GDK_IS_WINDOW(window));

    GdkDisplay *display = gdk_window_get_display(window);
    if (!GDK_IS_WAYLAND_DISPLAY(display))
        return;

    wl_dpy = gdk_wayland_display_get_wl_display(display);
    if (!wl_dpy)
        return;

    ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return;

    /* Discover globals via wl_registry (fresh each time) */
    registry = wl_display_get_registry(wl_dpy);
    wl_registry_add_listener(registry, &registry_listener, ctx);
    wl_display_roundtrip(wl_dpy);
    wl_registry_destroy(registry);

    if (!ctx->activation_manager) {
        g_debug("wayland-activate: xdg_activation_v1 not available");
        if (ctx->seat)
            wl_seat_destroy(ctx->seat);
        free(ctx);
        return;
    }

    /* Target surface */
    ctx->target_surface = gdk_wayland_window_get_wl_surface(window);
    if (!ctx->target_surface) {
        xdg_activation_v1_destroy(ctx->activation_manager);
        if (ctx->seat)
            wl_seat_destroy(ctx->seat);
        free(ctx);
        return;
    }

    /* Create activation token */
    token = xdg_activation_v1_get_activation_token(ctx->activation_manager);
    if (!token) {
        xdg_activation_v1_destroy(ctx->activation_manager);
        if (ctx->seat)
            wl_seat_destroy(ctx->seat);
        free(ctx);
        return;
    }

    xdg_activation_token_v1_add_listener(token, &activation_token_listener, ctx);

    /* Set serial from timestamp (if available) */
    if (timestamp && ctx->seat) {
        xdg_activation_token_v1_set_serial(token, timestamp, ctx->seat);
    }

    /* Set surface - the window being activated */
    xdg_activation_token_v1_set_surface(token, ctx->target_surface);

    xdg_activation_token_v1_commit(token);

    /*
     * Do not block here. GTK main loop will dispatch wayland events.
     * Context (including activation_manager and seat) is freed in done callback.
     */
}

#else

void
gdk_wayland_window_activate(GdkWindow *window, guint32 timestamp) {
    /* Dummy function - Wayland not available */
    (void)window;
    (void)timestamp;
}

#endif
