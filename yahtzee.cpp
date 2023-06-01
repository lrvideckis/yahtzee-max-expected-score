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

//distinctSubsetsForReroll[roll id] = vector of ways to remove die (size <= 2^5=32) such that
//no 2 subsets will leave the same set of die
vector<int> distinctSubsetsForReroll[252];

//cntReroll[roll id][subset rerolled] = list of possible die roll-ids after rerolling and their counts
//for example if you take the roll "1 2 3 1 1" and re-roll the 2 and the 3
//then you have 36 possibilities for the next roll:
//1 1 1 1 1
//1 1 2 1 1
//1 1 3 1 1
//...
//1 1 6 1 1
//1 2 1 1 1
//1 2 2 1 1
//1 2 3 1 1
//...
//1 2 6 1 1
//1 3 1 1 1
//1 3 2 1 1
//...
//1 6 6 1 1
//but looking at this list, some rolls appear twice (after uniqueing), like "1 2 3 1 1" and "1 3 2 1 1"
//so these will appear in cntReroll[...][...] as a pair: {2, roll id of "1 2 3 1 1"}
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
}

struct Move {
    int subsetReroll, scoreTaken;
    double evForMove;
};

bool operator<(const Move& x, const Move& y) {
    return x.evForMove > y.evForMove;
}

//maxEV[subset scores filled][num rerolls][roll] = max expected score
double maxEV[1 << 13][3][252];
double averageMaxEV[1 << 13];
vector<Move> transitions[1 << 13][3][252];

void calcExpectedValue() {
    for (int subsetFilled = 1; subsetFilled < (1 << 13); ++subsetFilled) {
        for (int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
            for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
                double& currDp = maxEV[subsetFilled][numberRerolls][roll];
                vector<Move>& currTransitions = transitions[subsetFilled][numberRerolls][roll];
                //take roll
                for (int scoreVal = 0; scoreVal < 13; ++scoreVal) {
                    if (subsetFilled & (1 << scoreVal)) {
                        const double nextScore = scoreForRoll[roll][scoreVal] + averageMaxEV[subsetFilled ^ (1 << scoreVal)];
                        currTransitions.push_back({-1, scoreVal, nextScore});
                        currDp = max(currDp, nextScore);
                    }
                }
                //re-roll
                if (numberRerolls > 0) {
                    //for each subset of die that you can re-roll
                    for (int subsetRerolled : distinctSubsetsForReroll[roll]) {//number of iterations is <= 32
                        //find average of expected values
                        double nextScore = 0;
                        for (auto [cnt, endRoll] : cntReroll[roll][subsetRerolled])//number of iterations is <= 252
                            nextScore += cnt * maxEV[subsetFilled][numberRerolls - 1][endRoll];
                        nextScore /= double(pow6[__builtin_popcount(subsetRerolled)]);
                        currTransitions.push_back({subsetRerolled, -1, nextScore});
                        currDp = max(currDp, nextScore);
                    }
                }
                if (numberRerolls == 2)
                    averageMaxEV[subsetFilled] += numberOfRoll[roll] * currDp / double(pow6[5]);
            }
        }
    }
}

bool cmpSeconds(const pair<int, int>& x, const pair<int, int>& y) {
    return x.second < y.second;
}

void inputOutput() {
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
        int subsetFilled = 0;
        for (int i = 0; i < 13; ++i) {
            cout << "enter in score for " << scoreDescription[i] << " (or -1 for not filled yet)";
            cout << string(18 - (int)scoreDescription[i].size(), '.') << ' ';
            int score;
            cin >> score;
            if (score >= 0)
                sumFilledScores += score;
            else
                subsetFilled += (1 << i);
        }
        cout << "enter in dice roll (ex: 2 4 6 3 2)" << string(30, '.') << ' ';
        vector<pair<int, int>> roll(5);
        array<int, 5> rollInt;
        for (int i = 0; i < 5; ++i) {
            cin >> roll[i].first;
            roll[i].second = i;
            rollInt[i] = roll[i].first;
        }
        cout << "number of re-rolls left (0, 1, or 2)" << string(28, '.') << ' ';
        int rerolls;
        cin >> rerolls;
        vector<Move>& currTransitions = transitions[subsetFilled][rerolls][getRollId(rollInt)];
        sort(currTransitions.begin(), currTransitions.end());
        cout << endl << "Options are:" << endl;
        for (const auto& currMove : currTransitions) {
            if (currMove.subsetReroll == -1) //score roll
                cout << "Score roll as " << scoreDescription[currMove.scoreTaken] << " giving " << double(sumFilledScores) + currMove.evForMove << " expected points." << endl;
            else {//re roll
                assert(currMove.scoreTaken == -1);
                cout << "Keep die";
                sort(roll.begin(), roll.end());
                vector<bool> reroll(5, false);
                for (int die = 0; die < 5; ++die) {
                    if (currMove.subsetReroll & (1 << die))
                        reroll[roll[die].second] = true;
                }
                sort(roll.begin(), roll.end(), cmpSeconds);
                for (int die = 0; die < 5; ++die) {
                    if (reroll[die])
                        cout << " _";
                    else
                        cout << " " << roll[die].first;
                }
                cout << " and reroll giving " << double(sumFilledScores) + currMove.evForMove << " expected points." << endl;
            }
        }
        cout << endl << endl;
    }
}

int main() {
    cout << setprecision(5) << fixed;
    //
    cout << "calling initAllRolls ... " << flush;
    auto start = high_resolution_clock::now();
    initAllRolls();
    auto duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << double(duration.count()) / double(1000 * 1000) << " seconds." << endl;
    //
    cout << "calling calculateScores ... " << flush;
    start = high_resolution_clock::now();
    calculateScores();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << double(duration.count()) / double(1000 * 1000) << " seconds." << endl;
    //
    cout << "calling calcHelperArraysForDP ... " << flush;
    start = high_resolution_clock::now();
    calcHelperArraysForDP();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << double(duration.count()) / double(1000 * 1000) << " seconds." << endl;
    //
    cout << "calling calcExpectedValue ... " << flush;
    start = high_resolution_clock::now();
    calcExpectedValue();
    duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    cout << "Finished in " << double(duration.count()) / double(1000 * 1000) << " seconds." << endl;
    //
    cout << "The maximum expected score for a single Yahtzee round is" << endl <<
         averageMaxEV[(1 << 13) - 1] << " points. This is lower than the true value as" << endl <<
         "the program doesn't consider multiple yahtzees (each worth 100 points), or" << endl <<
         "the +35 point bonus for scoring >= 63 points in the top section." << endl << endl;
    //
    inputOutput();
}
