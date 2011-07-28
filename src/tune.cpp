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

#include <tag.h>
#include <fileref.h>
#include "tune.h"
#include "util.h"
#include <menu.h>
#include "output.h"
#include "window.h"
#include <string.h>
#include <regex.h>

Tune::Tune(char *name){
	*title = 0;
	*album = 0;
	*artist = 0;
	*displayName = 0;
	track = 0;
	year = 0;
	if(strcopy(filename, name, TUNE_LEN_FNAME) == TUNE_LEN_FNAME){
		fprintf(stderr, "File path is too long. Max: %d\n",TUNE_LEN_FNAME);
		return;
	}
	parseFile();
	genDisplay();
	queue_index = -1;
	stopafter = 0;
}

void Tune::genDisplay(){
	char buffer[1024];
	if(strlen(title) > 0){
		sprintf(buffer, "%s - %s [%02d] - %s", artist, album, track, title);
		buffer[TUNE_LEN_DISP-1] = 0;
		strcpy(displayName,buffer);
	}
	else{
		int i;
		//Find filename start index
		for(i = strlen(filename) - 1; filename[i] != '/' && i >= 0; i--){}
		//Copy tail of file path
		if(strlen(&filename[i + 1]) + strlen(artist) < TUNE_LEN_DISP){
			sprintf(buffer, "%s - %s", artist, &filename[i+1]);
			buffer[TUNE_LEN_DISP-1] = 0;
			strcpy(displayName,buffer);
		}
		else if(strlen(&filename[i + 1]) < TUNE_LEN_DISP){
			strcpy(displayName, &filename[i + 1]);
		}
		else {
			memcpy((void*)displayName,&filename[i + 1], min(TUNE_LEN_DISP - 1, TUNE_LEN_FNAME - (i + 1)) );
			displayName[min(TUNE_LEN_DISP - 1, TUNE_LEN_FNAME - (i + 1))] = 0;
		};
	}
}

Tune::Tune(struct tune_block *block){
	memcpy((void *)filename, (void *)block->filename, TUNE_LEN_FNAME);
	memcpy((void *)artist, (void *)block->artist, TUNE_LEN_ARTIST);
	memcpy((void *)album, (void *)block->album, TUNE_LEN_ALBUM);
	memcpy((void *)title, (void *)block->title, TUNE_LEN_TITLE);
	track = block->track;
	year = block->year;
	
	queue_index = -1;
	stopafter = 0;
	genDisplay();
}


struct tune_block *Tune::getBlock(){
	struct tune_block *block = (struct tune_block *)malloc(sizeof(struct tune_block));
	memcpy((void *)block->filename, (void *)filename, TUNE_LEN_FNAME);
	memcpy((void *)block->artist, (void *)artist, TUNE_LEN_ARTIST);
	memcpy((void *)block->album, (void *)album, TUNE_LEN_ALBUM);
	memcpy((void *)block->title, (void *)title, TUNE_LEN_TITLE);
	block->track = track;
	block->year = year;
	return block;
}


Tune::~Tune(){
	
}


char *Tune::getMenuText(){
	return displayName;
}

ITEM *Tune::getItem(){
	// To be freed by ncurses, which uses this space instead of copying it
	char *item_text = (char *)malloc(TUNE_LEN_ITEM);
	sprintf(item_text, "   ");
	if(queue_index != -1){
		if(queue_index > 8)
			sprintf(item_text,"%d %s",queue_index+1, displayName);
		else sprintf(item_text,"%d  %s",queue_index+1, displayName);
	}
	else {
		strcpy(item_text + 3, displayName);
	}
	if(stopafter) item_text[2] = '*';
	item_text[TUNE_LEN_ITEM - 1] = 0;

	return new_item(item_text,NULL);
}

void Tune::updateItem(ITEM *item){
	char* item_text = (char*)malloc(TUNE_LEN_ITEM);
	sprintf(item_text, "   ");
	if(queue_index != -1){
		if(queue_index > 8)//needs 2 digits
			sprintf(item_text,"%d %s",queue_index+1, displayName);
		else sprintf(item_text,"%d  %s",queue_index+1, displayName);
	}
	else sprintf(item_text,"   %s",displayName);
	if(stopafter) item_text[2] = '*';
	free((void*)item->name.str);
	item->name.str = item_text;
}

bool Tune::startsWith(char* str){
	int len = strlen(str);
	char i;
	for(i = 0; i < strlen(str); i++){
		if(compare_i(str[i],displayName[i]) == 0) return false;
	}
	return true;
}

bool Tune::query(regex_t **terms){
	char i;
	for(i = 0; terms[i] != NULL; i++){
		if(regexec(terms[i], artist, 0, NULL, 0) != 0 &&
		   regexec(terms[i], title, 0, NULL, 0) != 0 &&
		   regexec(terms[i], album, 0, NULL, 0) != 0)
		return false;
	}
	return true;
}

void Tune::parseFile(){
	//Use a big buffer to cut down the size of the text copied
	char buffer[1024];
	char *tmpfile = (char *)malloc(TUNE_LEN_FNAME);
	strcpy(tmpfile, filename);
	TagLib::FileRef f(tmpfile);
	if(f.tag()->isEmpty()) {
		guessFile();
		return;
	}
	TagLib::String tmp = f.tag()->artist();
	strcpy(buffer,tmp.toCString());
	cleanString(buffer, TUNE_LEN_ARTIST);
	strcpy(artist,buffer);

	if(strlen(artist) != 0){ //Do the rest
		tmp = f.tag()->album();
		strcpy(buffer,tmp.toCString());
		cleanString(buffer, TUNE_LEN_ALBUM);
		strcpy(album, buffer);
	
		tmp = f.tag()->title();
		strcpy(buffer,tmp.toCString());
		cleanString(buffer, TUNE_LEN_TITLE);
		strcpy(title, buffer);
	
		track = f.tag()->track();
		year = f.tag()->year();
	} else guessFile();
}

void Tune::guessFile(){
	int length = strlen(filename);
	char buffer[512];
	for(int i = length; i >= 0; i--){
		if(filename[i] == '/'){
			strcpy(buffer, &(filename[i+1]));
			cleanString(buffer, TUNE_LEN_TITLE);
			strcpy(title, buffer);
			return;
		}		
	}

}

void Tune::play(){
	char* path = (char*)malloc(512);
	sprintf(path,"file://%s",filename);
	play_path(path);
}

char *Tune::getArtist(){ return artist; }
char *Tune::getTitle(){ return title; }
char *Tune::getAlbum(){ return album; }
uint32_t Tune::getTrack(){ return track; }
uint32_t Tune::getYear(){ return year; }

bool tune_compare(Tune* a, Tune* b){
	int result;
	result = strcmp(a->getArtist(), b->getArtist());
	if(result != 0)
		return (result < 0);
	result = strcmp(a->getAlbum(), b->getAlbum());
	if(result != 0)
		return (result < 0);
	result = a->getTrack() - b->getTrack();
	if(result != 0)
		return (result < 0);
	result = strcmp(a->getTitle(), b->getTitle());
	return (result < 0);
}

static bool compare_i(char a, char b){
	if(a == b) return true;
	if(a + 32 == b) return true;
	if(b + 32 == a) return true;
	return false;
}

static void cleanString(char *in, int maxlen){
	int i;
	for(i = 0; i < maxlen; i++){
		if(*in > 31 && *in < 127){
			in++;
			continue;
		}
		if(*in == 0)
			break;
		if(i == maxlen - 1){
			*in = 0;
			break;
		}
		*(in++) = '_';
	}
}
