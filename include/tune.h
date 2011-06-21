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
#ifndef _TUNE_H
#define _TUNE_H

#include <menu.h>
#include <string>
#include <regex.h>
#include <stdint.h>
#define TUNE_LEN_FNAME	512
#define TUNE_LEN_TITLE	96
#define TUNE_LEN_ALBUM	96
#define TUNE_LEN_ARTIST	96
#define TUNE_LEN_DISP	128
#define TUNE_LEN_ITEM	TUNE_LEN_DISP + 6

//For storing in a single file
struct tune_block {
	char filename[TUNE_LEN_FNAME];
	char artist[TUNE_LEN_ARTIST];
	char album[TUNE_LEN_ALBUM];
	char title[TUNE_LEN_TITLE];
	uint32_t track;
	uint32_t year;
};


class Tune {
	public:
		Tune(char *name);
		Tune(struct tune_block *block);
		char *getTitle();
		char *getArtist();
		char *getAlbum();
		uint32_t getTrack();
		uint32_t getYear();
		char *getMenuText();
		bool startsWith(char* str);
		ITEM *getItem();
		void updateItem(ITEM *item);
		void play();
		void pause();
		void stop();
		~Tune();
		struct tune_block *getBlock();
		//Null terminated
		bool query(regex_t **terms);

		char filename[TUNE_LEN_FNAME];
		int32_t	queue_index;
		bool stopafter;
	protected:
		void genDisplay();
		void parseFile();
		void guessFile();

		char artist[TUNE_LEN_ARTIST];
		char album[TUNE_LEN_ALBUM];
		char title[TUNE_LEN_TITLE];
		char displayName[TUNE_LEN_DISP];
		uint32_t track, year;
};

bool tune_compare(Tune* a, Tune* b);
static bool compare_i(char a, char b);
static void cleanString(char *in, int maxlen);
#endif
