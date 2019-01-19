/*
    Kiwi
    Copyright (c) 1999-2004,2005 Alessandro Scotti
    http://www.ascotti.org/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef TIME_MANAGER_H_
#define TIME_MANAGER_H_

const int MaxTimeControls = 3;  // Max number of time controls per game

struct TimeControl {
    int time;
    int moves;
    int increment;
};

class TimeManager
{
public:
    TimeManager();

    const TimeControl & currControl() const;

    const TimeControl & nextControl() const;
    
    int addControl( int moves, int time, int increment = 0 );
    
    void reset();

    bool skipMultipleMoves( int n );
    
    bool goNextMove();
    
    bool goPrevMove();
    
    int movesLeftInControl() const {
        return movesLeftInControl_;
    }

    int movesPlayed() const {
        return movesPlayed_;
    }

private:
    int getNextIndex() const;
    int getPrevIndex() const;

    TimeControl control_[ MaxTimeControls ];
    int curControlIndex_;
    int numControls_;
    int movesLeftInControl_;
    int movesPlayed_;
};

#endif // TIME_MANAGER_H_
