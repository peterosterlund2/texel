#!/usr/bin/python

import sys
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Ellipse

def numlegal_sets():
    fig, ax = plt.subplots(figsize=(10, 6))

    # --- S (universe) ---
    S = Rectangle((0, 0), 12, 8,
                  fill=False, edgecolor='black', linewidth=2)

    # --- candidates ---
    candidates = Rectangle((0.7, 2.0), 7.0, 5.5,
                           facecolor='#c9c9c9', alpha=0.25,
                           edgecolor='none')

    # --- legal (subset of candidates) ---
    legal = Rectangle((0.9, 2.5), 6.2, 4.5,
                      facecolor='#7fc97f', alpha=0.6,
                      edgecolor='none')

    # --- sample ---
    # Crosses outside candidates, but still intersects legal and candidates\legal
    sample = Ellipse((8.5, 5.0), width=6.0, height=2.5,
                     facecolor='#80b1d3', alpha=0.45,
                     edgecolor='none')

    # --- intersection: S_sample ∩ S_candidates ---
    sample_in_candidates = Ellipse(sample.get_center(),
                                   width=sample.get_width(),
                                   height=sample.get_height(),
                                   facecolor='none',
                                   hatch='///',
                                   edgecolor='navy',
                                   linewidth=0.0)

    # --- S2 ---
    # Entirely inside sample and candidates, but only overlaps part of legal
    S2 = Ellipse((6.7, 5.0), width=1.5, height=0.9,
                 facecolor='#fb8072', alpha=0.8,
                 edgecolor='none')

    # --- highlight S2 - S_legal ---
    legal_right = legal.get_x() + legal.get_width()
    right_of_legal = Rectangle((legal_right, 3), 8 - legal_right, 4,
                               alpha=0.0)

    S2_minus_legal = Ellipse(S2.get_center(),
                             width=S2.get_width(),
                             height=S2.get_height(),
                             facecolor='yellow',
                             alpha=0.5,
                             edgecolor='none')

    # -- Draw patches ---
    for patch in [S, candidates, legal, sample, sample_in_candidates, S2, S2_minus_legal, right_of_legal]:
        ax.add_patch(patch)

    sample_in_candidates.set_clip_path(candidates)
    S2_minus_legal.set_clip_path(right_of_legal)

    # Labels
    ax.text(0.25, 7.5, 'S', fontsize=14)
    ax.text(1.0, 7.15, r'$S_{\mathrm{candidates}}$', fontsize=13)
    ax.text(1.3, 6.4, r'$S_{\mathrm{legal}}$', fontsize=13)
    ax.text(9.8, 5.0, r'$S_{\mathrm{sample}}$', fontsize=13)
    ax.text(6.7, 5.0, r'$S_2$', fontsize=13, ha='center', va='center')

    ax.annotate(r'$S_{\mathrm{n1\_candidates}} =$' + '\n' +
                r'$S_{\mathrm{sample}} \cap S_{\mathrm{candidates}}$',
                xy=(7.2, 4.25),       # point inside the set
                xytext=(9.8, 3.0),    # label position
                fontsize=12,
                ha='center',
                va='center',
                arrowprops=dict(arrowstyle='-', lw=1.2))

    ax.annotate(r'$S_2 \backslash S_{\mathrm{legal}}$',
                xy=(7.25, 5.0),       # point inside the set
                xytext=(8.1, 7.0),    # label position
                fontsize=12,
                ha='left',
                va='center',
                arrowprops=dict(arrowstyle='-', lw=1.2))

    # Info boxes
    info = (
        r'$|S| = 2.196\ldots \times 10^{53}$' '\n'
        r'$|S_{\mathrm{candidates}}| ≈ 4.82 \times 10^{44}$' '\n'
        r'$|S_{\mathrm{legal}}| ≈      4.82 \times 10^{44}$' '\n'
        r'$|S_{\mathrm{candidates}} \backslash S_{\mathrm{legal}}| ≈ 8.2 \times 10^{40}$'
    )

    ax.text(
        3.7, 0.25, info,
        fontsize=11,
        ha='left',
        va='bottom',
        linespacing=1.0,
        bbox=dict(
            boxstyle='round,pad=0.6',
            facecolor='white',
            edgecolor='gray',
            alpha=0.9
        )
    )

    info2 = (
        r'$|S_{\mathrm{sample}}| = 2.745\ldots \times 10^{15}$' '\n'
        r'$|S_{\mathrm{n1\_candidates}}| = 6030640$' '\n'
        r'$|S_2| = 10^5$' '\n'
        r'$|S_2 \backslash S_{\mathrm{legal}}| = 17$'
    )

    ax.text(
        8.0, 0.25, info2,
        fontsize=11,
        ha='left',
        va='bottom',
        linespacing=1.0,
        bbox=dict(
            boxstyle='round,pad=0.6',
            facecolor='white',
            edgecolor='gray',
            alpha=0.9
        )
    )

    # Nice plotting settings
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8)
    ax.set_aspect('equal')
    ax.axis('off')

    mpl.rcParams['svg.hashsalt'] = 'sets'
    plt.savefig('svg/numlegal_sets.svg',
                bbox_inches='tight',
                pad_inches=0.0,
                metadata={'Date': None})


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: ./customsvgs.py name')
        sys.exit(2)

    name = sys.argv[1]
    if name == 'numlegal_sets':
        numlegal_sets()
    else:
        print(f'Unknown name: {name}')
