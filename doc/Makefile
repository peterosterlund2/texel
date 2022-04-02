all	: pdf

SVGS = castle_flags \
       trapped_bishop_1 trapped_bishop_2 \
       ext_kernel_example_illegal ext_kernel_example_legal \
       proofkernel_example revmoves_example \
       no_revmoves_1 no_revmoves_2 no_last_move_1 \
       heuristic_1_start heuristic_1_goal \
       heuristic_2_start heuristic_2_goal \
       pawn_cone_1 pawn_cone_2 deadlock_start deadlock_goal \
       only_irreversible non_admissible_goal non_admissible_best

SVG_FILES = $(patsubst %,svg/%.svg,$(SVGS))

svgs	: $(SVG_FILES)
pdf	: svgs
pdf	: proofgame.pdf

proofgame.pdf : proofgame.md $(SVG_FILES) Makefile
	pandoc -f gfm -t context -N $< -o $@

svg	:
	mkdir -p svg

$(subst .,%,$(SVG_FILES))	: gensvg.py svg
	./gensvg.py

clean	:
	rm -rf svg proofgame.pdf

.PHONY : clean
