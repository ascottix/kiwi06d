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
#include "engine.h"
#include "log.h"
#include "move.h"
#include "movelist.h"
#include "movehandler.h"
#include "position.h"
#include "recognizer.h"
#include "san.h"
#include "score.h"
#include "undoinfo.h"

const bool haveRecognizersInQuiesce = true;
const bool haveChecksInQuiesce      = true;
const bool haveHashInQuiesce        = true;

extern int nodesUntilInputCheck;

static int getRelativeEvaluation( const Position & pos )
{
    int result = pos.getEvaluation();

    if( pos.sideToPlay == Black ) {
        result = -result;
    }

    return result;
}

int Engine::negaMaxQuiesceMT( Position & pos, int gamma, int ply, int checks_depth )
{
    // Check for input every now and then
    if( --nodesUntilInputCheck <= 0 ) {
        isSearchOver();
    }

    if( searchMustBeInterrupted ) {
        return 0;
    }

    // Check for draw by 3-fold repetition or 50 moves rule
    int repCount = 0;

    if( pos.boardFlags & Position::PositionRepeatPossible ) {
        int hmc = pos.getHalfMoveClock();

        // Check the 50 move rule
        if( hmc >= 100 ) {
            return 0;
        }

        // It is possible that this position appeared before... check it
        int n = gameHistoryIdx + ply - 4;
        int e = gameHistoryIdx + ply - hmc;

        while( n >= e ) {
            if( pos.hashCode == rep3History[ n ].hashCode ) {
                // Found, update repetition count and exit from the loop
                repCount = 1 + rep3History[ n ].repCount;

                // Use 2-fold repetition during search
                return 0;

                // ...still leave the "break" here for safety!
                break;
            }

            n -= 2;
        }
    }

    if( ! pos.hasBlackPawns() && ! pos.hasWhitePawns() && (pos.numOfBlackPieces() + pos.numOfWhitePieces()) <= 1 ) {
        if( (pos.numOfBlackMajorPieces() + pos.numOfWhiteMajorPieces()) == 0 ) {
            return 0;
        }
    }

    if( ply >= MaxSearchPly ) {
        return getRelativeEvaluation( pos );
    }

    if( haveHashInQuiesce ) {
        HashTable::Entry * hashEntry = hashTable->probe( pos );

        if( hashEntry != 0 ) {
            // Position found in the hash table
            int value = hashEntry->getValue();

            // If score is mate, we must convert it from relative to absolute (for the current ply)
            if( value < Score::MateLo ) {
                value += ply;
            }
            else if( value > Score::MateHi ) {
                value -= ply;
            }

            if( hashEntry->isUpperBound() ) {
                if( value < gamma ) {
                    return value;
                }
            }
            else {
                if( value >= gamma ) {
                    return value;
                }
            }
        }
    }

    int inCheck = pos.boardFlags & Position::SideToPlayInCheck;

    int result = Score::Min;

    if( ! inCheck ) {
        result = getRelativeEvaluation( pos );

        if( result >= gamma ) {
            return result;
        }
    }

    // Probe recognizers
    if( haveRecognizersInQuiesce ) {
        RecognizerInfo  recognizerInfo;

        if( Recognizer::probe( pos, recognizerInfo ) ) {
            if( recognizerInfo.type() == rtExact ) {
                return recognizerInfo.adjust( ply );
            }
            else if( recognizerInfo.type() == rtLowerBound && recognizerInfo.value() >= gamma ) {
                return recognizerInfo.value();
            }
            else if( recognizerInfo.type() == rtUpperBound && recognizerInfo.value()  < gamma ) {
                return recognizerInfo.value();
            }
        }
    }

    // Generate and check moves
    Move        curr;
    UndoInfo    parent( pos );

    MoveHandler moveHandler( pos, ply, GenerateForQuiesce );

    while( ! moveHandler.getNextMove( curr ) ) {
        if( pos.doMove( curr ) == 0 ) {
            int temp = -negaMaxQuiesceMT( pos, 1-gamma, ply+1, checks_depth-1 );

            if( temp > result ) {
                result = temp;

                // Check if result is good enough to produce a cutoff
                if( result >= gamma ) {
                    pos.undoMove( curr, parent );
                    break;
                }
            }
        }

        pos.undoMove( curr, parent );
    }

    // If there was no cutoff so far, search checks
    if( haveChecksInQuiesce && (result < gamma) && ! inCheck && checks_depth > 0 ) {
        MoveList &  moves = moveHandler.getDiscardedMoves();

        pos.generateNonTactical( moves );

        int king = pos.blackToMove() ? pos.whiteKingSquare : pos.blackKingSquare;

        for( int i=0; i<moves.count(); i++ ) {
            curr = moves.get( i );

            bool possible = curr.getEnPassant() != 0;

            if( ! possible ) {
                unsigned dir1 = Attacks::Direction[ curr.getTo() ][king];
                unsigned dir2 = Attacks::Direction[ curr.getFrom() ][king];

                possible = (dir1 != 0) || (dir2 != 0);

                if( PieceType(pos.board.piece[ curr.getFrom() ]) == Knight ) {
                    if( Attacks::Knight[ curr.getTo() ].getBit( king ) ) {
                        possible = true;
                    }
                }
            }

            if( ! possible ) continue;

            if( pos.doMove( curr ) == 0 ) {
                if( pos.isSideToMoveInCheck() ) {
                    // The move gives check, search it
                    int temp = -negaMaxQuiesceMT( pos, 1-gamma, ply+1, checks_depth-1 );

                    if( temp > result ) {
                        result = temp;

                        // Check if result is good enough to produce a cutoff
                        if( result >= gamma ) {
                            pos.undoMove( curr, parent );
                            break;
                        }
                    }
                }
            }

            pos.undoMove( curr, parent );
        }
    }

    if( inCheck ) {
        // If in check and no moves, it's mate
        if( result == Score::Min ) {
            result = ply - Score::Mate;
        }
    }

    if( result >= gamma ) {
        if( haveHashInQuiesce ) {
            hashTable->store( pos,
                curr,
                result,
                HashTable::Entry::LowerBound,
                0 );
        }
    }

    return result;
}
