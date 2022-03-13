#!/usr/bin/python3
# -*- encoding: utf-8 -*-

# This script verifies the proof games computed by "texelutil proofgame -f"
# using python-chess, which can be installed by running:
#
#     pip install python-chess
#
# To validate a results file, run:
#
#     cat results_file | ./validate.py
#

import chess
import fileinput

numLegal = 0
numBad = 0
for line in fileinput.input():
    line = line.strip()
    words = line.split()
    fen = " ".join(words[0:4])  # Ignore move number and half-move clock
    legal = False
    board = None
    for w in words:
        if w == "legal:":
            legal = True
        elif w == "proof:":
            board = chess.Board()
        elif board != None:
            try:
                board.push_san(w)
            except ValueError:
                print("Illegal move: {}".format(w))
                board.clear()
                break
    if legal:
        fen2 = " ".join(board.fen().split()[0:4]) if board != None else None
        if fen == fen2:
            numLegal += 1
        else:
            numBad += 1
            print("Incorrect proof game: {}".format(line))
        print("Legal: {} Bad: {}".format(numLegal, numBad))
