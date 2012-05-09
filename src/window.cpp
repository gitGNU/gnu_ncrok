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

#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <string.h>
#include <string>
#include <cstdlib>

#include "window.h"

Window::Window(int xx, int yy, int ww, int hh){
	setDims(xx, yy, ww, hh);
}

Window::Window(){

}

Window::~Window(){
	delwin(window);
}

void Window::init(){
	window = newwin(h, w, y, x);
	panel = new_panel(window);
	box(window,0,0);
	//mvprintw(y,x,"Initializing window %u %u %u %u", w, h, x, y);
	wrefresh(window);
	update_panels();
	wmove(window,1,1);
	printTitle(title);
}

void Window::setDims(int xx, int yy, int ww, int hh){
	x = xx; y = yy; w = ww; h = hh;
	init();
}

void Window::setDimsFromCenter(int x, int y, int w, int h){
	setDims(x - w/2, y - h / 2, w, h);
}

void Window::getDims(int* xx, int* yy, int* ww, int* hh){
	*xx = x; *yy = y; *ww = w, *hh = h;
}

void Window::pointPanelTo(Window* win){
	set_panel_userptr(panel, win->getPanel());
}

void Window::reBox(){
	box(window,0,0); //overwrite any old text
	printCentered(title, 0);
}

void Window::clear(){
	werase(window);
	reBox();
	refresh();
}

void Window::printTitle(const std::string &ntitle){
	title.assign(ntitle);
	reBox();
}

void Window::printCentered(const std::string &text, int y){
	mvwprintw(window, y, (w/2)-(text.length()/2), "%s", text.c_str());
	wrefresh(window);
}

int Window::getHeight(){
	return h;
}

int Window::getWidth(){
	return w;
}

MENU* Window::addMenu(ITEM** items){
	menu = new_menu(items);
	set_menu_win(menu, window);
	return menu;
}

WINDOW* Window::getWindow(){
	return window;
}

PANEL* Window::getPanel(){
	return panel;
}

MENU* Window::getMenu(){
	return menu;
}

void Window::hide(){
	hide_panel(panel);
	update_panels();
	doupdate();
}

void Window::show(){
	show_panel(panel);
	update_panels();
	doupdate();
}

void Window::refresh(){
	wrefresh(window);
}

void Window::touch(){
	touchwin(window);
}

void Window::reDraw(){
	redrawwin(window);
}

void Window::doScroll(scroll_dir_t dir){
	switch(dir){
	case SCROLL_UP:
		wscrl(window, 1);
		break;
	case SCROLL_DN:
		wscrl(window, -1);
		break;
	}
}

