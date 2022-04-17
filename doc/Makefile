all	: pdf

TBPROBE_SVGS = tb_off_by_one tb_kbnk tb_kppkp tb_kqkrb_1 tb_kqkrb_2 \
               tb_krppkr tb_kbppkr
TBPROBE_SVG_FILES = $(patsubst %,svg/%.svg,$(TBPROBE_SVGS))

PROOFGAME_SVGS = castle_flags \
       trapped_bishop_1 trapped_bishop_2 \
       ext_kernel_example_illegal ext_kernel_example_legal \
       proofkernel_example revmoves_example \
       no_revmoves_1 no_revmoves_2 no_last_move_1 \
       heuristic_1_start heuristic_1_goal \
       heuristic_2_start heuristic_2_goal \
       pawn_cone_1 pawn_cone_2 deadlock_start deadlock_goal \
       only_irreversible non_admissible_goal non_admissible_best
PROOFGAME_SVG_FILES = $(patsubst %,svg/%.svg,$(PROOFGAME_SVGS))

SVG_FILES = $(TBPROBE_SVG_FILES) $(PROOFGAME_SVG_FILES)

PDFS = tbprobe proofgame
PDF_FILES = $(patsubst %,%.pdf,$(PDFS))

svgs	: $(SVG_FILES)
pdf	: svgs
pdf	: $(PDF_FILES)

svg	:
	mkdir -p svg

$(subst .,%,$(SVG_FILES))	: gensvg.py svg
	./gensvg.py

proofgame.pdf : proofgame.md $(PROOFGAME_SVG_FILES) Makefile
	pandoc -f gfm -t context -N $< -o $@

tbprobe.pdf : tbprobe.md $(TBPROBE_SVG_FILES) Makefile
	pandoc -f gfm -t context -N $< -o $@

clean	:
	rm -rf svg $(PDF_FILES)

.PHONY : clean
