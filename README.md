# Yahtzee Maximum Expected Score

This repo contains the following files:
- `calculate_expected_value_table.cpp` (run with `calculate_and_store_expected_values.sh`): calculates expected values/scores and writes it to `expected_value_table.bin`.
  - **make sure to create an empty file expected_value_table.bin before running calculate_expected_value_table.cpp!!!!!!!!!!** or else you'll wait 6-7 minutes to for it to run, and it won't get saved to the file
  - `expected_value_table.bin` is 1.6 GB
- `read_table_and_IO.cpp` (run with `./run.sh`): it reads in the expected value table from `expected_value_table.bin` then does a cumbersome command-line IO (LMK if you have ideas of how to improve the format of IO)

I don't plan to push the `expected_value_table.bin` file as github will charge me $5 a month to host it

I tested the program by comparing values with this website: http://www-set.win.tue.nl/~wstomv/misc/yahtzee/osyp.php


example run:

```
./run.sh
reading expected value table from expected_value_table.bin
The maximum expected score for a single Yahtzee round is
245.87077 points. This is lower than the true value as
the program doesn't consider multiple yahtzees (each worth 100 points)

enter in score for ones (or -1 for not filled yet).............. 3
enter in score for twos (or -1 for not filled yet).............. -1
enter in score for threes (or -1 for not filled yet)............ 9
enter in score for fours (or -1 for not filled yet)............. 12
enter in score for fives (or -1 for not filled yet)............. 15
enter in score for sixes (or -1 for not filled yet)............. 18
enter in score for three of a kind (or -1 for not filled yet)... -1
enter in score for four of a kind (or -1 for not filled yet).... -1
enter in score for full house (or -1 for not filled yet)........ -1
enter in score for small straight (or -1 for not filled yet).... -1
enter in score for large straight (or -1 for not filled yet).... -1
enter in score for yahtzee (or -1 for not filled yet)........... -1
enter in score for chance (or -1 for not filled yet)............ -1
enter in dice roll (ex: 2 4 6 3 2).............................. 2 2 3 5 1
number of re-rolls left (0, 1, or 2)............................ 2

Options are:
Keep die 2 2 _ _ _ and reroll giving 233.15741 expected points.
Keep die 2 2 3 _ _ and reroll giving 231.67584 expected points.
Keep die 2 2 _ 5 _ and reroll giving 231.53122 expected points.
Keep die 2 2 _ _ 1 and reroll giving 231.36398 expected points.
Keep die 2 2 3 5 _ and reroll giving 230.05951 expected points.
Keep die 2 2 3 _ 1 and reroll giving 229.55774 expected points.
Keep die 2 _ 3 5 _ and reroll giving 229.54678 expected points.
Keep die 2 _ _ _ _ and reroll giving 229.48814 expected points.
Keep die 2 _ 3 _ _ and reroll giving 229.17088 expected points.
Keep die 2 2 _ 5 1 and reroll giving 229.12396 expected points.
Keep die 2 _ _ 5 _ and reroll giving 229.10170 expected points.
Keep die _ _ _ 5 _ and reroll giving 228.53186 expected points.
Keep die _ _ 3 5 _ and reroll giving 228.40732 expected points.
Keep die _ _ _ _ _ and reroll giving 228.24413 expected points.
Keep die _ _ 3 _ _ and reroll giving 228.05923 expected points.
Keep die 2 _ 3 5 1 and reroll giving 227.77571 expected points.
Keep die 2 _ _ _ 1 and reroll giving 227.61565 expected points.
Keep die 2 _ 3 _ 1 and reroll giving 227.55289 expected points.
Keep die 2 _ _ 5 1 and reroll giving 226.80219 expected points.
Keep die _ _ _ _ 1 and reroll giving 226.57233 expected points.
Keep die _ _ _ 5 1 and reroll giving 226.51305 expected points.
Keep die _ _ 3 _ 1 and reroll giving 226.33034 expected points.
Keep die _ _ 3 5 1 and reroll giving 226.19484 expected points.
Score roll as yahtzee giving 216.44611 expected points.
Score roll as four of a kind giving 214.50961 expected points.
Score roll as chance giving 212.40567 expected points.
Score roll as full house giving 211.29120 expected points.
Score roll as three of a kind giving 205.49962 expected points.
Score roll as large straight giving 204.74057 expected points.
Score roll as twos giving 200.97118 expected points.
Score roll as small straight giving 196.67731 expected points.

enter in score for ones (or -1 for not filled yet)..............
```
