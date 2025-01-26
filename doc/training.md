# Texel network training procedure

Training a neural network for the Texel evaluation function requires several
steps, which are described in this document. This procedure has only been tested
in Linux, and very little effort has been made to make the process user
friendly.


## Obtaining PGN data

Texel is trained on positions derived from self-play games between Texel
versions. Most of these games are played during development (using
cutechess-cli) to test if proposed code changes improve the engine strength.
These games are saved in a file called `games.pgn`.


## Convert PGN games to a list of positions (FEN data)

To convert the `games.pgn` file to a corresponding list of positions, run the
following command:

```cat games.pgn | texelutil p2f 1 1 >gamepos.txt```


## Generate training positions from game positions

Texel is not trained directly on the positions from games. Instead, each game
position is searched by Texel, and during the search, positions from the search
tree are randomly sampled and stored in a file called `positions.txt`.

To enable position sampling, edit `lib/texellib/debug/searchTreeSampler.hpp` and
change the definition of `SearchTreeSampler` to:

```using SearchTreeSampler = SearchTreeSamplerReal;```

You may also want to modify the constant in
```SearchTreeSamplerReal::sample()``` to control the sampling frequency.

Then recompile Texel and run the following command (change `16` to match the
number of available CPU cores):

```cat gamepos.txt | texelutil -j 16 searchfens 1000 10```

This will create a file called `positions.txt`.

The network used in Texel 1.12 was trained on 871 million positions.


## Evaluate training positions

Revert the changes in `lib/texellib/debug/searchTreeSampler.hpp` and recompile
Texel.

Create a script called `evalfens.sh` that uses Texel to evaluate a list of FEN
positions. An example script is included in this directory. The real script used
for training is more advanced and distributes the work to different computers.

Then run the following command:

```cat positions.txt | texelutil -j 16 search ./evalfens.sh >evaluated.txt```


## Create binary training data

Convert the evaluated positions to a binary representation:

```cat evaluated.txt | texelutil fen2bin evaluated.bin```


## Add game result data to the training data

```
cat games.txt | texelutil p2f 3 | texelutil fen2bin -useResult result.bin
cat evaluated.bin result.bin >eval+result.bin
```


## Train the neural network

Perform the initial training by running:

```OMP_NUM_THREADS=1 torchutil train eval+result.bin```

Perform quantization aware training by running:

```
mkdir qat
cd qat
OMP_NUM_THREADS=1 torchutil train -i ../model40.pt -lr 6e-6 -epochs 10 -qat ../eval+result.bin
cd ..
```


## Convert the PyTorch model to Texel format

Create a Texel network file by running:

```torchutil quant -c -pl 1 qat/model10.pt nndata.tbin evaluated.bin```

Copy the created `nndata.tbin.compr` file to the Texel directory and recompile
Texel.
