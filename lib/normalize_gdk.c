/*
    normalize_gdk.c

    Copyright (C) 2010  Red Hat, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "normalize.h"
#include "gdb_frame.h"
#include "gdb_thread.h"
#include <stdbool.h>
#include <string.h>

void
btp_gdb_normalize_gdk_thread(struct btp_gdb_thread *thread)
{
    struct btp_gdb_frame *frame = thread->frames;
    while (frame)
    {
        struct btp_gdb_frame *next_frame = frame->next;

        /* Remove IA__ prefix. */
        btp_gdb_frame_remove_func_prefix(frame, "IA__gdk", strlen("IA__"));

        /* Remove frames which are not a cause of the crash. */
        bool removable =
            btp_gdb_frame_calls_func_in_file(frame, "gdk_x_error", "gdkmain-x11.c") ||
            btp_gdb_frame_calls_func_in_file(frame, "gdk_threads_dispatch", "gdk.c") ||
            btp_gdb_frame_calls_func_in_file2(frame, "gdk_event_dispatch", "gdkevents-x11.c", "gdkevents.c") ||
            btp_gdb_frame_calls_func_in_file(frame, "gdk_event_source_dispatch", "gdkeventsource.c");

        if (removable)
        {
            btp_gdb_thread_remove_frame(thread, frame);
        }

        frame = next_frame;
    }
}
