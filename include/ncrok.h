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

#ifndef _NCROCK_H
#define _NCROCK_H

#include "window.h"
#include "playlistwindow.h"
#include "bottomwindow.h"
#include "helpwindow.h"
#include "playlist.h"

#include <ncursesw/ncurses.h>
#include <form.h>
#include <pthread.h>
#include <deque>

#define IN_UP		(char)KEY_UP
#define IN_DOWN		(char)KEY_DOWN
#define IN_PUP		(char)KEY_PPAGE
#define IN_PDOWN	(char)KEY_NPAGE
#define IN_OPEN		'o'
#define IN_PLAY_PAUSE	' '
#define IN_PAUSE	'p'
#define IN_REMOVE	'd'
#define IN_PREV		'['
#define IN_NEXT		']'
#define IN_HELP1    '?'
#define IN_HELP2    'h'
#define IN_PLAY		10
#define IN_STOP		's'
#define IN_QUIT		'q'
#define IN_FWD_FINE	(char)KEY_RIGHT
#define IN_BACK_FINE	(char)KEY_LEFT
#define IN_FWD_COARSE	'>'
#define IN_BACK_COARSE	'<'
#define IN_SEARCH	'/'
#define IN_SEARCH_START '.'
#define IN_QUEUE	'a'
#define IN_STOP_AFTER	'z'
#define IN_ESC		27
#define IN_SPACE	32
#define IN_BACKSPACE	8
#define IN_TAB		9
#define IN_REDRAW	12
#define IN_DELETE	74
#define IN_DELETE2	127
//Not sure why, but my machine gives BELL on backspace
#define IN_BELL		7
#define IN_SAVE		';'
#define IN_LOAD		'l'
#define IN_JUMP		'j'

#define TITLE_STRING	"Welcome to Ncrok 0.63! This is the future!"

class Ncrok {
	public:
		Ncrok();
		~Ncrok();
		void initialize(int tracks);
		void runPlaylist();
		void nextTrack();
		void prevTrack();
		void updateTime(char *pos, double rel);
		void setLength(char *len);
		void reDraw();
		void resizeTerm();

		Playlist playlist;

		PlaylistWindow right;
		BottomWindow bottom;

		static Ncrok app;

	private:
		void playSelected();
		void initWindows();
		void updateQueueLabels();
		void drawProgress(double progress);
		void findByFirst();
		void refreshPlaylist();
		void addDir();
		void search();
		void jumpToIndex(int index);
		void showHelpWindow();
		void deleteSelected();
		void stop();

		void updatePanels();
		void doJumpAction();

		void lockDisplay(){pthread_mutex_lock(&display_mutex);}
		void unlockDisplay(){pthread_mutex_unlock(&display_mutex);}
		pthread_mutex_t display_mutex;
		pthread_mutexattr_t attr;

		uint16_t numRows, numCols;

		Tune *activetrack;
		char length[16];
		std::deque<Window *> windows;
};


#endif
