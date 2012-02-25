/*
 * humanPlayer.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef HUMANPLAYER_HPP_
#define HUMANPLAYER_HPP_

/**
 * A player that reads input from the keyboard.
 */
#if 0
public class HumanPlayer implements Player {
    private String lastCmd = "";
    private BufferedReader in;

    public HumanPlayer() {
        in = new BufferedReader(new InputStreamReader(System.in));
    }

    @Override
    public String getCommand(Position pos, boolean drawOffer, List<Position> history) {
        try {
            String color = pos.whiteMove ? "white" : "black";
            System.out.print(String.format("Enter move (%s):", color));
            String moveStr = in.readLine();
            if (moveStr == null)
                return "quit";
            if (moveStr.length() == 0) {
                return lastCmd;
            } else {
                lastCmd = moveStr;
            }
            return moveStr;
        } catch (IOException ex) {
            return "quit";
        }
    }
    
    @Override
    public boolean isHumanPlayer() {
        return true;
    }
    
    @Override
    public void useBook(boolean bookOn) {
    }

    @Override
    public void timeLimit(int minTimeLimit, int maxTimeLimit, boolean randomMode) {
    }

    @Override
    public void clearTT() {
    }
};
#endif



#endif /* HUMANPLAYER_HPP_ */
