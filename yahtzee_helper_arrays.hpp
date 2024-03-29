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
//no 2 subsets will leave the same set of die remaining
//
//for example, for the roll "1 1 1 1 2", here are all 9 possible re-rolls:
//re-rolling 1 die
//? 1 1 1 2
//1 1 1 1 ?
//re-rolling 2 die
//? 1 1 1 ?
//? ? 1 1 2
//re-rolling 3 die
//? ? 1 1 ?
//? ? ? 1 2
//re-rolling 4 die
//? ? ? 1 ?
//? ? ? ? 2
//re-rolling 5 die
//? ? ? ? ?
//
//and for example, we consider the following the same, as they leave the same set of die remaining
//? 1 1 1 2
//1 ? 1 1 2
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
    for (int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
        for (int subsetRerolled : distinctSubsetsForReroll[roll]) {
            int sum = 0;
            for (auto [cnt, _] : cntReroll[roll][subsetRerolled])
                sum += cnt;
            assert(sum == pow6[__builtin_popcount(subsetRerolled)]);
        }
    }
}
