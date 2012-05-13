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

/*
 * ncrok.cpp - Contains the UI for the application
 */

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
#include "audio.h"

Ncrok Ncrok::app;
static void resizeWin(int ignore);

inline static Tune *tune_from_item(const ITEM *item){
	return (Tune *)item_userptr(item);
}

/*
 * Run everything!
 */
int main(int argc, char **argv){
	FILE *_ = freopen ("/dev/null","w",stderr);
	int count = 0;

	if(argc > 1){
		int i;
		for(i = 1; i < argc; i++){
			count += Ncrok::app.playlist.readDir(argv[i]);
		}
	}

	printf("\033]0;%s\007", "Ncrok");
	Ncrok::app.initialize(count);
	Ncrok::app.runPlaylist();
	return 0;
}

Ncrok::Ncrok(){
	struct winsize ws;
	int fd = open("/dev/tty", O_RDWR);
\
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&display_mutex, &attr);

	if(ioctl(fd,TIOCGWINSZ,&ws)!=0) throw std::exception();

	numRows = ws.ws_row;
	numCols = ws.ws_col;


	windows.push_back(&right);
	windows.push_back(&bottom);
}

Ncrok::~Ncrok(){
	printf("\033]0;%s\007", "");
	audio.close();
	endwin();
	pthread_mutexattr_destroy(&attr);
}

void Ncrok::initialize(int tracks){
	(void) signal(SIGWINCH, resizeWin);
	initscr();
	char title[16];

	//raw();
	noecho();
	cbreak();
	keypad(stdscr, true);
	intrflush(stdscr, FALSE);
	curs_set(0);

	sprintf(title, "%d Tracks",tracks);

	audio.init();

	right.display(numCols, numRows);
	right.setPanelTop();
	right.printTitle(title);
	bottom.display(numCols, numRows);

	updatePanels();
}

