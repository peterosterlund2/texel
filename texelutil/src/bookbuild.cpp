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
BookNode::initScores() {
    if (negaMaxScore != INVALID_SCORE)
        return;
    for (auto& e : children)
        e.second->initScores();

    negaMaxScore = searchScore;
    negaMaxBookScoreW = bookScoreW();
    negaMaxBookScoreB = bookScoreB();
    for (const auto& e : children) {
        negaMaxScore = std::max(negaMaxScore, -e.second->negaMaxScore);
        negaMaxBookScoreW = std::max(negaMaxBookScoreW, -e.second->negaMaxBookScoreW);
        negaMaxBookScoreB = std::max(negaMaxBookScoreB, -e.second->negaMaxBookScoreB);
    }
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
        auto bn(std::make_shared<BookNode>());
        bn->deSerialize(bsd);
        bookNodes[bn->getHashKey()] = bn;
    }
    is.close();

    // Find positions for all book entries by exploring moves from the starting position
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    initPositions(pos);

    // Initialize all negamax scores
    const auto it = bookNodes.find(pos.bookHash());
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
Book::initPositions(Position& pos) {
    const U64 hash = pos.bookHash();
    const auto it = bookNodes.find(hash);
    if (it == bookNodes.end())
        return;
    auto node = it->second;

    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        pos.makeMove(moves[i], ui);

        U64 childHash = pos.bookHash();
        hashToParent.insert(std::make_pair(childHash, node));
        auto it2 = bookNodes.find(childHash);
        if (it2 != bookNodes.end()) {
            auto child = it2->second;
            node->addChild(moves[i].getCompressedMove(), child);
            child->addParent(moves[i].getCompressedMove(), node);
            if (child->getState() == BookNode::DESERIALIZED)
                initPositions(pos);
        }

        pos.unMakeMove(moves[i], ui);
    }
    node->setState(BookNode::INITIALIZED);
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
