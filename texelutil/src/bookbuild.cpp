/*
 * bookbuild.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: petero
 */

#include "bookbuild.hpp"
#include "moveGen.hpp"
#include "textio.hpp"

namespace BookBuild {

void
BookNode::initScores(bool force) {
    if (force || (negaMaxScore == INVALID_SCORE)) {
        for (auto& e : children)
            e.second->initScores();
        computeNegaMax();
    }
}

bool
BookNode::computeNegaMax() {
    int oldNM = negaMaxScore;
    int oldBW = negaMaxBookScoreW;
    int oldBB = negaMaxBookScoreB;

    negaMaxScore = searchScore;
    negaMaxBookScoreW = bookScoreW();
    negaMaxBookScoreB = bookScoreB();
    for (const auto& e : children) {
        negaMaxScore = std::max(negaMaxScore, -e.second->negaMaxScore);
        negaMaxBookScoreW = std::max(negaMaxBookScoreW, -e.second->negaMaxBookScoreW);
        negaMaxBookScoreB = std::max(negaMaxBookScoreB, -e.second->negaMaxBookScoreB);
    }
    return ((negaMaxScore != oldNM) ||
            (negaMaxBookScoreW != oldBW) ||
            (negaMaxBookScoreB != oldBB));
}

void
BookNode::propagateScore() {
    bool propagate = computeNegaMax();
    if (propagate) {
        for (auto& e : parents) {
            std::shared_ptr<BookNode> parent = e.second.lock();
            assert(parent);
            parent->propagateScore();
        }
    }
}

void
BookNode::setSearchResult(const Move& bestMove, int score, int time) {
    bool propagate = score != searchScore;
    bestNonBookMove = bestMove;
    searchScore = score;
    searchTime = time;
    if (propagate)
        propagateScore();
    else
        computeNegaMax();
}

void
BookNode::updateDepth() {
    bool updated = false;
    for (auto& e : parents) {
        std::shared_ptr<BookNode> parent = e.second.lock();
        assert(parent);
        if (depth != INT_MAX)
            assert((depth - parent->depth) % 2 != 0);
        if (depth > parent->depth + 1) {
            depth = parent->depth + 1;
            updated = true;
        }
    }
    if (updated)
        for (auto& e : children)
            e.second->updateDepth();
}

Book::Book()
    : startPosHash(TextIO::readFEN(TextIO::startPosFEN).bookHash()) {
}

void
Book::readFromFile(const std::string& filename) {
    bookNodes.clear();
    hashToParent.clear();

    // Read all book entries
    std::ifstream is;
    is.open(filename.c_str(), std::ios_base::in |
                              std::ios_base::binary);
    is.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    while (true) {
        BookNode::BookSerializeData bsd;
        is.read((char*)&bsd.data[0], sizeof(bsd.data));
        if (!is)
            break;
        auto bn(std::make_shared<BookNode>(0));
        bn->deSerialize(bsd);
        bookNodes[bn->getHashKey()] = bn;
    }
    is.close();

    // Find positions for all book entries by exploring moves from the starting position
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    initPositions(pos);

    // Initialize all negamax scores
    const auto& it = bookNodes.find(startPosHash);
    if (it != bookNodes.end())
        it->second->initScores();
}

void
Book::writeToFile(const std::string& filename) {
    std::ofstream os;
    os.open(filename.c_str(), std::ios_base::out |
                              std::ios_base::binary |
                              std::ios_base::trunc);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    for (const auto& e : bookNodes) {
        auto& node = e.second;
        BookNode::BookSerializeData bsd;
        node->serialize(bsd);
        os.write((const char*)&bsd.data[0], sizeof(bsd.data));
    }
    os.close();
}

void
Book::extend(Position& pos, const Move& move, std::vector<U64>& toSearch) {
    auto it = bookNodes.find(pos.bookHash());
    assert(it != bookNodes.end());
    UndoInfo ui;
    pos.makeMove(move, ui);
    U64 childHash = pos.bookHash();
    assert(bookNodes.find(childHash) == bookNodes.end());
    auto childNode = std::make_shared<BookNode>(childHash);

    setChildRefs(pos);
    childNode->initScores();

    toSearch.push_back(pos.bookHash());
    bookNodes[childHash] = childNode;

    auto b = hashToParent.lower_bound(std::make_pair(childHash, (U64)0));
    auto e = hashToParent.lower_bound(std::make_pair(childHash+1, (U64)0));
    int nParents = 0;
    for (auto p = b; p != e; ++p) {
        assert(p->first == childHash);
        U64 parentHash = p->second;
        auto it2 = bookNodes.find(parentHash);
        assert(it2 != bookNodes.end());
        nParents++;
        std::shared_ptr<BookNode> parent = it2->second;

        Position pos2;
        std::vector<Move> dummyMoves;
        getPosition(parentHash, pos2, dummyMoves);

        MoveList moves;
        MoveGen::pseudoLegalMoves(pos2, moves);
        MoveGen::removeIllegal(pos2, moves);
        Move move;
        bool found = false;
        for (int i = 0; i < moves.size; i++) {
            UndoInfo ui2;
            pos2.makeMove(moves[i], ui2);
            if (pos2.bookHash() == childHash) {
                move = moves[i];
                found = true;
                break;
            }
            pos2.unMakeMove(moves[i], ui2);
        }
        assert(found);

        childNode->addParent(move.getCompressedMove(), parent);
        parent->addChild(move.getCompressedMove(), childNode);
        parent->initScores(true);
        toSearch.push_back(parent->getHashKey());
    }
    assert(nParents > 0);

    pos.unMakeMove(move, ui);
    childNode->setState(BookNode::State::INITIALIZED);
}

bool
Book::getPosition(U64 hashKey, Position& pos, std::vector<Move>& moveList) const {
    if (hashKey == startPosHash) {
        pos = TextIO::readFEN(TextIO::startPosFEN);
        return true;
    }

    auto it = bookNodes.find(pos.bookHash());
    if (it == bookNodes.end())
        return false;
    const auto& parents = it->second->getParents();
    assert(!parents.empty());
    std::shared_ptr<BookNode> parent = parents.begin()->second.lock();
    assert(parent);
    bool ok = getPosition(parent->getHashKey(), pos, moveList);
    assert(ok);

    Move m;
    m.setFromCompressed(parents.begin()->first);
    UndoInfo ui;
    pos.makeMove(m, ui);
    moveList.push_back(m);
    return true;
}

void
Book::initPositions(Position& pos) {
    const U64 hash = pos.bookHash();
    const auto it = bookNodes.find(hash);
    if (it == bookNodes.end())
        return;
    std::shared_ptr<BookNode>& node = it->second;

    setChildRefs(pos);
    for (auto& e : node->getChildren()) {
        if (e.second->getState() == BookNode::DESERIALIZED) {
            UndoInfo ui;
            Move m;
            m.setFromCompressed(e.first);
            pos.makeMove(m, ui);
            initPositions(pos);
            pos.unMakeMove(m, ui);
        }
    }
    node->setState(BookNode::INITIALIZED);
}

void
Book::setChildRefs(Position& pos) {
    const U64 hash = pos.bookHash();
    const auto it = bookNodes.find(hash);
    assert(it != bookNodes.end());
    std::shared_ptr<BookNode>& node = it->second;

    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        pos.makeMove(moves[i], ui);
        U64 childHash = pos.bookHash();
        hashToParent.insert(std::make_pair(childHash, hash));
        auto it2 = bookNodes.find(childHash);
        if (it2 != bookNodes.end()) {
            auto child = it2->second;
            node->addChild(moves[i].getCompressedMove(), child);
            child->addParent(moves[i].getCompressedMove(), node);
        }
        pos.unMakeMove(moves[i], ui);
    }
}

void
Book::makeConsistent() {
#if 0
    while (true) {
        // Find node with maximum fullMoveCounter
        node = findInconsistenNode();
        if (!node)
            break;

        // Update node->children and node->parents

        auto requiredTime = ...;
        requiredTime = std::max(requiredTime, minTime);

        if ((node->bestNonBookMove in children) ||
            (node->searchTime < requiredTime / 2)) {
            node->search(requiredTime); // Updates node->searchScore and node->searchTime
        }

        childNegaMax = max(i=0,nChild, -child.negaMaxScore);

        if (node->searchScore > childNegaMax) {
            // Add child to book
            // Update child->parents and node->children
            continue;
        }

        node->negaMaxScore = childNegaMax;

        node->negaMaxBookScoreW = ...;
        node->negaMaxBookScoreB = ...;
    }
#endif
}

void
Book::extend(bool white) {



}


} // Namespace BookBuild
