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
		Tune(const std::string &name);
		Tune(struct tune_block &block);
		const std::string &getTitle() const;
		const std::string &getArtist() const;
		const std::string &getAlbum() const;
		uint32_t getTrack() const;
		uint32_t getYear() const;
		const std::string &getMenuText() const;
		bool startsWith(char* str) const;
		ITEM *getItem();
		void updateItem(ITEM *item);
		void play() const;
		void pause();
		void stop();
		~Tune();
		void getBlock(struct tune_block &block) const;
		//Null terminated
		bool query(regex_t **terms) const;

		std::string filename;
		uint32_t index;
		int32_t	queue_index;
		bool stopafter;

		static bool tune_compare(const Tune &a, const Tune &b);
	protected:
		void genDisplay();
		void parseFile();
		void guessFile();

		std::string artist;
		std::string album;
		std::string title;
		std::string displayName;
/*
		char artist[TUNE_LEN_ARTIST];
		char album[TUNE_LEN_ALBUM];
		char title[TUNE_LEN_TITLE];
		char displayName[TUNE_LEN_DISP];
*/
		uint32_t track, year;
};

static bool compare_i(char a, char b);
static void cleanString(char *in, int maxlen);
#endif
