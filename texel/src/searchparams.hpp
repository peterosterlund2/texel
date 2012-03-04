/*
 * searchparams.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef SEARCHPARAMS_HPP_
#define SEARCHPARAMS_HPP_

#if 0
/**
 * Store search parameters (times, increments, max depth, etc).
 * @author petero
 */
public class SearchParams {
    List<Move> searchMoves;  // If non-empty, search only these moves
    int wTime;               // White remaining time, ms
    int bTime;               // Black remaining time, ms
    int wInc;                // White increment per move, ms
    int bInc;                // Black increment per move, ms
    int movesToGo;           // Moves to next time control
    int depth;               // If >0, don't search deeper than this
    int nodes;               // If >0, don't search more nodes than this
    int mate;                // If >0, search for mate-in-x
    int moveTime;            // If >0, search for exactly this amount of time, ms
    boolean infinite;

    public SearchParams() {
        searchMoves = new ArrayList<Move>();
    }
}
#endif


#endif /* SEARCHPARAMS_HPP_ */
