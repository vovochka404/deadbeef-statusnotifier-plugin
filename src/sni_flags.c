/*
 * deadbeef-statusnotifier-plugin - Copyright (C) 2015 Vladimir Perepechin
 *
 * sni_flags.c
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
    uint32_t v1;
} sni_flags_t;

static sni_flags_t sni_flags = {0};

gboolean
sni_flag_get(uint32_t code) {
    gboolean ret = (sni_flags.v1 & ((uint32_t)1 << code));
    return ret;
}

void
sni_flag_set(uint32_t code) {
    sni_flags.v1 |= ((uint32_t)1 << code);
}

void
sni_flag_unset(uint32_t code) {
    sni_flags.v1 &= ~((uint32_t)1 << code);
}
