/*
 * gametree.hpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#ifndef GAMETREE_HPP_
#define GAMETREE_HPP_

#include "textio.hpp"
#include "util/util.hpp"

#include <map>
#include <memory>

/** A token in a PGN data stream. Used by the PGN parser. */
class PgnToken {
public:
    PgnToken(int type, const std::string& token);

    // These are tokens according to the PGN spec
    static const int STRING = 0;
    static const int INTEGER = 1;
    static const int PERIOD = 2;
    static const int ASTERISK = 3;
    static const int LEFT_BRACKET = 4;
    static const int RIGHT_BRACKET = 5;
    static const int LEFT_PAREN = 6;
    static const int RIGHT_PAREN = 7;
    static const int NAG = 8;
    static const int SYMBOL = 9;

    // These are not tokens according to the PGN spec, but the parser
    // extracts these anyway for convenience.
    static const int COMMENT = 10;
    static const int END = 11;

    // Actual token data
    int type;
    std::string token;
};


class PgnScanner {
public:
    PgnScanner(const std::string& pgn);

    void putBack(const PgnToken& tok) {
        savedTokens.push_back(tok);
    }

    PgnToken nextToken();

    PgnToken nextTokenDropComments();

private:
    std::string data;
    int idx;
    std::vector<PgnToken> savedTokens;
};


/**
 *  A node object represents a position in the game tree.
 *  The position is defined by the move that leads to the position from the parent position.
 *  The root node is special in that it doesn't have a move.
 */
class Node {
public:
    Node() : nag(0) { }
    Node(const std::shared_ptr<Node>& parent, const Move& m, const UndoInfo& ui, int nag,
         const std::string& preComment, const std::string& postComment);

    std::shared_ptr<Node> getParent() const { return parent.lock(); }
    const std::vector<std::shared_ptr<Node>>& getChildren() const { return children; }
    const Move& getMove() const { return move; }
    const UndoInfo& getUndoInfo() const { return ui; }
    const std::string& getPreComment() const { return preComment; }
    const std::string& getPostComment() const { return postComment; }

    static void parsePgn(PgnScanner& scanner, Position pos, std::shared_ptr<Node> node);

private:
    static void addChild(Position& pos, std::shared_ptr<Node>& node,
                         std::shared_ptr<Node>& child);

    Move move;                  // Move leading to this node. Empty in root node.
    UndoInfo ui;
    int nag;                    // Numeric annotation glyph
    std::string preComment;     // Comment before move
    std::string postComment;    // Comment after move

    std::weak_ptr<Node> parent; // Null if root node
    std::vector<std::shared_ptr<Node>> children;
};


class GameNode {
public:
    GameNode(const Position& pos, const std::shared_ptr<Node>& node);

    /** Get current position. */
    const Position& getPos() const;

    /** Get the i:th mvoe in this position. */
    const Move& getMove() const;

    /** Get preComment + postComment for move i. */
    std::string getComment() const;


    /** Go to parent position, unless already at root.
     * @return True if there was a parent position. */
    bool goBack();

    /** Get number of moves in this position. */
    int nChildren() const;

    /** Go to i:th child position. */
    void goForward(int i);

private:
    std::shared_ptr<Node> rootNode; // To prevent tree from being deleted too early
    Position currPos;
    std::shared_ptr<Node> currNode;
};


class GameTree {
public:
    /** Creates an empty GameTree starting at the standard start position. */
    GameTree();

    /** Import PGN data. */
    bool readPGN(const std::string& pgn);

    /** Get PGN header tags and values. */
    void getHeaders(std::map<std::string, std::string>& headers);

    /** Get node corresponding to start position. */
    GameNode getRootNode() const;

private:
    /** Set start position. Drops the whole game tree. */
    void setStartPos(const Position& pos);

    // Data from the seven tag roster (STR) part of the PGN standard
    std::string event, site, date, round, white, black, result;

    // Non-standard tags
    struct TagPair {
        std::string tagName;
        std::string tagValue;
    };
    std::vector<TagPair> tagPairs;

    Position startPos;
    std::shared_ptr<Node> rootNode;
};

#endif /* GAMETREE_HPP_ */
