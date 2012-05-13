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

#include <iostream>

#include "helpwindow.h"

const std::string HelpWindow::text =
"Key        Action\n" \
"Up/Dn      Navigate the library\n" \
"Pup/Pdn    Navigate the library faster\n" \
"o          Add path to library and sort\n" \
";          Save playlist -- This playlist is saved in ~/.ncroklst\n" \
"l          Load playlist\n" \
"Enter      Play track\n" \
"[          Previous track\n" \
"]          Next track\n" \
"p          Pause track\n" \
"s          Stop\n" \
"Right/left 5 second relative seek\n" \
"< and >    20 second coarse relative seek\n" \
".          Jump to first match from beginning of menu string. ESC exits this search mode.\n" \
"/          POSIX regex search. This will find the first match, then arrow keys navigate other results.\n" \
"a          Toggle track queueing. This list will be processed upon completion of the current track and, upon completion, will continue to play from the last track in the queue.\n" \
"z          Toggle stop after track. The playlist will stop after the next time the track is played.\n" \
"DELETE     Remove a track from the playlist. Does not delete the file.\n" \
"j          Jump to currently playing track and cycle through queue\n" \
"jz         Stop after currently playing track. This isn't really its own command, but I use the combination frequently enough to mention it.\n" \
"^l         Redraw the screen.\n" \
"q          Quit";

HelpWindow::HelpWindow() :
	Window(),
	first_line(0)
{

}

void HelpWindow::display(int screenCols, int screenRows){
	initCenter(screenCols/2, screenRows/2, 55, 20);

	Window::display(screenCols, screenRows);
}

void HelpWindow::redraw(int screenCols, int screenRows){
	resizeCenter(screenCols/2, screenRows/2, 55, 20);

	Window::display(screenCols, screenRows);
}

void HelpWindow::show(){
	std::deque<std::string>::iterator line_iter;

	shapeText();

	line_iter = lines.begin();

	for(int i = 0; (i < innerHeight()) && (line_iter != lines.end()); i++, line_iter++){
		mvwprintw(window, i + 1, 1, (*line_iter).c_str());
	}

	refresh();
	Window::show();
	printTitle("Help");
	setScrollable(true);
}

void HelpWindow::scrollDown(){
	if(first_line + innerHeight() + 1 < lines.size()){
		first_line++;
		wscrl(window, 1);
		for(int i = 1; i < innerWidth() + 1; i++){
			mvwprintw(window, innerHeight(), i, " ");
		}
		mvwprintw(window, innerHeight(), 1, lines[first_line + innerHeight()].c_str());
		reBox();
	}
}

void HelpWindow::scrollUp(){
	if(first_line > 0){
		first_line--;
		wscrl(window, -1);
		for(int i = 1; i < innerWidth() + 1; i++){
			mvwprintw(window, 1, i, " ");
		}
		mvwprintw(window, 1, 1, lines[first_line].c_str());
		reBox();
	}
}

void HelpWindow::shapeText(){
	lines.clear();

	std::string extract_str;
	bool eol, was_newline;
	size_t next_line, line_len;

	next_line = 0;
	line_len = innerWidth();
	was_newline = true;

	do{
		next_line = extract_line(text, next_line, line_len, extract_str, eol);

		if(!was_newline)
			extract_str.insert(0, continuation_indent, ' ');

		lines.push_back(extract_str);

		// Set length of next line
		line_len = innerWidth();
		if(!eol)
			 line_len -= continuation_indent;

		was_newline = eol;
	} while(next_line != -1);
}

//////////////////////////////////////////////////////////////////////////////
// Statics
//////////////////////////////////////////////////////////////////////////////


size_t HelpWindow::extract_line(const std::string &str, size_t start, size_t len, std::string &out, bool &eol){
	size_t next_newline, last_space;

	// Make true unless later determined to not be EOL
	eol = true;

	next_newline = str.find_first_of('\n', start);

	if(next_newline == std::string::npos){
		// No newline in the rest of the
		next_newline = str.length();
		eol = false;
	}

	if((next_newline - start - 1) <= len){
		// The whole line fits
		out.assign(str.substr(start, next_newline - start));
		if(!eol)
			return -1;

		return next_newline + 1;
	}

	eol = false;

	next_newline = std::min(next_newline - start, len) + start;

	// Now look for a suitable break since it doesn't all fit
	last_space = str.find_last_of(' ', next_newline);

	if((last_space == std::string::npos) || (last_space < start)){
		return -1;
		// No suitable gap found
		out.assign(str.substr(start, next_newline - start));
		return next_newline + 1;
	}

	out.assign(str.substr(start, last_space - start));
	return last_space + 1;
}
