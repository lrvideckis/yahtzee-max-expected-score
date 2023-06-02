#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <cassert>
#include <iomanip>

using namespace std;

int numberOfRoll[252];
int pow6[6];
vector<array<int, 5>> allRollsIndistinguishable;
vector<array<int, 5>> allRollsDistinguishable;

int getRollId(array<int, 5> roll) {
    for (int i = 0; i < (int)roll.size(); ++i)
        assert(1 <= roll[i] && roll[i] <= 6);
    sort(roll.begin(), roll.end());
    const auto it = lower_bound(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end(), roll);
    assert(*it == roll);
    return int(it - allRollsIndistinguishable.begin());
}

void initAllRolls() {
    pow6[0] = 1;
    for (int i = 1; i < 6; ++i)
        pow6[i] = 6 * pow6[i - 1];
    for (int rollVal = 0; rollVal < pow6[5]; ++rollVal) {
        int temp = rollVal;
        array<int, 5> roll;
        for (int i = 0; i < 5; ++i) {
            roll[i] = temp % 6 + 1;
            temp /= 6;
        }
        allRollsDistinguishable.push_back(roll);
        sort(roll.begin(), roll.end());
        allRollsIndistinguishable.push_back(roll);
    }
    sort(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end());
    allRollsIndistinguishable.erase(unique(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end()), allRollsIndistinguishable.end());
    assert(allRollsIndistinguishable.size() == 252);
    assert(allRollsDistinguishable.size() == 7776);//6 ^ 5
    for (const auto& roll : allRollsDistinguishable)
        ++numberOfRoll[getRollId(roll)];
}

enum scores {
    //top part
    ones,
    twos,
    threes,
    fours,
    fives,
    sixes,

    //bottom part
    threeOfAKind,
    fourOfAKind,
    fullHouse,
    smallStraight,
    largeStraight,
    yahtzee,
    chance
};

//scoreForRoll[i][j] = the score for roll with rollID=i if it counts for
//scores::x s.t. scores::x==j
int scoreForRoll[252][13];

void calculateScores() {
    for (int rollId = 0; rollId < (int)allRollsIndistinguishable.size(); ++rollId) {
        const array<int, 5>& roll = allRollsIndistinguishable[rollId];
        array<int, 7> cnts;
        fill(cnts.begin(), cnts.end(), 0);
        int mxCnt = 0;
        int sum = 0;
        for (int val : roll) {
            sum += val;
            ++cnts[val];
            mxCnt = max(mxCnt, cnts[val]);
            scoreForRoll[rollId][scores::ones] += (val == 1);
            scoreForRoll[rollId][scores::twos] += 2 * (val == 2);
            scoreForRoll[rollId][scores::threes] += 3 * (val == 3);
            scoreForRoll[rollId][scores::fours] += 4 * (val == 4);
            scoreForRoll[rollId][scores::fives] += 5 * (val == 5);
            scoreForRoll[rollId][scores::sixes] += 6 * (val == 6);
        }
        scoreForRoll[rollId][scores::chance] = sum;
        if (mxCnt >= 3)
            scoreForRoll[rollId][scores::threeOfAKind] = sum;
        if (mxCnt >= 4)
            scoreForRoll[rollId][scores::fourOfAKind] = sum;
        if (mxCnt >= 5)
            scoreForRoll[rollId][scores::yahtzee] = 50;
        bool seenCnt3 = false, seenCnt2 = false;
        for (int cnt : cnts) {
            seenCnt2 |= (cnt == 2);
            seenCnt3 |= (cnt == 3);
        }
        if (seenCnt2 && seenCnt3)
            scoreForRoll[rollId][scores::fullHouse] = 25;
        for (int straightLen = 4; straightLen <= 5; ++straightLen) {
            for (int startVal = 1; startVal + straightLen - 1 <= 6; ++startVal) {
                bool hasAll = true;
                for (int val = startVal, len = straightLen; len--; ++val) {
                    if (cnts[val] == 0) {
                        hasAll = false;
                        break;
                    }
                }
                if (hasAll) {
                    scoreForRoll[rollId][scores::smallStraight] = 30;
                    if (straightLen == 5)
                        scoreForRoll[rollId][scores::largeStraight] = 40;
                }
            }
        }
    }
}

vector<int> distinctSubsetsForReroll[252];
vector<pair<int, int>> cntReroll[252][1 << 5];

void calcHelperArraysForDP() {
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        map<vector<int>, int> keptDieToSubset;
        for (int subsetRerolled = 1; subsetRerolled < (1 << 5); ++subsetRerolled) {
            vector<int> keptDie;
            for (int die = 0; die < 5; ++die) {
                if ((subsetRerolled & (1 << die)) == 0)
                    keptDie.push_back(allRollsIndistinguishable[roll][die]);
            }
            sort(keptDie.begin(), keptDie.end());
            keptDieToSubset[keptDie] = subsetRerolled;
        }
        for (auto& p : keptDieToSubset)
            distinctSubsetsForReroll[roll].push_back(p.second);
    }
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        for (int subsetRerolled : distinctSubsetsForReroll[roll]) {
            const int iters = pow6[__builtin_popcount(subsetRerolled)];
            map<array<int, 5>, int> cnts;
            vector<array<int, 5>> tempRolls;
            for (int id = 0; id < iters; ++id) {
                array<int, 5> newRoll = allRollsIndistinguishable[roll];
                int ptr = 0;
                for (int die = 0; die < 5; ++die) {
                    if (subsetRerolled & (1 << die))
                        newRoll[die] = allRollsDistinguishable[id][ptr++];
                }
                //here, we have a triplet: (start roll, subset die re-rolled, end roll)
                sort(newRoll.begin(), newRoll.end());
                ++cnts[newRoll];
                tempRolls.push_back(newRoll);
            }
            sort(tempRolls.begin(), tempRolls.end());
            vector<int> tempCnt(252, 0);
            int ptr = 0;
            for (int i = 0; i < (int)tempRolls.size(); ++i) {
                while (ptr < (int)allRollsIndistinguishable.size() && allRollsIndistinguishable[ptr] < tempRolls[i]) ++ptr;
                assert(tempRolls[i] == allRollsIndistinguishable[ptr]);
                ++tempCnt[ptr];
            }
            for (int endRoll = 0; endRoll < 252; ++endRoll) {
                if (tempCnt[endRoll] > 0)
                    cntReroll[roll][subsetRerolled].emplace_back(tempCnt[endRoll], endRoll);
            }
        }
    }
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        for (int subsetRerolled : distinctSubsetsForReroll[roll]) {
            int sum = 0;
            for (auto [cnt, _] : cntReroll[roll][subsetRerolled])
                sum += cnt;
            assert(sum == pow6[__builtin_popcount(subsetRerolled)]);
        }
    }
}

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
    float& currDp = maxEV[subsetScoresFilled][pointsUpperToGo][numRerolls][rollId];
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
    if (FILE* expected_values_file = fopen("expected_value_table.bin", "rb")) {
        size_t sz = fread(maxEV, sizeof maxEV[0][0][0][0], (1 << 13) * 64 * 3 * 252, expected_values_file);
        fclose(expected_values_file);
        assert(sz == (1 << 13) * 64 * 3 * 252);
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
