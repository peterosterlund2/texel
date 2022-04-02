#!/usr/bin/python3

import chess
import chess.svg

def gen_svg(fen, file, opts={}):
    all_opts = { 'size': 200 }
    all_opts.update(opts)
    with open("svg/" + file, "w") as f:
        f.write(chess.svg.board(chess.Board(fen), **all_opts))

gen_svg("rnbqk1nr/ppppp1pp/8/8/8/2P5/PP1PPNPP/RNBQKBNR w KQ - 0 1", "castle_flags.svg",
        { 'fill': {chess.F7: "#00ff00ff" } })

gen_svg("rnbqkbnr/1ppppppp/8/8/2B5/8/1PPPPPPP/RNBQKBNR w KQ - 0 1", "trapped_bishop_1.svg")
gen_svg("Bnbqkbnr/1ppppppp/8/1B3r2/8/8/2PPPPPP/RNBQKBNR w KQ - 0 1", "trapped_bishop_2.svg")

gen_svg("rnbqkbnr/1ppppppp/8/8/8/1P2P3/PpPP1PPP/RNBQK1NR w KQkq - 0 1", "ext_kernel_example_illegal.svg")
gen_svg("rnbqkbnr/1ppppppp/8/8/1P6/4P3/PpPP1PPP/RNBQK1NR w KQkq - 0 1", "ext_kernel_example_legal.svg")

gen_svg("rnbqkbnr/ppp2ppp/8/3P4/3p4/8/PPP2PPP/RNBQKBNR w KQkq - 0 1", "proofkernel_example.svg")
gen_svg("rnbqkbnr/pppB1ppp/4p3/1Q6/4P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 1", "revmoves_example.svg")

gen_svg("1r4R1/4rp2/Pk2B1Qp/B2n3r/qb1r2Q1/2Q1NKbn/2BR1R1N/2R2Q1n b - - 0 1", "no_revmoves_1.svg")
gen_svg("rnbqkbnr/ppp1pppp/2Bp4/1Q6/4P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1", "no_revmoves_2.svg")
gen_svg("5r2/P1r1R1nR/kb2Bb2/1n1qpnQq/b1BB1B2/KN2P1np/n2N4/4N3 w - - 0 1", "no_last_move_1.svg")

gen_svg("4k3/8/8/8/8/8/N4N2/4K3 w - - 0 1", "heuristic_1_start.svg")
gen_svg("2N1k3/8/8/8/8/2N5/8/4K3 w - - 0 1", "heuristic_1_goal.svg")
gen_svg("4k3/8/8/p2p4/P2P4/K7/8/8 w - - 0 1", "heuristic_2_start.svg",
        {'squares': [chess.B4,chess.C4,chess.E4]})
gen_svg("4k3/8/K7/p2p4/P2P4/8/8/8 w - - 0 1", "heuristic_2_goal.svg",
        {'squares': [chess.B4,chess.C4,chess.E4]})

cone1 = [chess.F5,
         chess.E4,chess.F4,chess.G4,
         chess.D3,chess.E3,chess.F3,chess.G3,chess.H3,
         chess.C2,chess.D2,chess.E2,chess.F2,chess.G2,chess.H2]
gen_svg("8/8/8/5P2/8/8/8/8 w - - 0 1", "pawn_cone_1.svg", {'squares': cone1})
cone2 = [chess.F5,
         chess.E4,chess.F4,chess.G4,
         chess.E3,chess.F3,chess.G3,
         chess.E2,chess.F2,chess.G2]
gen_svg("8/8/8/5P2/8/8/8/8 w - - 0 1", "pawn_cone_2.svg", {'squares': cone2})

gen_svg("3k2br/5p1P/8/8/8/7P/8/4K3 w - - 0 1", "deadlock_start.svg")
gen_svg("3k2br/5p1P/8/8/8/7N/8/4K3 w - - 0 1", "deadlock_goal.svg")

gen_svg("R1B2kbR/3bpp2/Q7/1N2nb2/2pb3q/1np1r1bB/1b3K2/Qr4RQ w - - 0 1", "only_irreversible.svg")

gen_svg("bn1n2n1/8/kr1K1bbB/2rQR3/r1pb2B1/NN2r3/qq6/1B2nRB1 w - - 0 1", "non_admissible_goal.svg")
gen_svg("bn1n2n1/B7/kr1K1bbB/2rQR3/2pb2B1/NN2r3/qq6/rB2nR2 w - - 0 1", "non_admissible_best.svg")
