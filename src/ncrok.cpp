/***************************************************************************
 *   Copyright (C) 2008 by Ben Nahill                                      *
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

/*
 * nCrok.cpp - Contains the UI for the application
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ncurses.h>
#include <panel.h>
#include <string.h> 
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <menu.h>
#include <form.h>
#include <stdlib.h>

#include "ncrok.h"
#include "output.h"
#include "tune.h"
#include "playlist.h"
#include "window.h"

static Ncrok app;
static void resizeWin(int ignore);

/*
 * Run everything!
 */
int main(int argc, char **argv){
	FILE *hidestderr = freopen ("/dev/null","w",stderr);
	int count = 0;
	
	if(argc > 1){
		int i;
		for(i = 1; i < argc; i++){
			count += app.playlist.readDir(argv[i]);
		}
	}
	printf("\033]0;%s\007", "Ncrok");
	app.initialize(count);
	app.runPlaylist();
	return 0;
}

Ncrok::Ncrok(){
	
}

Ncrok::~Ncrok(){
	printf("\033]0;%s\007", "");
	close_gst();
	endwin();
}

int Ncrok::initialize(int tracks){
	(void) signal(SIGWINCH, resizeWin);
	initscr();
	char title[16];

	//raw();
	noecho();
	cbreak();
	keypad(stdscr, true);
	curs_set(0);
	
	pthread_mutex_init(&display_mutex, NULL);

	initWindows();

	//box(stdscr,0,0);
	update_panels();

	doupdate();
	sprintf(title, "%d Tracks",tracks);
	right.printTitle(title);
	
	init_gst(this);
	bottom.printTitle(TITLE_STRING);
	return 0;
}

