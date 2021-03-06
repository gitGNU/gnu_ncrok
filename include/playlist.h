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
#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include <deque>
#include <list>
#include <menu.h>
#include <regex.h>
#include <string>
#include <stdint.h>

#include "tune.h"

#define PLAYLIST_MARKER		"*"
#define MAX_SEARCH_TERMS	8


class Playlist {
	public:
		Playlist();
		Playlist(char *filename);
		bool addTrack(const std::string &filename);
		int size();
		~Playlist();
		Tune *nextTrack();
		Tune *prevTrack();
		Tune *activeTrack();
		int readDir(const std::string &path);
		MENU *getMenu();
		Tune *operator[](const int index);

		void play(int index);
		void play(Tune &tune);
		void play();
		void sort();
		void toggleQueue(Tune &tune);
		void stopAfter(Tune &tune);
		int getFirst(char *str) const;
		int search(char *str);
		int nextResult();
		int prevResult();
		void clearSearch();
		void clearQueue();
		void remove(Tune &tune);
		void correctList(int start, int change);
		int save(const std::string &filename) const;
		int load(const std::string &filename);
		int dequeued;
		int stopafter; //index
		int prevstopafter;

		uint32_t currIndex;

		typedef std::deque<uint32_t> play_queue_t;
		const play_queue_t &getQueue() const{return play_queue;}
	protected:
		void queue(int index);
		void queue(Tune &tune);
		bool dequeue(int index);
		bool dequeue(Tune &tune);
		void decrementQueue(int start);

		static void getCwd(std::string &str);

		MENU *playmenu;

		typedef std::deque<Tune> list_t;

		list_t list;
		list_t::iterator iter;
		char **search_term;//Null terminated
		regex_t **search_exprs;
		std::deque<int> search_results;
		int search_index;
		play_queue_t play_queue;
};

#endif
