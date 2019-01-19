# Kiwi 0.6d

Kiwi is a chess engine. It can be used with any program compatible with the [xboard protocol](https://www.gnu.org/software/xboard/engine-intf.html).

## Notes

Kiwi is the first chess engine I wrote... it's a very old program now! It is also one of the very few engines to use the [MTD(f) algorithm](https://en.wikipedia.org/wiki/MTD-f) invented by Aske Plaat.

Not a very strong engine, especially by today's standards, it can be still quite challenging for human players and in general it's a well rounded player. 

Kiwi supports opening books and bitbases (endgame tablebases) in its own proprietary format. To use the engine in actual play, it is in fact highly recommended to give it both an opening book and at least a few basic bitbases. The opening book will give the engine some variety in the opening and will make for more interesting games, while the bitbases will help it play better some endgames that would be otherwise very difficult. A couple of books and some bitbases are available in the `data` folder.

I haven't tried to recompile the code in years. It could be a nice experiment though, as Kiwi internally uses a 64-bit bitboard-based representation of the chess board, yet it has never been compiled on a 64-bit platform... maybe one of these days! :-)

## License

See LICENSE file.
