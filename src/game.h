#ifndef GAME_H_
#define GAME_H_

enum {
    MaxMovesPerGame = 300   // The theoretical limit should be 3150 or so...
};

struct GameItem
{
    Position    pos;
    Move        moveToPos;
};

struct Game
{
    int initialMoveNumber;
    GameItem 
};

#endif // GAME_H_
