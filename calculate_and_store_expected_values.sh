#!/bin/bash
g++ -std=c++17 -O2 calculate_expected_value_table.cpp -o calculate_expected_value_table.out
test -f expected_value_table.bin || touch expected_value_table.bin && ./calculate_expected_value_table.out
