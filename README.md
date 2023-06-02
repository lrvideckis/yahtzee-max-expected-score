# Yahtzee Maximum Expected Score

This repo contains the following files:
- `calculate_expected_value_table.cpp`: calculates expected values/scores and writes it to `expected_value_table.bin`. It takes about ~6.7 minutes to run on my laptop
- `expected_value_table.bin`: the table of expected scores I got via running `calculate_expected_value_table.cpp` myself. You can either generate this yourself locally (`./calculate_and_store_expected_values.sh`; **make sure to create the empty file expected_value_table.bin before running calculate_expected_value_table.cpp!!&&), or attempt to download it from this repo, both options will take a while :(
- `read_table_and_IO.cpp`: it reads in the expected value table from `expected_value_table.bin` then does a cumbersome command-line IO (LMK if you have ideas of how to improve the format of IO)

I tested the program by comparing values with this website: http://www-set.win.tue.nl/~wstomv/misc/yahtzee/osyp.php


example run:

```
Calculating expected values, should take 3-4 seconds... Finished in 3.21408 seconds.

The maximum expected score for a single Yahtzee round is
229.63850 points. This is lower than the true value as
the program doesn't consider multiple yahtzees (each worth 100 points), or
the +35 point bonus for scoring >= 63 points in the top section.

enter in score for ones (or -1 for not filled yet).............. -1
enter in score for twos (or -1 for not filled yet).............. -1
enter in score for threes (or -1 for not filled yet)............ 9
enter in score for fours (or -1 for not filled yet)............. 12
enter in score for fives (or -1 for not filled yet)............. -1
enter in score for sixes (or -1 for not filled yet)............. 12
enter in score for three of a kind (or -1 for not filled yet)... 19
enter in score for four of a kind (or -1 for not filled yet).... -1
enter in score for full house (or -1 for not filled yet)........ 25
enter in score for small straight (or -1 for not filled yet).... -1
enter in score for large straight (or -1 for not filled yet).... 40
enter in score for yahtzee (or -1 for not filled yet)........... -1
enter in score for chance (or -1 for not filled yet)............ 24
enter in dice roll (ex: 2 4 6 3 2).............................. 5 6 1 1 4
number of re-rolls left (0, 1, or 2)............................ 1

Options are:
Keep die 5 _ _ _ _ and reroll giving 208.82924 expected points.
Keep die 5 _ _ _ 4 and reroll giving 208.82140 expected points.
Keep die _ _ 1 1 _ and reroll giving 208.81006 expected points.
Keep die 5 6 _ _ 4 and reroll giving 208.62430 expected points.
Keep die _ _ _ _ 4 and reroll giving 208.57608 expected points.
Keep die _ _ _ _ _ and reroll giving 208.54993 expected points.
Keep die _ _ 1 1 4 and reroll giving 208.54234 expected points.
Keep die _ _ 1 _ 4 and reroll giving 208.49311 expected points.
Keep die _ _ 1 _ _ and reroll giving 208.44879 expected points.
Keep die 5 _ 1 1 _ and reroll giving 208.38386 expected points.
Keep die _ 6 1 1 _ and reroll giving 208.28239 expected points.
Keep die 5 6 _ _ _ and reroll giving 208.21692 expected points.
Keep die 5 _ 1 _ _ and reroll giving 208.21439 expected points.
Keep die _ 6 1 1 4 and reroll giving 208.10691 expected points.
Keep die 5 6 1 1 _ and reroll giving 208.10691 expected points.
Keep die 5 _ 1 1 4 and reroll giving 208.10691 expected points.
Keep die _ 6 _ _ _ and reroll giving 208.07231 expected points.
Keep die 5 _ 1 _ 4 and reroll giving 208.07041 expected points.
Keep die 5 6 1 _ 4 and reroll giving 208.05340 expected points.
Keep die _ 6 1 _ _ and reroll giving 207.94523 expected points.
Keep die _ 6 1 _ 4 and reroll giving 207.94117 expected points.
Score roll as ones giving 207.94024 expected points.
Keep die _ 6 _ _ 4 and reroll giving 207.93483 expected points.
Keep die 5 6 1 _ _ and reroll giving 207.75491 expected points.
Score roll as twos giving 204.25783 expected points.
Score roll as yahtzee giving 202.96577 expected points.
Score roll as fives giving 201.59302 expected points.
Score roll as four of a kind giving 198.13692 expected points.
Score roll as small straight giving 182.61921 expected points.


enter in score for ones (or -1 for not filled yet)..............
```
