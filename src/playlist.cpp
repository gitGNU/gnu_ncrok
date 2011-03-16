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

#include "playlist.h"
#include "tune.h"
#include "util.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <ncurses.h>
#include <algorithm>
#include <regex.h>


Playlist::Playlist(){
	currIndex=0;
	dequeued = -1;
	stopafter = -1;
	prevstopafter = -1;
	search_term = (char **)calloc(MAX_SEARCH_TERMS + 1, sizeof(char*));
	search_exprs = (regex_t **)calloc(MAX_SEARCH_TERMS + 1, sizeof(regex_t*));
	search_index = -1;
}

Playlist::~Playlist(){
	list.erase(list.begin(),list.end());
	play_queue.erase(play_queue.begin(),play_queue.end());
	if(playmenu != NULL){
		if(item_count(playmenu) > 0){
			free_menu(playmenu);
			free(playmenu);
		} else free_menu(playmenu);
	}
}

Tune *Playlist::operator[](const int index){
	return list[index];
}

bool Playlist::addTrack(char *filename){
	Tune* tmp = new Tune(filename);
	list.push_back(tmp);
	//printw("%s\n",list[list.size()-1]->getTitle());
	//currIndex = list.size() - 1;
	return 1;
}

int Playlist::size(){
	return list.size();
}

Tune *Playlist::nextTrack(){
	if(stopafter == currIndex){
		//clear stopafter flag
		prevstopafter = currIndex;
		stopafter = -1;
		list[currIndex]->stopafter = 0;
		return NULL;
	}
	if(play_queue.size() > 0){
		currIndex = play_queue[0];
		play_queue.erase(play_queue.begin());
		list[currIndex]->queue_index = -1;
		dequeued = currIndex;
		decrementQueue(0);
		return list[currIndex];
	}
	if(currIndex == list.size() - 1)
		currIndex = 0; //then increment
	return list[++currIndex];
}

Tune *Playlist::prevTrack(){
	if(currIndex == 0) return list[currIndex];
	return list[--currIndex];
}

Tune *Playlist::activeTrack(){
	return list[currIndex];
}

MENU *Playlist::getMenu(){
	ITEM **items = (ITEM**)calloc(list.size()+1, sizeof(ITEM*));
	char *tmp;
	for(int i = 0; i < list.size(); i++){
		items[i] = list[i]->getItem();
		set_item_userptr(items[i], (void*)i);
	}
	items[list.size()] = NULL;
	playmenu = new_menu(items);
}

/*
 * Recursively (not yet) add a directory and return the number of files found
 */
int Playlist::readDir(const char* path){
	//Make sure this path isn't too long
	if(strlen(path) >= TUNE_LEN_FNAME) return 0;
	//copy to editable region
	char directory[TUNE_LEN_FNAME];
	if(path[0] != '/'){//gstreamer will want absolute paths
		int start = 0;
		char *ptr = getcwd(directory, TUNE_LEN_FNAME);
		while(directory[start] != 0){start++;} //Increment start point to end of cwd
		directory[start] = '/';
		strcopy(&directory[start + 1],path,TUNE_LEN_FNAME-(start+1));//Then copy it, getcwd includes trailing slash so get rid of it
	}
	else
		strcopy(directory, path,TUNE_LEN_FNAME);
	//Strip trailing slash. Don't listen to it. Unless it's root dir.
	if(directory[strlen(directory) - 1] == '/' && strlen(directory) > 1)
		directory[strlen(directory) - 1] = 0;
	DIR *dir;
	if((dir = opendir(directory)) == NULL) return 0;
	int count = 0;
	struct dirent *dit;
	char *tmpname = (char*)malloc(TUNE_LEN_FNAME);
	char *ext;
	int subcount;
	while((dit = readdir(dir)) != NULL){
		if(dit->d_name[0] != '.'){//Eliminate hidden dirs and other pointers
			sprintf(tmpname,"%s/%s", directory, dit->d_name);
			subcount = readDir(tmpname);
			if(subcount > 0){
				count += subcount;
			}
			else if(strlen(dit->d_name) > 5){
				ext = (char*)((uint64_t)dit->d_name + 
					(uint64_t)(strlen(dit->d_name)-4));
				if(strcmp(ext,".mp3")==0 || strcmp(ext,".ogg")==0){
					addTrack(tmpname);
					//printw("%u\n", list[list.size()-1]);
					//printw("%s\n", activeTrack()->getAlbum());
					count++;
				}
			}
		}
	}
	closedir(dir);
	free(tmpname);
	return count;
}

void Playlist::play(int index){
	currIndex = index;
	activeTrack()->play();
}

void Playlist::play(){
	activeTrack()->play();
}

/*
 * returns true if track was already in queue and numbering needs to be refreshed
 */
void Playlist::queue(int index){
	play_queue.push_back(index);
	list[index]->queue_index = play_queue.size()-1;
}

/*
 * returns true if index found
 */
bool Playlist::dequeue(int index){
	int pos = list[index]->queue_index;
	if(pos < 0) return false;
	list[index]->queue_index = -1;
	play_queue.erase(play_queue.begin() + pos);
	dequeued = index;
	decrementQueue(pos);
}

void Playlist::toggleQueue(int index){
	if(list[index]->queue_index == -1){
		queue(index);
	}
	else{
		dequeue(index);
	}
}

int Playlist::queueSize(){
	return play_queue.size();
}

