/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */


/*
 * fate.c
 *
 * Copyright (C) 2004 The Free Software Foundation, Inc.
 *
 * Developed by Jon Trowbridge <trow@gnu.org>
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifdef CONFIG_H
#include <config.h>
#endif
#include "fate.h"

#include <glib.h>
#include <stdlib.h>
#include <time.h>

void
fate_seed (unsigned seed)
{
    srandom (seed);
}

void
fate_seed_from_time (void)
{
    time_t t;
    time (&t);
    fate_seed ((unsigned) t);
}

unsigned
fate_random (unsigned N)
{
    g_return_val_if_fail (N > 1, 0);
    return random () % N;
}