void Ncrok::runPlaylist(){
	int tmp = 0;
	char longtmp[256];
	char longtmp2[256];
	char ch;
	ITEM *cur;

	if(playlist.size() == 0){
		sprintf(longtmp,"%s/.ncroklst",getenv("HOME"));

		if((tmp = playlist.load(longtmp)) > 0){
			sprintf(longtmp2, "Playlist Loaded - %d files", tmp);
			right.printTitle(longtmp2);
		}
	}
	playlist.sort();
	keypad(right.getWindow(), TRUE);

	right.assignMenu(playlist.getMenu());

	while((ch = wgetch(right.getWindow())) != IN_QUIT){
		switch(ch){
			case IN_HELP1:
			case IN_HELP2:
				showHelpWindow();
				break;
			case IN_SAVE:
				sprintf(longtmp,"%s/.ncroklst", getenv("HOME"));
				if((tmp = playlist.save(longtmp)) <= 0) break;
				sprintf(longtmp2, "Playlist Saved - %s - %d files", longtmp, tmp);
				lockDisplay();
				right.printTitle(longtmp2);
				unlockDisplay();
				break;
			case IN_LOAD:
				sprintf(longtmp,"%s/.ncroklst",getenv("HOME"));
				if((tmp = playlist.load(longtmp)) <= 0)
					break;
				sprintf(longtmp2, "Playlist Loaded - %d files", tmp);
				lockDisplay();
				right.printTitle(longtmp2);
				refreshPlaylist();
				unlockDisplay();
				break;
			case IN_DOWN:
				right.scrollDown(1);
				updatePanels();
				break;
			case IN_UP:
				right.scrollUp(1);
				updatePanels();
				break;
			case IN_PUP:
				right.scrollUp(right.innerHeight());
				updatePanels();
				break;
			case IN_PDOWN:
				right.scrollDown(right.innerHeight());
				updatePanels();
				break;
			case IN_OPEN:
				addDir();
				break;
			case IN_PLAY_PAUSE:
			case IN_PAUSE:
				if(activetrack == NULL) break;
				bottom.printCentered("                       ",1);
				if(!audio.playPause())
					bottom.printCentered("PAUSED",1);
				break;
			case IN_REMOVE:
				//remove item
				break;
			case IN_NEXT:
				if(right.itemCount() == 0) break;
				nextTrack();
				break;
			case IN_PREV:
				if(right.itemCount() == 0) break;
				prevTrack();
				break;
			case IN_STOP:
				if(activetrack == NULL) break;
				stop();
				break;
			case IN_PLAY:
				if(right.itemCount() == 0) break;
				playSelected();
				break;
			case IN_JUMP:
				doJumpAction();
				break;
			case IN_FWD_FINE:
				audio.seekFine(Audio::DIR_FORWARD); break;
			case IN_BACK_FINE:
				audio.seekFine(Audio::DIR_BACKWARD); break;
			case IN_FWD_COARSE:
				audio.seekCoarse(Audio::DIR_FORWARD); break;
			case IN_BACK_COARSE:
				audio.seekCoarse(Audio::DIR_BACKWARD); break;
			case IN_SEARCH:
				if(right.itemCount() == 0) break;
				search();
				break;
			case IN_QUEUE:
				if(right.itemCount() == 0) break;
				cur = right.getCurrentItem();
				playlist.toggleQueue(*tune_from_item(cur));
				updateQueueLabels();
				break;
			case IN_STOP_AFTER:
				if(right.itemCount() == 0) break;
				cur = right.getCurrentItem();
				playlist.stopAfter(*tune_from_item(cur));
				updateQueueLabels();
				break;
			case IN_DELETE:
				deleteSelected();
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
		updatePanels();
	}
}

void Ncrok::doJumpAction(){
	const Playlist::play_queue_t &queue = playlist.getQueue();
	Tune *tune;
	int currqueue;
	if(queue.size() > 0){
		tune = tune_from_item(right.getCurrentItem());
		// Check to see if the current item is in the queue
		currqueue = tune->queue_index;
		if(tune->index == playlist.currIndex){
			// On currently playing item, jump to first in queue
			jumpToIndex(queue[0]);
			return;
		} else if(currqueue >= 0){
			if(queue.size() > currqueue + 1){
				// Currently looking at a queued item, go to the next
				jumpToIndex(queue[currqueue + 1]);
				return;
			}
		}

	}
	// By default just go to the current track
	jumpToIndex(playlist.currIndex);
}

void Ncrok::playSelected(){
	ITEM *cur;
	if(playlist.size() == 0) return;

	bottom.printCentered("                       ",1);
	cur = right.getCurrentItem();
	activetrack = tune_from_item(cur);
	bottom.printTitle(activetrack->getMenuText());
	playlist.play(*activetrack);
	right.posCursor();
}

void Ncrok::showHelpWindow(){
	char ch;
	HelpWindow helpWindow;
	bool get_out = false;

	lockDisplay();

	windows.push_back(&helpWindow);

	helpWindow.display(numCols, numRows);

	helpWindow.setPanelTop();

	updatePanels();

	unlockDisplay();

	while(!get_out && ((ch = wgetch(right.getWindow())) != IN_QUIT)){
		switch(ch){
		case IN_HELP1:
		case IN_HELP2:
		case IN_ESC:
			get_out = true;
			break;
		case IN_UP:
			helpWindow.scrollUp();
			updatePanels();
			break;
		case IN_DOWN:
			helpWindow.scrollDown();
			updatePanels();
			break;
		default:
			break;
		}
	}

	lockDisplay();
	windows.pop_back();

	right.reBox();
	bottom.reBox();
	right.setPanelTop();

	updatePanels();

	unlockDisplay();
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

	while((ch = wgetch(right.getWindow())) != IN_ESC){
		switch(ch){
			case IN_BELL:
			case IN_DELETE:
			case IN_DELETE2:
			case IN_BACKSPACE:
				if(len > 0){
					qstring[--len] = 0;
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
						qstring[++len] = 0;
					}
					break;
				} get_out = true;
		}
		if(get_out) break;
		sprintf(out, "Open: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		updatePanels();
	}
	right.reBox();
	updatePanels();
}

void Ncrok::deleteSelected(){
	Tune *tune;
	ITEM *item;
	uint32_t index;
	if(right.itemCount() == 0) return;
	item = right.getCurrentItem();
	tune = tune_from_item(item);
	if(tune == activetrack){
		activetrack = playlist.activeTrack();
	}
	index = tune->index;
	playlist.remove(*tune);
	free_item(item);
	refreshPlaylist();
	jumpToIndex(index);
	right.printTitle("1 File Deleted");
}

void Ncrok::refreshPlaylist(){
	right.assignMenu(playlist.getMenu());

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

	while((ch = wgetch(right.getWindow())) != IN_ESC && !get_out){

		switch(ch){
			case IN_BELL:
			case IN_DELETE:
			case IN_DELETE2:
			case IN_BACKSPACE:
				if(len > 0){
					qstring[--len] = 0;
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
						qstring[++len] = 0;
					}
					break;
				} get_out = true;
		}
		if(get_out) break;
		right.reBox();
		sprintf(out, "Jump: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		if(len > 0){
			index = playlist.getFirst(qstring);
			if(index > -1){
				jumpToIndex(index);
			}
		}
		updatePanels();
	}
	right.reBox();
	updatePanels();
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

	right.printCentered(out, right.getHeight() - 1);

	updatePanels();

	while((ch = wgetch(right.getWindow())) != IN_ESC){
		search_again = true;

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
					right.reBox();
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
						qstring[++len] = 0;
					}
				}
				else get_out = true;
		}
		if(get_out) break;
		sprintf(out, "Search: %s",qstring);
		right.printCentered(out,right.getHeight() - 1);
		updatePanels();
		if(!search_again){
			continue;
		}
		if(len > 0){
			index = playlist.search(qstring);
			if(index > -1){
				jumpToIndex(index);
			}
		}
		updatePanels();
	}

	playlist.clearSearch();
	right.reBox();
	updatePanels();
}

