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

#ifndef _PLAYLIST_WINDOW_H
#define _PLAYLIST_WINDOW_H

#include "window.h"

class PlaylistWindow : public Window {
public:
    PlaylistWindow();

    void assignMenu(MENU *m);
    void scrollUp(int count);
    void scrollDown(int count);
    int itemCount(){return item_count(menu);}
    ITEM *getCurrentItem(){return current_item(menu);}
    void posCursor(){pos_menu_cursor(menu);}
    void setItem(ITEM *item){
		int ind;
		ind = item_index(item);
		if(ind > h){
			set_top_row(menu, std::max(ind - h/2, 0));
		}
		set_current_item(menu, item);
	}
    ITEM **getItems(){return menu_items(menu);}
    void unpost(){unpost_menu(menu);}
    void post(){post_menu(menu);}

    void display(int screenCols, int screenRows);
    void redraw(int screenCols, int screenRows);
private:
    MENU *menu;
};

#endif
