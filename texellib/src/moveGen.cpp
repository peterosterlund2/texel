/*
 * moveGen.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "moveGen.hpp"

void
MoveGen::MoveList::filter(const std::vector<Move> & searchMoves)
{
    int used = 0;
    for(int i = 0;i < size;i++)
        if(std::find(searchMoves.begin(), searchMoves.end(), m[i]) != searchMoves.end())
            m[used++] = m[i];
    size = used;
}

MoveGen&
MoveGen::instance()
{
    static MoveGen inst;
    return inst;
}