void Ncrok::stop(){
	audio.stop();
	bottom.printTitle("");
	bottom.clear();
	updatePanels();
	bottom.printCentered("STOPPED",1);
}

void Ncrok::jumpToIndex(int index){
	ITEM **items = right.getItems();

	right.setItem(items[index]);
	if(index > 0){
		right.scrollUp(1);
		right.scrollDown(1);
		updatePanels();
	}
}

void Ncrok::updateQueueLabels(){
	ITEM *index = right.getCurrentItem();
	ITEM **items = right.getItems();
	const Playlist::play_queue_t &queue = playlist.getQueue();
	Playlist::play_queue_t::const_iterator i;
	for(i = queue.begin(); i != queue.end(); i++){
		playlist[i[0]]->updateItem(items[*i]);
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

	right.unpost();
	right.post();
	right.setItem(index);

	right.posCursor();

	updatePanels();
}

void Ncrok::updateTime(char* pos, double rel){
	char buf[64];
	sprintf(buf,"%s / %s",pos, length);
	lockDisplay();

	bottom.printCentered(buf,3);
	drawProgress(rel);

	updatePanels();

	unlockDisplay();
}

void Ncrok::setLength(char *len){
	strcpy(length, len);
}

void Ncrok::nextTrack(){
	if(playlist.size() == 0) return;
	bottom.clear();
	if((activetrack = playlist.nextTrack()) != NULL){
		bottom.printTitle(activetrack->getMenuText());
		playlist.play();
	}
	else {
		stop();
		bottom.reBox();
	}
	updateQueueLabels();
}

void Ncrok::prevTrack(){
	if(playlist.size() == 0) return;
	if((activetrack = playlist.prevTrack()) != NULL){
		bottom.printTitle(activetrack->getMenuText());
		playlist.play();
		updateQueueLabels();
	}
}

void Ncrok::updatePanels(){
	lockDisplay();
	update_panels();
	doupdate();
	unlockDisplay();
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

	bottom.printCentered(str,2);
}

/*
 * Redraw everything. Go corrupted data
 */
void Ncrok::reDraw(){
	updatePanels();
	redrawwin(stdscr);
}


void Ncrok::resizeTerm(){
	std::deque<Window *>::iterator win_iter;
	//Get the window size directly
	struct winsize ws;
	int fd = open("/dev/tty", O_RDWR);

	lockDisplay();

	if(ioctl(fd,TIOCGWINSZ,&ws)!=0) return;

	resizeterm(ws.ws_row, ws.ws_col);

	numRows = ws.ws_row;
	numCols = ws.ws_col;

	for(win_iter = windows.begin(); win_iter != windows.end(); win_iter++){
		(*win_iter)->redraw(numCols, numRows);
	}

	updatePanels();

	unlockDisplay();
}

static void resizeWin(int){
	Ncrok::app.resizeTerm();
}

