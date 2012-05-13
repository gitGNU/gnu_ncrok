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

#include <deque>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <ncurses.h>
#include <algorithm>
#include <regex.h>
#include <string>
#include <iostream>

#include "playlist.h"
#include "tune.h"


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
	return &list[index];
}

bool Playlist::addTrack(const std::string &filename){
	list.push_back(Tune(filename));
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
		list[currIndex].stopafter = 0;
		return NULL;
	}
	if(play_queue.size() > 0){
		currIndex = play_queue[0];
		play_queue.erase(play_queue.begin());
		list[currIndex].queue_index = -1;
		dequeued = currIndex;
		decrementQueue(0);
		return &list[currIndex];
	}
	if(currIndex == list.size() - 1)
		currIndex = 0; //then increment
	return &list[++currIndex];
}

Tune *Playlist::prevTrack(){
	if(currIndex == 0) return &list[currIndex];
	return &list[--currIndex];
}

Tune *Playlist::activeTrack(){
	return &list[currIndex];
}

MENU *Playlist::getMenu(){
	ITEM **items = (ITEM**)calloc(list.size()+1, sizeof(ITEM*));
	char *tmp;
	uint32_t count = 0;
	for(list_t::iterator iter = list.begin(); iter != list.end(); iter++){
		items[count] = (*iter).getItem();
		(*iter).index = count;
		set_item_userptr(items[count++], &(*iter));
	}
	items[list.size()] = NULL;
	playmenu = new_menu(items);
}

/*
 * Recursively (not yet) add a directory and return the number of files found
 */
int Playlist::readDir(const std::string &path){
	std::string dirPath, subPath;
	DIR *dir;
	int count = 0;
	struct dirent *dit;
	char *ext;
	int subcount;

	if(path.length() == 0) return 0;

	if(path[0] != '/'){
		getCwd(dirPath);
		//if(*(dirPath.end() - 1) != '/')
		//	dirPath.append('/', 1);
		dirPath.append(path);
	} else {
		dirPath.assign(path);
	}

	//if(*(dirPath.end() - 1) == '/')
	//	dirPath.erase(dirPath.end()-1, dirPath.end());

	if((dir = opendir(dirPath.c_str())) == NULL) return 0;

	while((dit = readdir(dir)) != NULL){
		if(dit->d_name[0] != '.'){//Eliminate hidden dirs and other pointers
			subPath.clear();
			subPath.assign(dirPath);
			subPath.append(1, '/');
			subPath.append(dit->d_name);

			subcount = readDir(subPath);
			if(subcount > 0){
				count += subcount;
			} else {
				for(ext = dit->d_name + strlen(dit->d_name) - 4; ext > dit->d_name && *ext != '.'; ext--);
				if(*ext != '.')
					continue;
				if(strncmp(ext,".mp3", 4)==0 || strncmp(ext,".ogg", 4)==0 || strncmp(ext,".m4a", 4)==0){
					addTrack(subPath);
					count++;
				}
			}
		}
	}
	closedir(dir);

	return count;
}

void Playlist::play(int index){
	currIndex = index;
	activeTrack()->play();
}

void Playlist::play(Tune &tune){
	currIndex = tune.index;
	tune.play();
}

void Playlist::play(){
	activeTrack()->play();
}

/*
 * returns true if track was already in queue and numbering needs to be refreshed
 */
void Playlist::queue(int index){
	play_queue.push_back(index);
	list[index].queue_index = play_queue.size()-1;
}

void Playlist::queue(Tune &tune){
	play_queue.push_back(tune.index);
	tune.queue_index = play_queue.size()-1;
}

bool Playlist::dequeue(Tune &tune){
	int pos = tune.queue_index;
	if(pos < 0) return false;
	tune.queue_index = -1;
	play_queue.erase(play_queue.begin() + pos);
	dequeued = tune.index;
	decrementQueue(pos);
	return true;
}

void Playlist::toggleQueue(Tune &tune){
	if(tune.queue_index == -1){
		queue(tune);
	} else {
		dequeue(tune);
	}
}

void Playlist::decrementQueue(int start){
	int size = play_queue.size();
	for(int i = start; i < size; i++){
		list[play_queue[i]].queue_index--;
	}
}

void Playlist::stopAfter(Tune &tune){
	prevstopafter = stopafter;
	if(stopafter == tune.index){
		stopafter = -1;
		tune.stopafter = false;
		return;
	}
	if(prevstopafter != -1)
		list[prevstopafter].stopafter = false;
	stopafter = tune.index;
	tune.stopafter = true;
}

int Playlist::getFirst(char *str) const{
	int i;
	for(i = 0; i < list.size(); i++){
		if(list[i].startsWith(str)) return i;
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
		if(list[i].query(search_exprs)){
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
		if(list[i].query(search_exprs)){
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
	while(play_queue.size() != 0){
		toggleQueue(list[play_queue.back()]);
	}
	if(stopafter > -1)
		stopAfter(list[stopafter]);
	dequeued = -1;
	prevstopafter = -1;
}

void Playlist::remove(Tune &tune){
	list_t::iterator iter;
	play_queue_t::iterator queue_iter;
	uint32_t index;
	if(tune.queue_index >= 0)
		toggleQueue(tune);
	if(tune.stopafter)
		stopAfter(tune);

	index = tune.index;

	// Adjust the index of the track to stop after
	if(stopafter > index)
		stopafter -= 1;

	// Adjust the indices of play queue pointers for items after this
	for(queue_iter = play_queue.begin(); queue_iter != play_queue.end(); queue_iter++){
		if((*queue_iter) > index)
			*queue_iter -= 1;
	}

	// Adjust the indices of all following tracks
	for(iter = list.begin() + index + 1; iter != list.end(); iter++){
		(*iter).index -= 1;
	}

	list.erase(list.begin() + index);

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
		std::sort(list.begin(),list.end(),&Tune::tune_compare);
}

int Playlist::load(const std::string &filename){
	FILE *fp;
	uint32_t count;
	if((fp = fopen(filename.c_str(), "r")) == NULL) return -1;
	//Read number of records
	fseek(fp, 0, SEEK_SET);
	if(fread(&count, sizeof(count), 1, fp) < 1)
		return -1;

	struct tune_block block;
	int i;

	for(i = 0; i < count; i++){
		if(fread(&block, sizeof(struct tune_block), 1, fp) < 1)
			return -1;
		list.push_back(Tune(block));
	}
	fclose(fp);
	return i;
}

int Playlist::save(const std::string &filename) const{
	FILE *fp;
	struct tune_block block;
	if((fp = fopen(filename.c_str(), "w")) == NULL) return -1;
	fseek(fp, sizeof(int), SEEK_SET);
	int max = list.size();
	uint32_t count = 0;
	int i;
	for(i = 0; i < max; i++){
		list[i].getBlock(block);
		if(fwrite(&block, sizeof(block), 1, fp) > 0){
			count++;
		}
		// Reset file pointer to the last valid write
		else fseek(fp, sizeof(block) * count, SEEK_SET);
	}
	fseek(fp, 0, SEEK_SET);
	if(fwrite(&count, sizeof(count), 1, fp) < 1)
		return -1; // did not save item count
	fclose(fp);
	return count;
}

//////////////////////////////////////////////////////////////////////////////
// Statics
//////////////////////////////////////////////////////////////////////////////

void Playlist::getCwd(std::string &str){
	char *tmp = get_current_dir_name();
	str.assign(tmp);
	free(tmp);
}
