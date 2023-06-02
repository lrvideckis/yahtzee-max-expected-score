#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <cassert>
#include <iomanip>

using namespace std;

#include "yahtzee_helper_arrays.hpp"

float maxEV[1 << 13][64][3][252];
float averageMaxEV[1 << 13][64];

struct Transition {
    int subsetReroll = -1, scoreTaken = -1;
    float expectedValue = 0.0;
    bool receivedUpperBonus = false;

    bool operator<(const Transition& other) {
        return expectedValue > other.expectedValue;
    }
};

vector<Transition> getTransitionsForState(int subsetScoresFilled, int pointsUpperSectionSoFar, int numRerolls, array<int, 5> roll) {
    int pointsUpperToGo = min(63 - pointsUpperSectionSoFar, 0);
    int rollId = getRollId(roll);
    vector<Transition> transitions;
    //take roll
    for (int scoreVal = 0; scoreVal < 13; ++scoreVal) {
        if (subsetScoresFilled & (1 << scoreVal)) {
            Transition currTransition;
            currTransition.scoreTaken = scoreVal;
            int currScoreRoll = scoreForRoll[rollId][scoreVal];
            float nextScore = currScoreRoll;
            if (scoreVal < 6) {//if upper section
                int nextPointsUpperSection = max(pointsUpperToGo - currScoreRoll, 0);
                nextScore += averageMaxEV[subsetScoresFilled ^ (1 << scoreVal)][nextPointsUpperSection];
                if (pointsUpperToGo > 0 && nextPointsUpperSection == 0) {
                    nextScore += 35.0;
                    currTransition.receivedUpperBonus = true;
                }
            } else
                nextScore += averageMaxEV[subsetScoresFilled ^ (1 << scoreVal)][pointsUpperToGo];
            currTransition.expectedValue = nextScore;
            transitions.push_back(currTransition);
        }
    }
    //re-roll
    if (numRerolls > 0) {
        //for each subset of die that you can re-roll
        for (int subsetRerolled : distinctSubsetsForReroll[rollId]) {
            Transition currTransition;
            currTransition.subsetReroll = subsetRerolled;
            //find average of expected values
            float nextScore = 0;
            for (auto [cnt, endRoll] : cntReroll[rollId][subsetRerolled])
                nextScore += cnt * maxEV[subsetScoresFilled][pointsUpperToGo][numRerolls - 1][endRoll];
            nextScore /= float(pow6[__builtin_popcount(subsetRerolled)]);
            currTransition.expectedValue = nextScore;
            transitions.push_back(currTransition);
        }
    }
    return transitions;
}

int main() {
    cout << setprecision(5) << fixed;
    initAllRolls();
    calculateScores();
    calcHelperArraysForDP();
    cout << "reading expected value table from expected_value_table.bin" << endl;
    if (FILE* expected_values_file = fopen("expected_value_table.bin", "rb")) {
        size_t sz = fread(maxEV, sizeof maxEV[0][0][0][0], (1 << 13) * 64 * 3 * 252, expected_values_file);
        fclose(expected_values_file);
        assert(sz == (1 << 13) * 64 * 3 * 252);
    } else {
        cout << "expected_value_table.bin file doesn't exist :(" << endl;
        return 0;
    }
    for (int subsetFilled = 1; subsetFilled < (1 << 13); ++subsetFilled)
        for (int pointsUpperSection = 0; pointsUpperSection <= 63; pointsUpperSection++)
            for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll)
                averageMaxEV[subsetFilled][pointsUpperSection] += numberOfRoll[roll] * maxEV[subsetFilled][pointsUpperSection][2][roll] / float(pow6[5]);
    cout << "The maximum expected score for a single Yahtzee round is" << endl <<
         averageMaxEV[(1 << 13) - 1][63] << " points. This is lower than the true value as" << endl <<
         "the program doesn't consider multiple yahtzees (each worth 100 points)" << endl << endl;
    vector<string> scoreDescription = {
        "ones",
        "twos",
        "threes",
        "fours",
        "fives",
        "sixes",
        "three of a kind",
        "four of a kind",
        "full house",
        "small straight",
        "large straight",
        "yahtzee",
        "chance"
    };
    while (true) {
        int sumFilledScores = 0;
        int sumUpperSection = 0;
        int subsetFilled = 0;
        for (int i = 0; i < 13; ++i) {
            cout << "enter in score for " << scoreDescription[i] << " (or -1 for not filled yet)";
            cout << string(18 - (int)scoreDescription[i].size(), '.') << ' ';
            int score;
            cin >> score;
            if (score >= 0) {
                sumFilledScores += score;
                if (i < 6)
                    sumUpperSection += score;
            } else
                subsetFilled += (1 << i);
        }
        if (sumUpperSection >= 63)
            sumFilledScores += 35;
        cout << "enter in dice roll (ex: 2 4 6 3 2)" << string(30, '.') << ' ';
        array<int, 5> roll;
        array<pair<int, int>, 5> rollWithIndex;
        for (int i = 0; i < 5; ++i) {
            cin >> roll[i];
            rollWithIndex[i] = {roll[i], i};
        }
        cout << "number of re-rolls left (0, 1, or 2)" << string(28, '.') << ' ';
        int numRerolls;
        cin >> numRerolls;
        vector<Transition> transitions = getTransitionsForState(subsetFilled, sumUpperSection, numRerolls, roll);
        sort(transitions.begin(), transitions.end());
        cout << endl << "Options are:" << endl;
        for (const auto& currTransition : transitions) {
            if (currTransition.subsetReroll == -1) {//score roll
                assert(currTransition.scoreTaken != -1);
                cout << "Score roll as " << scoreDescription[currTransition.scoreTaken] << " giving " << float(sumFilledScores) + currTransition.expectedValue << " expected points." << endl;
            } else {//re roll
                assert(currTransition.scoreTaken == -1);
                cout << "Keep die";
                sort(rollWithIndex.begin(), rollWithIndex.end());
                vector<bool> reroll(5, false);
                for (int die = 0; die < 5; ++die) {
                    if (currTransition.subsetReroll & (1 << die))
                        reroll[rollWithIndex[die].second] = true;
                }
                sort(rollWithIndex.begin(), rollWithIndex.end(), [](const pair<int, int>& x, const pair<int, int>& y) -> bool {
                         return x.second < y.second;
                     });
                for (int die = 0; die < 5; ++die) {
                    if (reroll[die])
                        cout << " _";
                    else
                        cout << " " << rollWithIndex[die].first;
                }
                cout << " and reroll giving " << float(sumFilledScores) + currTransition.expectedValue << " expected points." << endl;
            }
        }
        cout << endl << endl;
    }
    return 0;
}
