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

#include <gst/gst.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "audio.h"
#include "ncrok.h"

Audio audio;

void Audio::init(){
	gst_init(NULL, NULL);
	// Use default context
	loop = g_main_loop_new (NULL, false);
	pipeline = gst_element_factory_make ("playbin","player");
	out_state = OUT_STATE_STOPPED;
	len = 0;
	failupdates = 0;
	pthread_attr_init(&out_attr);
	pthread_attr_setdetachstate(&out_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&out_thread, &out_attr, gstRun, this);
}

void Audio::close(){
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
	gst_object_unref(GST_OBJECT (pipeline));
	out_state = OUT_STATE_NOTREADY;
	pthread_cancel(out_thread);
}

gboolean Audio::busCall(GstBus *bus, GstMessage *msg){
	switch (GST_MESSAGE_TYPE (msg)){
		case GST_MESSAGE_EOS:
			stop();
			Ncrok::app.nextTrack();
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



bool Audio::getLength(){
	GstFormat fmt = GST_FORMAT_TIME;
	char lenstr[16];
	gst_element_query_duration(pipeline, &fmt, &len);
	// Got a valid length
	if( len > 100 ){
		g_snprintf(lenstr, 16, OUT_TIME_FMT, GST_TIME_ARGS(len));
		Ncrok::app.setLength(lenstr);
		// Don't run again
		return false;
	}
	return true;
}

bool Audio::getLengthCB(void *ptr){
	Audio *a = (Audio *)ptr;
	return a->getLength();
}

void Audio::playPath(const std::string &path){
	if(out_state != OUT_STATE_STOPPED){
		stop();
	}
	if(path.length()){
		g_object_set(G_OBJECT (pipeline), "uri", path.c_str(), NULL);

		bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
		gst_bus_add_watch (bus, busCallCB, this);
		// Update time every half second
		g_timeout_add (500, (GSourceFunc) updateTimeCB, this);
		// Check for track length every 100ms (until successful)
		g_timeout_add (100, (GSourceFunc) getLengthCB, this);
		gst_object_unref (bus);
		gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
		out_state = OUT_STATE_PLAYING;
	}
	Ncrok::app.reDraw();
}


//Returns true on play
bool Audio::playPause(){
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

bool Audio::updateTime(){
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
	if (gst_element_query_position (pipeline, &fmt, &pos)) {
		char *posstr;
		posstr = (char*)malloc(16);
		g_snprintf(posstr,16,OUT_TIME_FMT, GST_TIME_ARGS(pos));
		double rel = (double)pos / (double)len;
		Ncrok::app.updateTime(posstr, rel);
	}
	return true;
}

void Audio::stop(){
	GstState state;
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
	gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE);
	while(state != GST_STATE_NULL){
		usleep(200000);
		gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE);
	}
	out_state = OUT_STATE_STOPPED;
}

void Audio::seekFine(dir_t dir){
	if(dir == DIR_FORWARD) seekRel(OUT_SEEK_FINE);
	else seekRel(-1 * OUT_SEEK_FINE);
}

void Audio::seekCoarse(dir_t dir){
	if(dir == DIR_FORWARD) seekRel(OUT_SEEK_COARSE);
	else seekRel(-1 * OUT_SEEK_COARSE);
}

void Audio::seekRel(int64_t micros){
	if(out_state != OUT_STATE_PLAYING && out_state != OUT_STATE_PAUSED) return;
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len, newtime;

	if (gst_element_query_position (pipeline, &fmt, &pos)
		&& gst_element_query_duration (pipeline, &fmt, &len)){

		newtime = pos + (GST_MSECOND * micros);
		if(newtime < 0) newtime = 0;
		if(newtime > len){
			Ncrok::app.nextTrack();
			return;
		}

		gst_element_seek_simple(pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, newtime);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Statics
//////////////////////////////////////////////////////////////////////////////

gboolean Audio::busCallCB(GstBus *_bus, GstMessage *msg, gpointer ptr) {
	return ((Audio *)ptr)->busCall(_bus, msg);
}

//GST Main loop pthread function.
void *Audio::gstRun(void *ptr){
	Audio *a = (Audio *)ptr;
	while(1){
		g_main_loop_run(a->loop);
		pthread_testcancel();
	}
	pthread_exit((void *) 0);
}

bool Audio::updateTimeCB(void *ptr){
	return ((Audio *)ptr)->updateTime();
}
