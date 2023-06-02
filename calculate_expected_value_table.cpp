#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <cassert>
#include <iomanip>
#include <chrono>
using namespace std;
using namespace std::chrono;

#include "yahtzee_helper_arrays.hpp"

//maxEV[subset scores filled][num points you have to score to get upper bonus][num rerolls][roll] = max expected score
float maxEV[1 << 13][64][3][252];
float averageMaxEV[1 << 13][64];

void calcExpectedValue() {
    for (int subsetFilled = 1; subsetFilled < (1 << 13); ++subsetFilled) {
        if (subsetFilled % 50 == 0)
            cout << "progress: " << 100 * subsetFilled / float(1 << 13) << " %" << endl;
        for (int pointsUpperSection = 0; pointsUpperSection <= 63; pointsUpperSection++) {
            for (int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
                for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
                    float& currDp = maxEV[subsetFilled][pointsUpperSection][numberRerolls][roll];
                    //take roll
                    for (int scoreVal = 0; scoreVal < 13; ++scoreVal) {
                        if (subsetFilled & (1 << scoreVal)) {
                            int currScoreRoll = scoreForRoll[roll][scoreVal];
                            float nextScore = currScoreRoll;
                            if (scoreVal < 6) {//if upper section
                                int nextPointsUpperSection = max(pointsUpperSection - currScoreRoll, 0);
                                nextScore += averageMaxEV[subsetFilled ^ (1 << scoreVal)][nextPointsUpperSection];
                                if (pointsUpperSection > 0 && nextPointsUpperSection == 0)
                                    nextScore += 35.0;
                            } else
                                nextScore += averageMaxEV[subsetFilled ^ (1 << scoreVal)][pointsUpperSection];
                            currDp = max(currDp, nextScore);
                        }
                    }
                    //re-roll
                    if (numberRerolls > 0) {
                        //for each subset of die that you can re-roll
                        for (int subsetRerolled : distinctSubsetsForReroll[roll]) {
                            //find average of expected values
                            float nextScore = 0;
                            for (auto [cnt, endRoll] : cntReroll[roll][subsetRerolled])
                                nextScore += cnt * maxEV[subsetFilled][pointsUpperSection][numberRerolls - 1][endRoll];
                            nextScore /= float(pow6[__builtin_popcount(subsetRerolled)]);
                            currDp = max(currDp, nextScore);
                        }
                    }
                    if (numberRerolls == 2)
                        averageMaxEV[subsetFilled][pointsUpperSection] += numberOfRoll[roll] * currDp / float(pow6[5]);
                }
            }
        }
    }
}

int main() {
    cout << setprecision(5) << fixed;
    //
    cout << "calling initAllRolls ... " << flush;
    auto start = high_resolution_clock::now();
    initAllRolls();
    auto duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << float(duration.count()) / float(1000 * 1000) << " seconds." << endl;
    //
    cout << "calling calculateScores ... " << flush;
    start = high_resolution_clock::now();
    calculateScores();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << float(duration.count()) / float(1000 * 1000) << " seconds." << endl;
    //
    cout << "calling calcHelperArraysForDP ... " << flush;
    start = high_resolution_clock::now();
    calcHelperArraysForDP();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << float(duration.count()) / float(1000 * 1000) << " seconds." << endl;
    //
    //to test the commented example of distinctSubsetsForReroll
    assert((int)distinctSubsetsForReroll[getRollId({1, 1, 1, 1, 2})].size() == 9);
    //
    cout << "calling calcExpectedValue ... " << flush;
    start = high_resolution_clock::now();
    calcExpectedValue();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << float(duration.count()) / float(1000 * 1000) << " seconds." << endl;
    //
    cout << "The maximum expected score for a single Yahtzee round is" << endl <<
         averageMaxEV[(1 << 13) - 1][63] << " points. This is lower than the true value as" << endl <<
         "the program doesn't consider multiple yahtzees (each worth 100 points)" << endl << endl;
    cout << "saving expected values to file" << endl;
    if (FILE* expected_values_file = fopen("expected_value_table.bin", "wb")) {
        fwrite(maxEV, sizeof maxEV[0][0][0][0], (1 << 13) * 64 * 3 * 252, expected_values_file);
        fclose(expected_values_file);
    }
}
