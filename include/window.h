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
#ifndef _WINDOW_H
#define _WINDOW_H
#include <ncurses.h>
#include <panel.h>
#include <menu.h>


class Window{
	public:
		Window(int xx, int yy, int ww, int hh);
		Window();
		~Window();
		void setDims(int x, int y, int w, int h);
		void getDims(int* xx, int* yy, int* ww, int* hh);
		int getHeight();
		int getWidth();
		MENU* addMenu(ITEM** items);
		WINDOW* getWindow();
		PANEL* getPanel();
		MENU*	getMenu();
		void pointPanelTo(Window* win);
		void printTitle(const char* ntitle);
		void printCentered(const char* text, int y);
		void refresh();
		void hide();
		void show();
		void reBox();
		void clear();
		void touch();
		void reDraw();
	private:
		char title[256];
		MENU* menu;
		int x, y, w, h;
		WINDOW* window;
		PANEL* panel;
		void init();
};

#endif
