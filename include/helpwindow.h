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

#ifndef __HELPWINDOW_H_
#define __HELPWINDOW_H_

#include <deque>

#include "window.h"

class HelpWindow : public Window {
public:
    HelpWindow();
    void scrollUp();
    void scrollDown();

    void show();

    void display(int screenCols, int screenRows);
    void redraw(int screenCols, int screenRows);
private:


    void shapeText();

    std::deque<std::string> lines;
    int first_line;

    static size_t extract_line(const std::string &str, size_t start, size_t len, std::string &out, bool &eol);

    static const size_t continuation_indent = 5;
    static const std::string text;
};

#endif
