/***************************************************************************
 *   Copyright (C) 2008-2012 by Ben Nahill                                 *
 *   bnahill@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _OUTPUT_H
#define _OUTPUT_H

#include <gst/gst.h>
#include <stdbool.h>
#include "ncrok.h"
#include <pthread.h>

#define OUT_STATE_STOPPED	0
#define OUT_STATE_PAUSED	1
#define OUT_STATE_PLAYING	2
#define OUT_STATE_NOTREADY	3

//update every 200ms
#define OUT_TIMER_INTERVAL	200000

//in ms
#define OUT_SEEK_FINE		5000
#define OUT_SEEK_COARSE		20000

#define OUT_TIME_FMT		"%02u:%02u:%02u"

#define OUT_MAX_FAIL		3

class Audio {
public:
    typedef enum {
        DIR_BACKWARD,
        DIR_FORWARD
    } dir_t;

    void init();
    void close();
    void playPath(const std::string &path);
    bool playPause();
    void seekFine(dir_t dir);
    void seekCoarse(dir_t dir);
    void stop();
protected:
    void seekRel(int64_t micros);

    gboolean busCall(GstBus *bus, GstMessage *msg);
    static gboolean busCallCB(GstBus *bus, GstMessage *msg, gpointer user_data);
    bool getLength();
    static bool getLengthCB(void *ptr);
    bool updateTime();
    static bool updateTimeCB(void *ptr);
    static void *gstRun(void *ptr);

    GMainLoop *loop;
    GstBus *bus;
    GstElement *pipeline;
    char out_state;
    gint64 len;
    int failupdates;
    pthread_t out_thread;
    pthread_attr_t out_attr;
};

extern Audio audio;

#endif
