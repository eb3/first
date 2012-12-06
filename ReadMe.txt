This week I have edited the 3moduleFinal program to add an additional case to the score file.

- Reads in an AIFF file (I have included loop.aiff).

- Plays back the loop high-pass filtered (the frequency is set in score.c in the twoPoleHigh arguments)

- I edited the names of the filters in modules.c and updated the header.h file to reflect these changes. They are now called: onePoleLow, and twoPoleHigh

- I used Max Mathews' twopole function as a template and modified it to work as a high-pass filter

To compile and run the program do: gcc -o modular main.c modules.c gens.c score.c -lportaudio -lportsf

You can use the new score case #5 by doing something like: ./modular 440 3 5. The first integer (frequency) doesn't matter, the second integer will control how long the audio playback lasts in seconds, but the included loop file is only 7 seconds.