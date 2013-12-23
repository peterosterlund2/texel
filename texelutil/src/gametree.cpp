/*
 * gametree.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "gametree.hpp"
#include <stdexcept>

// --------------------------------------------------------------------------------

PgnToken::PgnToken(int type0, const std::string& token0)
    : type(type0), token(token0) {
}

// --------------------------------------------------------------------------------

PgnScanner::PgnScanner(const std::string& pgn) {
    // Skip "escape" lines, ie lines starting with a '%' character
    int len = pgn.length();
    bool col0 = true;
    for (int i = 0; i < len; i++) {
        char c = pgn[i];
        if (c == '%' && col0) {
            while (i + 1 < len) {
                char nextChar = pgn[i + 1];
                if ((nextChar == '\n') || (nextChar == '\r'))
                    break;
                i++;
            }
            col0 = true;
        } else {
            data += c;
            col0 = ((c == '\n') || (c == '\r'));
        }
    }
    data += '\n'; // Terminating whitespace simplifies the tokenizer
    idx = 0;
}

PgnToken PgnScanner::nextToken() {
    if (savedTokens.size() > 0) {
        int len = savedTokens.size();
        PgnToken ret = savedTokens[len - 1];
        savedTokens.pop_back();
        return ret;
    }

    PgnToken ret(PgnToken::END, "");
    try {
        while (true) {
            char c = data.at(idx++);
            if (isspace(c)) {
                // Skip
            } else if (c == '.') {
                ret.type = PgnToken::PERIOD;
                break;
            } else if (c == '*') {
                ret.type = PgnToken::ASTERISK;
                break;
            } else if (c == '[') {
                ret.type = PgnToken::LEFT_BRACKET;
                break;
            } else if (c == ']') {
                ret.type = PgnToken::RIGHT_BRACKET;
                break;
            } else if (c == '(') {
                ret.type = PgnToken::LEFT_PAREN;
                break;
            } else if (c == ')') {
                ret.type = PgnToken::RIGHT_PAREN;
                break;
            } else if (c == '{') {
                ret.type = PgnToken::COMMENT;
                std::string sb;
                while ((c = data.at(idx++)) != '}')
                    sb += c;
                ret.token = sb;
                break;
            } else if (c == ';') {
                ret.type = PgnToken::COMMENT;
                std::string sb;
                while (true) {
                    c = data.at(idx++);
                    if ((c == '\n') || (c == '\r'))
                        break;
                    sb += c;
                }
                ret.token = sb;
                break;
            } else if (c == '"') {
                ret.type = PgnToken::STRING;
                std::string sb;
                while (true) {
                    c = data.at(idx++);
                    if (c == '"') {
                        break;
                    } else if (c == '\\') {
                        c = data.at(idx++);
                    }
                    sb += c;
                }
                ret.token = sb;
                break;
            } else if (c == '$') {
                ret.type = PgnToken::NAG;
                std::string sb;
                while (true) {
                    c = data.at(idx++);
                    if (!isdigit(c)) {
                        idx--;
                        break;
                    }
                    sb += c;
                }
                ret.token = sb;
                break;
            } else { // Start of symbol or integer
                ret.type = PgnToken::SYMBOL;
                std::string sb;
                sb += c;
                bool onlyDigits = isdigit(c);
                const std::string term = ".*[](){;\"$";
                while (true) {
                    c = data.at(idx++);
                    if (isspace(c) || (term.find_first_of(c) != std::string::npos)) {
                        idx--;
                        break;
                    }
                    sb += c;
                    if (!isdigit(c))
                        onlyDigits = false;
                }
                if (onlyDigits)
                    ret.type = PgnToken::INTEGER;
                ret.token = sb;
                break;
            }
        }
    } catch (const std::out_of_range& e) {
        ret.type = PgnToken::END;
    }
    return ret;
}

PgnToken PgnScanner::nextTokenDropComments() {
    while (true) {
        PgnToken tok = nextToken();
        if (tok.type != PgnToken::COMMENT)
            return tok;
    }
}

// --------------------------------------------------------------------------------

Node::Node(const std::shared_ptr<Node>& parent0, const Move& m, const UndoInfo& ui0, int nag0,
           const std::string& preComment0, const std::string& postComment0)
    : move(m), ui(ui0), nag(nag0), preComment(preComment0),
      postComment(postComment0), parent(parent0) {
}

void Node::addChild(Position& pos, std::shared_ptr<Node>& node, std::shared_ptr<Node>& child) {
    child->parent = node;
    node->children.push_back(child);
    node = child;
    child = std::make_shared<Node>();
    UndoInfo ui;
    pos.makeMove(node->getMove(), ui);
}

void Node::parsePgn(PgnScanner& scanner, Position pos, std::shared_ptr<Node> node) {
    std::shared_ptr<Node> nodeToAdd(std::make_shared<Node>());
    bool moveAdded = false;
    while (true) {
        PgnToken tok = scanner.nextToken();
        switch (tok.type) {
        case PgnToken::INTEGER:
        case PgnToken::PERIOD:
            break;
        case PgnToken::LEFT_PAREN:
            if (moveAdded) {
                addChild(pos, node, nodeToAdd);
                moveAdded = false;
            }
            if (node->getParent()) {
                parsePgn(scanner, pos, node->getParent());
            } else {
                int nestLevel = 1;
                while (nestLevel > 0) {
                    switch (scanner.nextToken().type) {
                    case PgnToken::LEFT_PAREN: nestLevel++; break;
                    case PgnToken::RIGHT_PAREN: nestLevel--; break;
                    case PgnToken::END: return; // Broken PGN file. Just give up.
                    }
                }
            }
            break;
        case PgnToken::NAG:
            if (moveAdded) { // NAG must be after move
                if (!str2Num(tok.token, nodeToAdd->nag))
                    nodeToAdd->nag = 0;
            }
            break;
        case PgnToken::SYMBOL: {
            if ((tok.token =="1-0") || (tok.token == "0-1") ||
                (tok.token == "1/2-1/2") || (tok.token == "*")) {
                if (moveAdded)
                    addChild(pos, node, nodeToAdd);
                return;
            }
            char lastChar = tok.token[tok.token.length() - 1];
            if (lastChar == '+')
                tok.token = tok.token.substr(0, tok.token.length() - 1);
            if ((lastChar == '!') || (lastChar == '?')) {
                int movLen = tok.token.length() - 1;
                while (movLen > 0) {
                    char c = tok.token[movLen - 1];
                    if ((c == '!') || (c == '?'))
                        movLen--;
                    else
                        break;
                }
                std::string ann = tok.token.substr(movLen);
                tok.token = tok.token.substr(0, movLen);
                int nag = 0;
                if      (ann == "!")  nag = 1;
                else if (ann == "?")  nag = 2;
                else if (ann == "!!") nag = 3;
                else if (ann == "??") nag = 4;
                else if (ann == "!?") nag = 5;
                else if (ann == "?!") nag = 6;
                if (nag > 0)
                    scanner.putBack(PgnToken(PgnToken::NAG, num2Str(nag)));
            }
            if (tok.token.length() > 0) {
                if (moveAdded) {
                    addChild(pos, node, nodeToAdd);
                    moveAdded = false;
                }
                nodeToAdd->move = TextIO::stringToMove(pos, tok.token);
                if (nodeToAdd->move.isEmpty())
                    throw ChessParseError("Invalid move");
                moveAdded = true;
            }
            break;
        }
        case PgnToken::COMMENT:
            if (moveAdded)
                nodeToAdd->postComment += tok.token;
            else
                nodeToAdd->preComment += tok.token;
            break;
        case PgnToken::ASTERISK:
        case PgnToken::LEFT_BRACKET:
        case PgnToken::RIGHT_BRACKET:
        case PgnToken::STRING:
        case PgnToken::RIGHT_PAREN:
        case PgnToken::END:
            if (moveAdded)
                addChild(pos, node, nodeToAdd);
            return;
        }
    }
}

// --------------------------------------------------------------------------------

GameNode::GameNode(const Position& pos, const std::shared_ptr<Node>& node)
    : rootNode(node), currPos(pos), currNode(node) {
}

const Position& GameNode::getPos() const {
    return currPos;
}

const Move& GameNode::getMove() const {
    return currNode->getMove();
}

std::string GameNode::getComment() const {
    std::string pre = currNode->getPreComment();
    std::string post = currNode->getPostComment();
    if ((pre.length() > 0) && (post.length() > 0))
        pre += " ";
    return pre + post;
}

bool GameNode::goBack() {
    std::shared_ptr<Node> parent = currNode->getParent();
    if (!parent)
        return false;
    currPos.unMakeMove(currNode->getMove(), currNode->getUndoInfo());
    currNode = parent;
    return true;
}

int GameNode::nChildren() const {
    return currNode->getChildren().size();
}

void GameNode::goForward(int i) {
    std::shared_ptr<Node> next = currNode->getChildren()[i];
    UndoInfo ui;
    currPos.makeMove(next->getMove(), ui);
    currNode = next;
}

// --------------------------------------------------------------------------------

GameTree::GameTree() {
    setStartPos(TextIO::readFEN(TextIO::startPosFEN));
}

bool GameTree::readPGN(const std::string& pgn) {
    PgnScanner scanner(pgn);
    PgnToken tok = scanner.nextToken();

    // Parse tag section
    std::vector<TagPair> tPairs;
    while (tok.type == PgnToken::LEFT_BRACKET) {
        TagPair tp;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::SYMBOL)
            break;
        tp.tagName = tok.token;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::STRING)
            break;
        tp.tagValue = tok.token;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::RIGHT_BRACKET) {
            // In a well-formed PGN, there is nothing between the string
            // and the right bracket, but broken headers with non-escaped
            // " characters sometimes occur. Try to do something useful
            // for such headers here.
            PgnToken prevTok(PgnToken::STRING, "");
            while ((tok.type == PgnToken::STRING) || (tok.type == PgnToken::SYMBOL)) {
                if (tok.type != prevTok.type)
                    tp.tagValue += '"';
                if ((tok.type == PgnToken::SYMBOL) && (prevTok.type == PgnToken::SYMBOL))
                    tp.tagValue += ' ';
                tp.tagValue += tok.token;
                prevTok = tok;
                tok = scanner.nextTokenDropComments();
            }
        }
        tPairs.push_back(tp);
        tok = scanner.nextToken();
    }
    scanner.putBack(tok);

    std::string fen = TextIO::startPosFEN;
    int nTags = tPairs.size();
    for (int i = 0; i < nTags; i++)
        if (tPairs[i].tagName == "FEN")
            fen = tPairs[i].tagValue;
    setStartPos(TextIO::readFEN(fen));

    // Parse move section
    std::shared_ptr<Node> gameRoot(std::make_shared<Node>());
    Node::parsePgn(scanner, startPos, gameRoot);

    if ((tPairs.size() == 0) && (gameRoot->getChildren().size() == 0))
        return false;

    // Store parsed data in GameTree

    for (int i = 0; i < nTags; i++) {
        const std::string& name = tPairs[i].tagName;
        const std::string& val = tPairs[i].tagValue;
        if ((name == "FEN") || (name == "Setup")) {
            // Already handled
        } else if (name == "Event") {
            event = val;
        } else if (name == "Site") {
            site = val;
        } else if (name == "Date") {
            date = val;
        } else if (name == "Round") {
            round = val;
        } else if (name == "White") {
            white = val;
        } else if (name == "Black") {
            black = val;
        } else if (name == "Result") {
            result = val;
        } else {
            tagPairs.push_back(tPairs[i]);
        }
    }

    rootNode = gameRoot;

    return true;
}

void GameTree::getHeaders(std::map<std::string, std::string>& headers) {
    headers.insert(std::make_pair("Event", event));
    headers.insert(std::make_pair("Site",  site));
    headers.insert(std::make_pair("Date",  date));
    headers.insert(std::make_pair("Round", round));
    headers.insert(std::make_pair("White", white));
    headers.insert(std::make_pair("Black", black));
    for (size_t i = 0; i < tagPairs.size(); i++) {
        TagPair tp = tagPairs[i];
        headers.insert(std::make_pair(tp.tagName, tp.tagValue));
    }
}

GameNode GameTree::getRootNode() const {
    return GameNode(startPos, rootNode);
}

void GameTree::setStartPos(const Position& pos) {
    event = "?";
    site = "?";
    date = "?";
    round = "?";
    white = "?";
    black = "?";
    result = "?";
    startPos = pos;
    tagPairs.clear();
    rootNode = std::make_shared<Node>();
}