int *Playlist::getQueue(){
	int *ret = (int *)calloc(queueSize(),sizeof(int));
	std::vector<uint32_t>::iterator queue_iter;
	queue_iter = play_queue.begin();
	int i = 0;
	while(queue_iter != play_queue.end()){
		ret[i++] = *queue_iter;
		queue_iter++;
	}
	return ret;
}

void Playlist::decrementQueue(int start){
	int size = play_queue.size();
	for(int i = start; i < size; i++){
		list[play_queue[i]]->queue_index--;
	}
}

/*
 *  Toggle stop-after for a track at index
 */
void Playlist::stopAfter(int index){
	prevstopafter = stopafter;
	if(stopafter == index){
		stopafter = -1;
		list[index]->stopafter = 0;
		return;
	}
	if(prevstopafter != -1)
		list[prevstopafter]->stopafter = 0;
	stopafter = index;
	list[index]->stopafter = 1;
}

int Playlist::getFirst(char *str){
	int i;
	for(i = 0; i < list.size(); i++){
		if(list[i]->startsWith(str)) return i;
	}
	return -1;
}

int Playlist::search(char *str){
	int i;
	for(i = 0; search_term[i] != NULL; i++){
		free((void*) search_term[i]);
		free((void*) search_exprs[i]);
	}
	int start = 0;
	int count = 0;
	int max = strlen(str);
	int debug;
	for(i = 0; i <= max; i++){
		if((str[i] == ' ' || str[i] == 0) && i - start > 0){//space or EOS, not just after another space
			if(count + 1 == MAX_SEARCH_TERMS) break;
			search_term[count] = (char*)malloc((i - start) + 1);
			memcpy(search_term[count], &str[start], i - start);
			search_term[count][i - start] = 0; //terminate
			search_exprs[count] = (regex_t *)malloc(sizeof(regex_t));
			if(regcomp(search_exprs[count], search_term[count], REG_ICASE) != 0){
				//if regex doesnt compile, just skip this one
				free((void *)search_term[count]);
				free((void *)search_exprs[count]);
			}
			else ++count;
			start = i + 1;
		}
	}
	//return count;

	//NULL terminate the arrays
	search_exprs[count] = NULL;
	search_term[count] = NULL;
	
	max = size();
	if(search_results.size() > 0) i = search_results.front();
	else i = 0;
	clearSearch();
	for(; i < max; i++){
		if(list[i]->query(search_exprs)){
			search_results.push_back(i);
			search_index = 0;
			return i;
		}
	}
	return -1;
}

int Playlist::nextResult(){
	if(search_index == -1) return -1;
	if(search_index < search_results.size() - 1) //next result already found
		return search_results[++search_index];
	int max = size();
	int i;
	for(i = search_results.back() + 1; i < max; i++){
		if(list[i]->query(search_exprs)){
			search_results.push_back(i);
			++search_index;
			return i;
		}
	}
	return -1;
}

int Playlist::prevResult(){
	if(search_index > 0)
		return search_results[--search_index];
	return -1;
}

void Playlist::clearSearch(){
	search_index = -1;
	search_results.erase(search_results.begin(), search_results.end());
}

/*
 * void clearQueue(void):
 * clear the queue and stopafter
 * intended for use before adding additional tracks where indices may change
 */
void Playlist::clearQueue(){
	while(queueSize() != 0){
		toggleQueue(play_queue.back());
	}
	if(stopafter > -1)
		stopAfter(stopafter);
	dequeued = -1;
	prevstopafter = -1;
}

void Playlist::remove(int index){
	if(list[index]->queue_index >= 0)
		toggleQueue(index);
	if(list[index]->stopafter)
		stopAfter(index);
	free(list[index]);
	list.erase(list.begin() + index);
	correctList(index, -1);
}

void Playlist::correctList(int start, int change){
	int i;
	if(currIndex >= start)
		currIndex += change;
	if(stopafter >= start)
		stopafter += change;
	for(i = 0; i < play_queue.size(); i++)
		if(play_queue[i] >= start)
			play_queue[i] += change;
}

void Playlist::sort(){
	if(size() > 0)
		std::sort(list.begin(),list.end(),&tune_compare);
}

int Playlist::load(char *filename){
	FILE *fp;
	int count;
	if((fp = fopen(filename, "r")) == NULL) return -1;
	//Read number of records
	fseek(fp, 0, SEEK_SET);
	if(fread(&count, sizeof(int), 1, fp) < 1)
		return -1;
	
	struct tune_block block;
	Tune* tmptune;
	int i;
	for(i = 0; i < count; i++){
		if(fread(&block, sizeof(struct tune_block), 1, fp) < 1)
			return -1;
		tmptune = new Tune(&block);
		list.push_back(tmptune);
	}
	fclose(fp);
	return i;
}

int Playlist::save(char *filename){
	FILE *fp;
	if((fp = fopen(filename, "w")) == NULL) return -1;
	fseek(fp, sizeof(int), SEEK_SET);
	int max = list.size();
	struct tune_block *block;
	int count = 0;
	int i;
	for(i = 0; i < max; i++){
		if((block = list[i]->getBlock()) == NULL) return -1;
		if(fwrite(block, sizeof(struct tune_block), 1, fp) > 0){
			count++;
		}
		// Reset file pointer to the last valid write
		else fseek(fp, sizeof(struct tune_block) * count, SEEK_SET);
		free(block);
	}
	fseek(fp, 0, SEEK_SET);
	if(fwrite(&count, sizeof(int), 1, fp) < 1)
		return -1; // did not save item count
	fclose(fp);
	return count;
}
