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

#ifndef _WINDOW_H
#define _WINDOW_H
#include <ncurses.h>
#include <panel.h>
#include <menu.h>

#include <string>


class Window{
public:
	Window();
	virtual ~Window();

	void init(int x, int y, int w, int h);
	void initCenter(int x, int y, int w, int h);

	void resize(int x, int y, int w, int h);
	void resizeCenter(int x, int y, int w, int h);

	int getHeight(){return h;}
	int getWidth(){return w;}
	MENU* addMenu(ITEM** items);
	WINDOW* getWindow(){return window;}
	//PANEL* getPanel(){return panel;}
	MENU*	getMenu(){return menu;}
	//void pointPanelTo(Window* win);
	void printTitle(const std::string &ntitle);
	void printCentered(const std::string &text, int y);

	void hide();
	virtual void show();
	void reBox();
	void clear();
	void touch();
	void setPanelTop(){top_panel(panel);}

	void setScrollable(bool s){scrollok(window, s);}

	virtual void display(int screenCols, int screenRows);
	virtual void redraw(int screenCols, int screenRows) = 0;

	size_t innerWidth() const {return w - 2;}
	size_t innerHeight() const {return h - 2;}

protected:
	void init();

	std::string title;
	MENU* menu;
	int x, y, w, h;
	WINDOW* window;
	PANEL* panel;
};

#endif