void Ncrok::runPlaylist(){
	int tmp = 0;
	char longtmp[256];
	char longtmp2[256];
	char ch;
	ITEM *cur;

	activewin = &right;
	if(playlist.size() == 0){
		sprintf(longtmp,"%s/.ncroklst",getenv("HOME"));
		if((tmp = playlist.load(longtmp)) > 0){
			sprintf(longtmp2, "Playlist Loaded - %d files", tmp);
			right.printTitle(longtmp2);
		}
	}
	playlist.sort();
	keypad(right.getWindow(), TRUE);

	int x, y, w, h;
	right.getDims(&x,&y,&w,&h);

	playmenu = playlist.getMenu();

	set_menu_win(playmenu, right.getWindow());

	set_menu_sub(playmenu, derwin(right.getWindow(), h-2, w-2, 1, 1));
	set_menu_format(playmenu, h-2, 1);
	set_menu_mark(playmenu, 0);
	post_menu(playmenu);
	right.refresh();

	while((ch = wgetch(right.getWindow())) != IN_QUIT){
		pthread_mutex_lock(&display_mutex);
		switch(ch){
			case IN_SAVE:
				sprintf(longtmp,"%s/.ncroklst",getenv("HOME"));
				if((tmp = playlist.save(longtmp)) <= 0) break;
				sprintf(longtmp2, "Playlist Saved - %s - %d files", longtmp, tmp);
				right.printTitle(longtmp2);
				break;
			case IN_LOAD:
				sprintf(longtmp,"%s/.ncroklst",getenv("HOME"));
				if((tmp = playlist.load(longtmp)) <= 0)
					break;
				sprintf(longtmp2, "Playlist Loaded - %d files", tmp);
				right.printTitle(longtmp2);
				refreshPlaylist();
				break;
			case IN_DOWN:
				menu_driver(playmenu, REQ_DOWN_ITEM);
				break;
			case IN_UP:
				menu_driver(playmenu, REQ_UP_ITEM);
				break;
			case IN_PUP:
				for(tmp = 0; tmp < h - 2; tmp++)
					menu_driver(playmenu, REQ_UP_ITEM);
				break;
			case IN_PDOWN:
				for(tmp = 0; tmp < h - 2; tmp++)
					menu_driver(playmenu, REQ_DOWN_ITEM);
				break;
			case IN_OPEN:
				addDir();
				break;
			case IN_PLAY_PAUSE:
			case IN_PAUSE:
				if(activetrack == NULL) break;
				bottom.printCentered("                       ",1);
				if(!out_play_pause())
					bottom.printCentered("PAUSED",1);
				//Pause/play
				break;
			case IN_REMOVE:
				//remove item
				break;
			case IN_NEXT:
				if(item_count(playmenu) == 0) break;
				nextTrack();
				break;
			case IN_PREV:
				if(item_count(playmenu) == 0) break;
				prevTrack();
				break;
			case IN_STOP:
				if(activetrack == NULL) break;
				out_stop();
				bottom.printTitle("");
				bottom.clear();
				bottom.printCentered("STOPPED",1);
				break;
			case IN_PLAY:
				if(item_count(playmenu) == 0) break;
				playSelected();
				break;
			case IN_JUMP:
				cur = current_item(playmenu);
				if(playlist.queueSize() > 0){
					int *queue = playlist.getQueue();
					int currqueue = playlist[(uint64_t) item_userptr(cur)]->queue_index;
					if((uint64_t) item_userptr(cur) == playlist.currIndex){
						jumpToIndex(queue[0]);
						break;
					}
					else if(currqueue >= 0){
						if(playlist.queueSize() > currqueue + 1){
							jumpToIndex(queue[currqueue + 1]);
							break;
						}
					}

				}
				jumpToIndex(playlist.currIndex);
				break;
			case IN_FWD_FINE:
				out_seek_fine(1); break;
			case IN_BACK_FINE:
				out_seek_fine(0); break;
			case IN_FWD_COARSE:
				out_seek_coarse(1); break;
			case IN_BACK_COARSE:
				out_seek_coarse(0); break;
			case IN_SEARCH:
				if(item_count(playmenu) == 0) break;
				search();
				break;
			case IN_QUEUE:
				if(item_count(playmenu) == 0) break;
				cur = current_item(playmenu);
				playlist.toggleQueue((uint64_t) item_userptr(cur));
				updateQueueLabels();
				break;
			case IN_STOP_AFTER:
				if(item_count(playmenu) == 0) break;
				cur = current_item(playmenu);
				playlist.stopAfter((uint64_t) item_userptr(cur));
				updateQueueLabels();
				break;
			case IN_DELETE:
				if(item_count(playmenu) == 0) break;
				cur = current_item(playmenu);
				tmp = (uint64_t) item_userptr(cur);
				if(playlist[tmp] == activetrack){
					if(tmp > 0)
						activetrack = playlist.activeTrack();
				}
				playlist.remove(tmp);
				free_item(cur);
				refreshPlaylist();
				jumpToIndex(tmp);
				right.printTitle("1 File Deleted");
				break;
			case IN_SEARCH_START:
				findByFirst();
				break;
			case IN_REDRAW:
				reDraw();
				break;
			default:
				//sprintf(longtmp,"%u",ch);
				//bottom.printCentered(longtmp,1);
				break;
		}
		right.refresh();
		pthread_mutex_unlock(&display_mutex);
	}
}

void Ncrok::playSelected(){
	ITEM *cur;
	bottom.printCentered("                       ",1);
	cur = current_item(playmenu);
	activetrack = (Tune*) playlist[(long) item_userptr(cur)];
	bottom.printTitle(activetrack->getMenuText());
	playlist.play((long) item_userptr(cur));
	pos_menu_cursor(playmenu);
}

void Ncrok::initWindows(){
	int w, h;
	getmaxyx(stdscr, h, w);
	//mvprintw(10, 10, "%u, %u, %u, %u\n", h-4, w-20, 0, 20);

	/*left.setDims(0, 0, 20, h);
	right.setDims(20, 0, w-20, h-4);
	bottom.setDims(20, h-4, w-20, 4);*/

	right.setDims(0, 0, w, h-5);
	bottom.setDims(0, h-5, w, 5);

	//Set cycle order
	//left.pointPanelTo(&right);
	right.pointPanelTo(&bottom);
	bottom.pointPanelTo(&left);

	//left.printTitle("Artists");
	right.printTitle("Playlist");
}

/*
 * Allow the user to enter the path of a directory to add to the collection
 */
