/***************************************************************************
 *   Copyright (C) 2008-2011 by Ben Nahill                                      *
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


#include <gst/gst.h>
#include <stdbool.h>
#include "output.h"
#include "ncrok.h"
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

static bool update_time();
void out_seek_rel(int64_t micros);

static GMainLoop *loop;
GstBus *bus;
GstElement *pipeline;
char out_state;
Ncrok *ncrok;
gint64 len;
int failupdates;

pthread_t out_thread;
pthread_attr_t out_attr;

void init_gst(Ncrok *ref){
	ncrok = ref;
	gst_init(NULL, NULL);
	// Use default context
	loop = g_main_loop_new (NULL, false);
	pipeline = gst_element_factory_make ("playbin","player");
	out_state = OUT_STATE_STOPPED;
	len = 0;
	failupdates = 0;
	pthread_attr_init(&out_attr);
	pthread_attr_setdetachstate(&out_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&out_thread, &out_attr, out_gst_run, NULL);
}

void close_gst(){
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
	gst_object_unref(GST_OBJECT (pipeline));
	out_state = OUT_STATE_NOTREADY;
	pthread_cancel(out_thread);
}

static gboolean bus_call (
		GstBus *bus,
		GstMessage *msg,
		gpointer user_data) {
	switch (GST_MESSAGE_TYPE (msg)){
		case GST_MESSAGE_EOS:
			out_stop();
			ncrok->nextTrack();
			//g_message ("End of Stream");
			break;
		case GST_MESSAGE_ERROR:
			gchar *debug;
			GError *err;
			gst_message_parse_error (msg, &err, &debug);
			g_free(debug);
			//g_error("%s",err->message);
			g_error_free(err);
			g_main_loop_quit(loop);
			break;
		case GST_MESSAGE_UNKNOWN:
			g_print("Unknown message");
			break;
		default:
			break;
	}


	return true;
}

static bool out_get_length(void){
	GstFormat fmt = GST_FORMAT_TIME;
	char lenstr[16];
	gst_element_query_duration(pipeline, &fmt, &len);
	// Got a valid length
	if( len > 100 ){
		g_snprintf(lenstr,16,OUT_TIME_FMT, GST_TIME_ARGS(len));
		ncrok->setLength(lenstr);
		// Don't run again
		return false;
	}
	return true;
}

void play_path (gchar *path){
	if(out_state != OUT_STATE_STOPPED){
		out_stop();
	}
	if(path){
		g_object_set(G_OBJECT (pipeline), "uri", path, NULL);

		bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
		gst_bus_add_watch (bus, bus_call, NULL);
		// Update time every half second
		g_timeout_add (500, (GSourceFunc) update_time, NULL);
		// Check for track length every 100ms (until successful)
		g_timeout_add (100, (GSourceFunc) out_get_length, NULL);
		gst_object_unref (bus);
		gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
		out_state = OUT_STATE_PLAYING;
	}
	free(path);
	ncrok->reDraw();
}

//GST Main loop pthread function.
void *out_gst_run(void *null){
	while(1){
		g_main_loop_run(loop);
		pthread_testcancel();
	}
	pthread_exit((void *) 0);
}

//Returns true on play
bool out_play_pause(){	
	switch(out_state){
		case OUT_STATE_PAUSED:
			gst_element_set_state(GST_ELEMENT (pipeline), GST_STATE_PLAYING);
			out_state = OUT_STATE_PLAYING;
			return true;
		case OUT_STATE_PLAYING:
			gst_element_set_state(GST_ELEMENT (pipeline), GST_STATE_PAUSED);
			out_state = OUT_STATE_PAUSED;
			return false;
	}
	
}

static bool update_time(){
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;
	if(out_state == OUT_STATE_STOPPED){
		if(failupdates < OUT_MAX_FAIL){
			failupdates++;
			return true;
		}
		failupdates = 0;
		return false;
	}
	failupdates = 0;
	pthread_mutex_lock(&ncrok->display_mutex);
	if (gst_element_query_position (pipeline, &fmt, &pos)) {
		char *posstr;
		posstr = (char*)malloc(16);
		g_snprintf(posstr,16,OUT_TIME_FMT, GST_TIME_ARGS(pos));
		double rel = (double)pos / (double)len;
		ncrok->updateTime(posstr, rel);
	}
	pthread_mutex_unlock(&ncrok->display_mutex);
	return true;
}

void out_stop(){
	GstState state;
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
	gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE);
	while(state != GST_STATE_NULL){
		usleep(200000);
		gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE);
	}
	out_state = OUT_STATE_STOPPED;
}

void out_seek_fine(bool dir){
	if(dir) out_seek_rel(OUT_SEEK_FINE);
	else out_seek_rel(-1 * OUT_SEEK_FINE);
}

void out_seek_coarse(bool dir){
	if(dir) out_seek_rel(OUT_SEEK_COARSE);
	else out_seek_rel(-1 * OUT_SEEK_COARSE);
}

void out_seek_rel(int64_t micros){
	if(out_state != OUT_STATE_PLAYING && out_state != OUT_STATE_PAUSED) return;
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len, newtime;
	
	if (gst_element_query_position (pipeline, &fmt, &pos)
		&& gst_element_query_duration (pipeline, &fmt, &len)){
		
		newtime = pos + (GST_MSECOND * micros);
		if(newtime < 0) newtime = 0;
		if(newtime > len){
			ncrok->nextTrack();
			return;
		}
		
		gst_element_seek_simple(pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, newtime);
	}
}

