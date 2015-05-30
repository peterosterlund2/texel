/*
 * bookBuildTest.cpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#include "bookBuildTest.hpp"
#include "../bookbuild.hpp"
#include "textio.hpp"

#include "cute.h"

using namespace BookBuild;

void
BookBuildTest::testBookNode() {
    {
        BookNode bn(1234);
        ASSERT_EQUAL(INT_MAX, bn.getDepth());
    }

    auto bn(std::make_shared<BookNode>(12345678, true));
    ASSERT_EQUAL(12345678, bn->getHashKey());
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(BookNode::EMPTY, bn->getState());
    ASSERT_EQUAL(0, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());

    bn->setState(BookNode::INITIALIZED);
    ASSERT_EQUAL(BookNode::INITIALIZED, bn->getState());

    Move e4(TextIO::uciStringToMove("e2e4"));
    Move d4(TextIO::uciStringToMove("d2d4"));
    bn->setSearchResult(d4, 17, 4711);
    ASSERT_EQUAL(d4, bn->getBestNonBookMove());
    ASSERT_EQUAL(17, bn->getSearchScore());
    ASSERT_EQUAL(4711, bn->getSearchTime());

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreW());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreB());

    {
        BookNode::BookSerializeData bsd;
        bn->serialize(bsd);
        BookNode bn2(0);
        bn2.deSerialize(bsd);
        ASSERT_EQUAL(12345678, bn2.getHashKey());
        ASSERT_EQUAL(0, bn2.getChildren().size());
        ASSERT_EQUAL(0, bn2.getParents().size());
        ASSERT_EQUAL(BookNode::DESERIALIZED, bn2.getState());
        ASSERT_EQUAL(d4, bn2.getBestNonBookMove());
        ASSERT_EQUAL(17, bn2.getSearchScore());
        ASSERT_EQUAL(4711, bn2.getSearchTime());
    }

    auto child(std::make_shared<BookNode>(1234, false));
    U16 e4c = e4.getCompressedMove();
    bn->addChild(e4c, child);
    child->addParent(e4c, bn);

    ASSERT_EQUAL(1, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());
    ASSERT_EQUAL(0, child->getChildren().size());
    ASSERT_EQUAL(1, child->getParents().size());
    ASSERT_EQUAL(child, bn->getChildren().find(e4c)->second);
    ASSERT_EQUAL(bn, child->getParents().lower_bound(e4c)->second.lock());
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(1, child->getDepth());

    Move e5(TextIO::uciStringToMove("e7e5"));
    Move c5(TextIO::uciStringToMove("c7c5"));
    child->setSearchResult(c5, -20, 10000);
    ASSERT_EQUAL(20, bn->getNegaMaxScore());
    ASSERT_EQUAL(22, bn->getNegaMaxBookScoreW());
    ASSERT_EQUAL(20, bn->getNegaMaxBookScoreB());

    child->setSearchResult(c5, -16, 10000);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(18, bn->getNegaMaxBookScoreW());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreB());

    auto child2(std::make_shared<BookNode>(1235, false));
    U16 e5c = e5.getCompressedMove();
    child->addChild(e5c, child2);
    child2->addParent(e5c, child);

    ASSERT_EQUAL(1, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());
    ASSERT_EQUAL(1, child->getChildren().size());
    ASSERT_EQUAL(1, child->getParents().size());
    ASSERT_EQUAL(child, bn->getChildren().find(e4c)->second);
    ASSERT_EQUAL(bn, child->getParents().lower_bound(e4c)->second.lock());
    ASSERT_EQUAL(0, child2->getChildren().size());
    ASSERT_EQUAL(1, child2->getParents().size());
    ASSERT_EQUAL(child2, child->getChildren().find(e5c)->second);
    ASSERT_EQUAL(child, child2->getParents().lower_bound(e5c)->second.lock());
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(1, child->getDepth());
    ASSERT_EQUAL(2, child2->getDepth());

    Move nf3(TextIO::uciStringToMove("g1f3"));
    child2->setSearchResult(nf3, 17, 10000);
    ASSERT_EQUAL(17, child2->getNegaMaxScore());
    ASSERT_EQUAL(19, child2->bookScoreW());
    ASSERT_EQUAL(15, child2->bookScoreB());
    ASSERT_EQUAL(19, child2->getNegaMaxBookScoreW());
    ASSERT_EQUAL(15, child2->getNegaMaxBookScoreB());

    ASSERT_EQUAL(-16, child->getNegaMaxScore());
    ASSERT_EQUAL(-18, child->bookScoreW());
    ASSERT_EQUAL(-16, child->bookScoreB());
    ASSERT_EQUAL(-18, child->getNegaMaxBookScoreW());
    ASSERT_EQUAL(-15, child->getNegaMaxBookScoreB());

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(17, bn->bookScoreW());
    ASSERT_EQUAL(17, bn->bookScoreB());
    ASSERT_EQUAL(18, bn->getNegaMaxBookScoreW());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreB());

    child2->setSearchResult(nf3, 10, 10000);
    ASSERT_EQUAL(10, child2->getNegaMaxScore());
    ASSERT_EQUAL(12, child2->bookScoreW());
    ASSERT_EQUAL(8, child2->bookScoreB());
    ASSERT_EQUAL(12, child2->getNegaMaxBookScoreW());
    ASSERT_EQUAL(8, child2->getNegaMaxBookScoreB());

    ASSERT_EQUAL(-10, child->getNegaMaxScore());
    ASSERT_EQUAL(-18, child->bookScoreW());
    ASSERT_EQUAL(-16, child->bookScoreB());
    ASSERT_EQUAL(-12, child->getNegaMaxBookScoreW());
    ASSERT_EQUAL(-8, child->getNegaMaxBookScoreB());

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(17, bn->bookScoreW());
    ASSERT_EQUAL(17, bn->bookScoreB());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreW());
    ASSERT_EQUAL(17, bn->getNegaMaxBookScoreB());
}

void
BookBuildTest::testShortestDepth() {
    auto n1(std::make_shared<BookNode>(1, true));
    auto n2(std::make_shared<BookNode>(2, false));
    auto n3(std::make_shared<BookNode>(3, false));
    auto n4(std::make_shared<BookNode>(4, false));
    Move m(0, 0, Piece::EMPTY);
    U16 mc = m.getCompressedMove();

    n1->addChild(mc, n2);
    n2->addParent(mc, n1);

    n2->addChild(mc, n3);
    n3->addParent(mc, n2);

    n3->addChild(mc, n4);
    n4->addParent(mc, n3);

    ASSERT_EQUAL(0, n1->getDepth());
    ASSERT_EQUAL(1, n2->getDepth());
    ASSERT_EQUAL(2, n3->getDepth());
    ASSERT_EQUAL(3, n4->getDepth());

    Move m2(1, 1, Piece::EMPTY);
    U16 m2c = m2.getCompressedMove();
    n1->addChild(m2c, n4);
    n4->addParent(m2c, n1);

    ASSERT_EQUAL(0, n1->getDepth());
    ASSERT_EQUAL(1, n2->getDepth());
    ASSERT_EQUAL(2, n3->getDepth());
    ASSERT_EQUAL(1, n4->getDepth());
}

void
BookBuildTest::testBookNodeDAG() {
    auto n1(std::make_shared<BookNode>(1, true));
    auto n2(std::make_shared<BookNode>(2, false));
    auto n3(std::make_shared<BookNode>(3, false));
    auto n4(std::make_shared<BookNode>(4, false));
    auto n5(std::make_shared<BookNode>(5, false));
    auto n6(std::make_shared<BookNode>(6, false));

    U16 m = TextIO::uciStringToMove("e2e4").getCompressedMove();
    n1->addChild(m, n2);
    n2->addParent(m, n1);

    m = TextIO::uciStringToMove("g8f6").getCompressedMove();
    n2->addChild(m, n3);
    n3->addParent(m, n2);

    m = TextIO::uciStringToMove("d2d4").getCompressedMove();
    n3->addChild(m, n4);
    n4->addParent(m, n3);

    m = TextIO::uciStringToMove("d2d4").getCompressedMove();
    n1->addChild(m, n5);
    n5->addParent(m, n1);

    m = TextIO::uciStringToMove("g8f6").getCompressedMove();
    n5->addChild(m, n6);
    n6->addParent(m, n5);

    m = TextIO::uciStringToMove("e2e4").getCompressedMove();
    n6->addChild(m, n4);
    n4->addParent(m, n6);

    Move nm(0, 0, Piece::EMPTY);
    n1->setSearchResult(nm, 10, 10000);
    n2->setSearchResult(nm, -8, 10000);
    n3->setSearchResult(nm, 7, 10000);
    n4->setSearchResult(nm, -12, 10000);
    n5->setSearchResult(nm, -12, 10000);
    n6->setSearchResult(nm, 11, 10000);

    ASSERT_EQUAL(-12, n4->getNegaMaxScore());
    ASSERT_EQUAL(12, n3->getNegaMaxScore());
    ASSERT_EQUAL(-8, n2->getNegaMaxScore());
    ASSERT_EQUAL(12, n6->getNegaMaxScore());
    ASSERT_EQUAL(-12, n5->getNegaMaxScore());
    ASSERT_EQUAL(12, n1->getNegaMaxScore());

    n4->setSearchResult(nm, -6, 10000);
    ASSERT_EQUAL(-6, n4->getNegaMaxScore());
    ASSERT_EQUAL(7, n3->getNegaMaxScore());
    ASSERT_EQUAL(-7, n2->getNegaMaxScore());
    ASSERT_EQUAL(11, n6->getNegaMaxScore());
    ASSERT_EQUAL(-11, n5->getNegaMaxScore());
    ASSERT_EQUAL(11, n1->getNegaMaxScore());

    n1->setSearchResult(nm, 13, 10000);
    ASSERT_EQUAL(-6, n4->getNegaMaxScore());
    ASSERT_EQUAL(7, n3->getNegaMaxScore());
    ASSERT_EQUAL(-7, n2->getNegaMaxScore());
    ASSERT_EQUAL(11, n6->getNegaMaxScore());
    ASSERT_EQUAL(-11, n5->getNegaMaxScore());
    ASSERT_EQUAL(13, n1->getNegaMaxScore());
}

cute::suite
BookBuildTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testBookNode));
    s.push_back(CUTE(testShortestDepth));
    s.push_back(CUTE(testBookNodeDAG));
    return s;
}