void Ncrok::addDir(){
	char qstring[74];
	char out[80];
	int len = 0;
	bool get_out = 0;
	char ch;
	int count;
	sprintf(out,"Open:");
	right.printCentered(out, right.getHeight() - 1);
	pthread_mutex_unlock(&display_mutex);
	while((ch = wgetch(right.getWindow())) != IN_ESC){
		pthread_mutex_lock(&display_mutex);
		right.reBox();
		right.refresh();
		switch(ch){
			case IN_BELL:
			case IN_DELETE:
			case IN_DELETE2:
			case IN_BACKSPACE:
				if(len > 0){
					qstring[--len] = NULL;
				}
				else get_out = true;
				break;
			case 10:
				playlist.clearQueue();
				count = playlist.readDir(qstring);
				if(count > 0){
					sprintf(out,"%d Tracks added",count);
					right.printTitle(out);
					playlist.sort();
					refreshPlaylist();
				}
				get_out = true;
				break;
			default:
				if(ch >= 32 && ch <= 126){
					if(len < 72){
						qstring[len] = ch;
						qstring[++len] = NULL;
					}
					break;
				} get_out = true;
		}
		if(get_out) break;
		sprintf(out, "Open: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		doupdate();
		// Unlock only if looping
		pthread_mutex_unlock(&display_mutex);
	}
	right.reBox();
}

void Ncrok::refreshPlaylist(){
	if(playmenu != NULL){
		free_menu(playmenu);
	}
	playmenu = playlist.getMenu();
	set_menu_win(playmenu, right.getWindow());
	int x, y, w, h;
	right.getDims(&x,&y,&w,&h);

	set_menu_sub(playmenu, derwin(right.getWindow(), h-2, w-2, 1, 1));
	set_menu_format(playmenu, h-2, 1);
	set_menu_mark(playmenu, 0);
	post_menu(playmenu);
	jumpToIndex(0);
}

void Ncrok::findByFirst(){
	char qstring[32];
	char out[48];
	char ch;
	int len = 0;
	int index = 0;
	bool get_out = false;
	sprintf(out,"Jump:");
	right.printCentered(out, right.getHeight() - 1);
	pthread_mutex_unlock(&display_mutex);

	while((ch = wgetch(right.getWindow())) != IN_ESC && !get_out){
		pthread_mutex_lock(&display_mutex);
		right.reBox();
		right.refresh();
		switch(ch){
			case IN_BELL:
			case IN_DELETE:
			case IN_DELETE2:
			case IN_BACKSPACE:
				if(len > 0){
					qstring[--len] = NULL;
				}
				else get_out = true;
				break;
			case IN_PLAY:
				if(len > 0)
					playSelected();
				get_out = true;
				break;
			default:
				if(ch >= 32 && ch <= 126){
					if(len < 32){
						qstring[len] = ch;
						qstring[++len] = NULL;
					}
					break;
				} get_out = true;
		}
		if(get_out) break;
		sprintf(out, "Jump: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		if(len > 0){
			index = playlist.getFirst(qstring);
			if(index > -1){
				jumpToIndex(index);
			}
		}
		doupdate();
		pthread_mutex_unlock(&display_mutex);
	}
	right.reBox();
}

void Ncrok::search(){
	char qstring[32];
	qstring[0] = 0;
	char out[40];
	char ch;
	int len = 0;
	int index = 0;
	bool get_out = false;
	bool search_again;
	sprintf(out,"Search:");
	//debug output
	char test[10];
	right.printCentered(out, right.getHeight() - 1);
	pthread_mutex_unlock(&display_mutex);
	while((ch = wgetch(right.getWindow())) != IN_ESC){
		pthread_mutex_lock(&display_mutex);
		search_again = true;
		right.reBox();
		right.refresh();
		//Debug keys
		//sprintf(test,"%u",ch);
		//right.printTitle(test);
		switch(ch){
			case IN_BELL:
			case IN_DELETE:
			case IN_DELETE2:
			case IN_BACKSPACE:
				if(len > 0){
					playlist.clearSearch();
					qstring[--len] = 0;
					
				}
				else get_out = true;
				break;
			case IN_TAB:
			case IN_DOWN:
				index = playlist.nextResult();
				if(index > -1)
					jumpToIndex(index);
				search_again = false;
				break;
			case IN_UP:
				index = playlist.prevResult();
				if(index > -1)
					jumpToIndex(index);
				search_again = false;
				break;
			case IN_PLAY:
				playSelected();
				search_again = false;
				break;
			default:
				if(ch >= 32 && ch <= 126){ //letter...
					if(len < 32){ //Just do nothing in this case
						qstring[len] = ch;
						qstring[++len] = NULL;
					}
				}
				else get_out = true;
		}
		if(get_out) break;
		sprintf(out, "Search: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		if(!search_again){
			doupdate();
			pthread_mutex_unlock(&display_mutex);
			continue;
		}
		if(len > 0){
			index = playlist.search(qstring);
			if(index > -1){
				jumpToIndex(index);
			}
		}
		doupdate();
		pthread_mutex_unlock(&display_mutex);
	}
	right.reBox();
	playlist.clearSearch();
}

void Ncrok::jumpToIndex(int index){
	ITEM **items = menu_items(playmenu);
	set_current_item(playmenu, items[index]);
	if(index > 0){
		menu_driver(playmenu, REQ_UP_ITEM);
		menu_driver(playmenu, REQ_DOWN_ITEM);
	}
}

void Ncrok::updateQueueLabels(){
	ITEM *index = current_item(playmenu);
	ITEM **items = menu_items(playmenu);
	int *queue = playlist.getQueue();
	int size = playlist.queueSize();
	for(int i = 0; i < size; i++){
		playlist[queue[i]]->updateItem(items[queue[i]]);
	}
	if(playlist.dequeued >= 0){ //update dequeued
		playlist[playlist.dequeued]->updateItem(items[playlist.dequeued]);
		playlist.dequeued = -1;
	}
	if(playlist.stopafter >= 0){
		playlist[playlist.stopafter]->updateItem(items[playlist.stopafter]);
	}
	if(playlist.prevstopafter >= 0){
		playlist[playlist.prevstopafter]->updateItem(items[playlist.prevstopafter]);
	}
	free(queue);
	
	unpost_menu(playmenu);
	post_menu(playmenu);
	set_current_item(playmenu, index);
	right.refresh();
	pos_menu_cursor(playmenu);
}

void Ncrok::updateTime(char* pos, double rel){
	char buf[64];
	sprintf(buf,"%s / %s",pos, length);
	bottom.printCentered(buf,3);
	//bottom.printCentered("Tuna",3);
	drawProgress(rel);
}

void Ncrok::setLength(char *len){
	strcpy(length, len);
}

void Ncrok::nextTrack(){
	bottom.clear();
	if((activetrack = playlist.nextTrack()) != NULL){
		bottom.printTitle(activetrack->getMenuText());
		playlist.play();
	}
	else bottom.reBox();
	updateQueueLabels();
}

void Ncrok::prevTrack(){
	if((activetrack = playlist.prevTrack()) != NULL){
		bottom.printTitle(activetrack->getMenuText());
		playlist.play();
		updateQueueLabels();
	}
}


//This function is called by another thread!
void Ncrok::drawProgress(double progress){
	char stat = (char)(progress * 64);
	char str[67];
	str[0] = '|';
	str[65] = '|';
	str[66] = '\0';
	int i;
	for(i = 0; i < 64; i++){
		if(i < stat)
			str[i+1] = '=';
		else if(i == stat)
			str[i+1] = '>';
		else str[i+1] = '-';
	}
	//sprintf(str,"%f",progress);
	bottom.printCentered(str,2);
}

/*
 * Redraw everything. Go corrupted data
 */
void Ncrok::reDraw(){
	redrawwin(stdscr);
}


void Ncrok::resizeTerm(){
	//Get the window size directly
	struct winsize ws;
	int fd = open("/dev/tty", O_RDWR);
	if(ioctl(fd,TIOCGWINSZ,&ws)!=0) return;
	
	resizeterm(ws.ws_row, ws.ws_col);

	right.setDims(0,0,ws.ws_col, ws.ws_row - 5);
	bottom.setDims(0, ws.ws_row - 5, ws.ws_col, 5);

	int x, y, w, h;
	right.getDims(&x,&y,&w,&h);

	unpost_menu(playmenu);
	set_menu_sub(playmenu, derwin(right.getWindow(), h-2, w-2, 1, 1));
	set_menu_format(playmenu, h-2, 1);
	set_menu_mark(playmenu, 0);
	keypad(right.getWindow(), TRUE);
	post_menu(playmenu);

	update_panels();
	refresh();
	doupdate();
}

static void resizeWin(int ignore){
	app.resizeTerm();
}

