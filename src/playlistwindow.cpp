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

#include "playlistwindow.h"
#include <cmath>

PlaylistWindow::PlaylistWindow() :
	Window(),
	menu(NULL)
{

}

void PlaylistWindow::display(int screenCols, int screenRows){
	init(0, 0, screenCols, screenRows - 5);

	meta(window, TRUE);
	unpost_menu(menu);
	set_menu_sub(menu, derwin(window, h-2, w-2, 1, 1));
	set_menu_format(menu, h-2, 1);
	set_menu_mark(menu, 0);
	keypad(window, TRUE);
	post_menu(menu);

	show();
}

void PlaylistWindow::redraw(int screenCols, int screenRows){
	ITEM *item = getCurrentItem();
	resize(0, 0, screenCols, screenRows - 5);

	unpost_menu(menu);
	set_menu_sub(menu, derwin(window, h-2, w-2, 1, 1));
	set_menu_format(menu, h-2, 1);
	set_menu_mark(menu, 0);
	keypad(window, TRUE);
	post_menu(menu);

	set_current_item(menu, item);

	Window::display(screenCols, screenRows);
}

void PlaylistWindow::assignMenu(MENU *m){
	if(menu != NULL){
		free_menu(menu);
	}
	menu = m;
	set_menu_win(menu, window);

	set_menu_sub(menu, derwin(window, h-2, w-2, 1, 1));
	set_menu_format(menu, h - 2, 1);
	set_menu_mark(menu, 0);
	post_menu(menu);
}

void PlaylistWindow::setItem(ITEM *item){
	int ind = item_index(item);
	if(menu->nitems - ind < h){
		// Item is on last possible page
		// Jump to the last possible page
		set_top_row(menu, std::max(0, menu->nitems - h));
	} else if(std::abs(ind - top_row(menu)) > h){
		// Item is not on current page
		// Jump to have it centered
		set_top_row(menu, std::max(ind - h/2, 0));
	}
	set_current_item(menu, item);
}

void PlaylistWindow::scrollDown(int count){
	while(count--){
		menu_driver(menu, REQ_DOWN_ITEM);
	}
}

void PlaylistWindow::scrollUp(int count){
	while(count--){
		menu_driver(menu, REQ_UP_ITEM);
	}
}
