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
#include "log.h"
#include "time_manager.h"

TimeManager::TimeManager()
{
    reset();
}

void TimeManager::reset()
{
    for( int i=0; i<MaxTimeControls; i++ ) {
        control_[i].time = 0;
        control_[i].moves = 0;
        control_[i].increment = 0;
    }
    
    numControls_ = 0;
    curControlIndex_ = 0;
    movesLeftInControl_ = 0;
    movesPlayed_ = 0;
}

const TimeControl & TimeManager::currControl() const
{
    return control_[ curControlIndex_ ];
}

const TimeControl & TimeManager::nextControl() const
{
    int index = curControlIndex_ < (numControls_-1) ? 
        curControlIndex_ + 1 :
        numControls_ - 1;
    
    return control_[index];
}

bool TimeManager::skipMultipleMoves( int n )
{
    bool result = false;

    while( n > 0 ) {
        result |= goNextMove();

        n--;
    }

    return result;
}

bool TimeManager::goNextMove()
{
    bool result = false;
    
    movesLeftInControl_--;
    movesPlayed_++;
    
    if( movesLeftInControl_ == 0 ) {
        // Go to next time control
        curControlIndex_ = getNextIndex();
        
        movesLeftInControl_ = control_[ curControlIndex_ ].moves;
        
        result = true;
    }
    
    return result;
}

bool TimeManager::goPrevMove()
{
    bool result = false;
    
    movesLeftInControl_++;
    movesPlayed_--;
    
    if( movesLeftInControl_ > control_[ curControlIndex_ ].moves ) {
        // Go to previous time control
        movesLeftInControl_ = 1;

        curControlIndex_ = getPrevIndex();
        
        result = true;
    }

    return result;
}
    
int TimeManager::getNextIndex() const
{
    int result = curControlIndex_ + 1;
    
    return result >= numControls_ ? numControls_ - 1 : result;
}

int TimeManager::getPrevIndex() const
{
    return curControlIndex_ > 0 ? curControlIndex_ - 1 : 0;
}

int TimeManager::addControl( int moves, int time, int increment )
{
    if( numControls_ < MaxTimeControls ) {
        control_[ numControls_ ].moves = moves;
        control_[ numControls_ ].time = time;
        control_[ numControls_ ].increment = increment;
        
        if( numControls_ == 0 ) {
            movesLeftInControl_ = moves;
        }
        
        numControls_++;
    }
    
    return 0;
}
