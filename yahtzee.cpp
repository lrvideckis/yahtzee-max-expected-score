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

int scoreForRoll[252][13];

vector<array<int, 5>> allRollsIndistinguishable;
vector<array<int, 5>> allRollsDistinguishable;

//scoreForRoll[i][j] = the score for roll with rollID=i if it counts for
//scores::x s.t. scores::x==j
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

int getRollId(array<int, 5> roll) {
    for (int i = 0; i < (int)roll.size(); ++i)
        assert(1 <= roll[i] && roll[i] <= 6);
    sort(roll.begin(), roll.end());
    const auto it = lower_bound(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end(), roll);
    assert(*it == roll);
    return int(it - allRollsIndistinguishable.begin());
}

int numberOfRoll[252];
int pow6[6];

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
    for (const auto& roll : allRollsDistinguishable)
        ++numberOfRoll[getRollId(roll)];
}

//maxEV[subset scores filled][num rerolls][roll] = max expected score
double maxEV[1 << 13][3][252];
double averageMaxEV[1 << 13];
vector<pair<int, int>> distinctSubsetsForReroll[252];
vector<pair<int, int>> cntReroll[252][1 << 5];
int tempCnt[252];
array<int, 5> tempRolls[7776];

struct Move {
    int subsetReroll, scoreTaken;
    double evForMove;
};

bool operator<(const Move& x, const Move& y) {
    return x.evForMove > y.evForMove;
}

vector<Move> transitions[1 << 13][3][252];

vector<pair<int, int>> rollToSubsetKeptCnts[252];

void calcExpectedValue() {
    cout << "Calculating expected values, should take 3-4 seconds... " << flush;
    auto start = high_resolution_clock::now();
    vector<vector<int>> allDieKept;
    for (int i = 0; i < 36; ++i) {
        int die1 = allRollsDistinguishable[i][0];
        int die2 = allRollsDistinguishable[i][1];
        if (die1 > die2) swap(die1, die2);
        allDieKept.push_back({});
        allDieKept.push_back({die1});
        allDieKept.push_back({die1, die2});
    }
    sort(allDieKept.begin(), allDieKept.end());
    allDieKept.erase(unique(allDieKept.begin(), allDieKept.end()), allDieKept.end());
    assert(allDieKept.size() == 28);
    {
        vector<vector<int>> rerollCnts(28, vector<int>(252, 0));
        for (int dieKeptID = 0; dieKeptID < (int)allDieKept.size(); ++dieKeptID) {
            const auto& dieKept = allDieKept[dieKeptID];
            const int numReroll = 5 - (int)dieKept.size();
            const int iters = pow6[numReroll];
            for (int id = 0; id < iters; ++id) {
                array<int, 5> newRoll;
                int newRollSz = 0;
                for (int val : dieKept)
                    newRoll[newRollSz++] = val;
                for (int i = 0; i < numReroll; ++i)
                    newRoll[newRollSz++] = allRollsDistinguishable[id][i];
                assert(newRollSz == 5);
                sort(newRoll.begin(), newRoll.end());
                ++rerollCnts[dieKeptID][getRollId(newRoll)];
            }
        }
        for (int rollID = 0; rollID < 252; ++rollID) {
            for (int dieKeptID = 0; dieKeptID < (int)allDieKept.size(); ++dieKeptID) {
                if (rerollCnts[dieKeptID][rollID] > 0) {
                    rollToSubsetKeptCnts[rollID].push_back({dieKeptID, rerollCnts[dieKeptID][rollID]});
                }
            }
        }
    }
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        map<vector<int>, pair<int, int>> keptDieToSubset;
        for (int subsetRerolled = 1; subsetRerolled < (1 << 5); ++subsetRerolled) {
            vector<int> keptDie;
            for (int die = 0; die < 5; ++die) {
                if ((subsetRerolled & (1 << die)) == 0)
                    keptDie.push_back(allRollsIndistinguishable[roll][die]);
            }
            sort(keptDie.begin(), keptDie.end());
            keptDieToSubset[keptDie] = {subsetRerolled, lower_bound(allDieKept.begin(), allDieKept.end(), keptDie) - allDieKept.begin()};
        }
        for (auto& p : keptDieToSubset)
            distinctSubsetsForReroll[roll].push_back(p.second);
    }
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        for (auto [subsetRerolled, keptDieID] : distinctSubsetsForReroll[roll]) {
            const int iters = pow6[__builtin_popcount(subsetRerolled)];
            int sz = 0;
            map<array<int, 5>, int> cnts;
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
                tempRolls[sz++] = newRoll;
            }
            sort(tempRolls, tempRolls + sz);
            for (int endRoll = 0; endRoll < 252; ++endRoll)
                tempCnt[endRoll] = 0;
            int ptr = 0;
            for (int i = 0; i < sz; ++i) {
                while (ptr < (int)allRollsIndistinguishable.size() && allRollsIndistinguishable[ptr] < tempRolls[i]) ++ptr;
                assert(tempRolls[i] == allRollsIndistinguishable[ptr]);
                ++tempCnt[ptr];
            }
            for (int endRoll = 0; endRoll < 252; ++endRoll) {
                if (tempCnt[endRoll] > 0) {
                    cntReroll[roll][subsetRerolled].push_back({tempCnt[endRoll], endRoll});
                }
            }
        }
    }
    for (int subsetFilled = 1; subsetFilled < (1 << 13); ++subsetFilled) {
        vector<double> sumOfDpValsForSubsetKept(28, 0.0);
        for (int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
            vector<double> newSumOfDpVals(28, 0.0);
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
                    for (auto [subsetRerolled, keptDieID] : distinctSubsetsForReroll[roll]) {
                        //find average of expected values
                        double nextScore = 0;
                        if (__builtin_popcount(subsetRerolled) >= 3)
                            nextScore = sumOfDpValsForSubsetKept[keptDieID];
                        else {
                            for (auto [cnt, endRoll] : cntReroll[roll][subsetRerolled])
                                nextScore += cnt * maxEV[subsetFilled][numberRerolls - 1][endRoll];
                        }
                        nextScore /= double(pow6[__builtin_popcount(subsetRerolled)]);
                        currTransitions.push_back({subsetRerolled, -1, nextScore});
                        currDp = max(currDp, nextScore);
                    }
                }
                if (numberRerolls == 2)
                    averageMaxEV[subsetFilled] += numberOfRoll[roll] * currDp / double(pow6[5]);
                else {
                    for (auto [subsetKeptID, cnt] : rollToSubsetKeptCnts[roll])
                        newSumOfDpVals[subsetKeptID] += cnt * currDp;
                }
            }
            sumOfDpValsForSubsetKept = newSumOfDpVals;
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Finished in " << double(duration.count()) / double(1000 * 1000) << " seconds." << endl << endl;
    cout << "The maximum expected score for a single Yahtzee round is" << endl <<
         averageMaxEV[(1 << 13) - 1] << " points. This is lower than the true value as" << endl <<
         "the program doesn't consider multiple yahtzees (each worth 100 points), or" << endl <<
         "the +35 point bonus for scoring >= 63 points in the top section." << endl << endl;
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
    initAllRolls();
    calculateScores();
    calcExpectedValue();
    inputOutput();
}
