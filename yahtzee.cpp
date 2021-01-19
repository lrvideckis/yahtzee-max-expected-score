#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <ctime>
#include <cassert>
#include <complex>
#include <string>
#include <cstring>
#include <chrono>
#include <random>
#include <bitset>
using namespace std;

//TODO: handle multiple yahtzee's
//TODO: handle +35 points if score of first part is >= 63
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

vector<vector<int>> allRolls;

//scoreForRoll[i][j] = the score for roll with rollID=i if it counds for
//scores::x s.t. scores::x==j
void calculateScores() {
	for(int rollId = 0; rollId < (int)allRolls.size(); ++rollId) {
		const vector<int> &roll = allRolls[rollId];
		vector<int> cnts(7,0);
		int mxCnt = 0;
		int sum = 0;
		for(int val : roll) {
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
		if(mxCnt >= 3) {
			scoreForRoll[rollId][scores::threeOfAKind] = sum;
		}
		if(mxCnt >= 4) {
			scoreForRoll[rollId][scores::fourOfAKind] = sum;
		}
		if(mxCnt >= 5) {
			scoreForRoll[rollId][scores::yahtzee] = 50;
		}

		bool seenCnt3 = false, seenCnt2 = false;
		for(int cnt : cnts) {
			seenCnt2 |= (cnt == 2);
			seenCnt3 |= (cnt == 3);
		}
		if(seenCnt2 && seenCnt3) {
			scoreForRoll[rollId][scores::fullHouse] = sum;
		}

		for(int straightLen = 4; straightLen <= 5; ++straightLen) {
			for(int startVal = 1; startVal + straightLen - 1 <= 6; ++startVal) {
				bool hasAll = true;
				for(int val = startVal, len = straightLen; len--; ++val) {
					if(cnts[val] == 0) {
						hasAll = false;
						break;
					}
				}
				if(hasAll) {
					scoreForRoll[rollId][scores::smallStraight] = 30;
					if(straightLen == 5) {
						scoreForRoll[rollId][scores::largeStraight] = 40;
					}
				}
			}
		}
	}
}

int getRollId(vector<int> roll) {
	assert(roll.size() == 5);
	for(int i = 0; i < (int)roll.size(); ++i) {
		assert(1 <= roll[i] && roll[i] <= 6);
	}
	sort(roll.begin(), roll.end());
	auto it = lower_bound(allRolls.begin(), allRolls.end(), roll);
	assert(*it == roll);
	return lower_bound(allRolls.begin(), allRolls.end(), roll) - allRolls.begin();
}

void initAllRolls() {
	for(int a = 1; a <= 6; ++a) {
		for(int b = 1; b <= 6; ++b) {
			for(int c = 1; c <= 6; ++c) {
				for(int d = 1; d <= 6; ++d) {
					for(int e = 1; e <= 6; ++e) {
						vector<int> roll = {a,b,c,d,e};
						sort(roll.begin(), roll.end());
						allRolls.push_back(roll);
					}
				}
			}
		}
	}
	sort(allRolls.begin(), allRolls.end());
	allRolls.erase(unique(allRolls.begin(), allRolls.end()), allRolls.end());
	assert(allRolls.size() == 252);
}

double probOfRoll[252];
double prob[252][252][2];

//This calculates prob[i][j][k] = starting with hand ID = i, this is the
//probability of getting to hand ID = j with (k+1) re-rolls
//0 <= i < 252
//0 <= j < 252
//0 <= k < 2
void calcjRollProbs() {
	vector<vector<int>> allRollsDistinguishable;
	for(int a = 1; a <= 6; ++a) {
		for(int b = 1; b <= 6; ++b) {
			for(int c = 1; c <= 6; ++c) {
				for(int d = 1; d <= 6; ++d) {
					for(int e = 1; e <= 6; ++e) {
						allRollsDistinguishable.push_back({a,b,c,d,e});
						++probOfRoll[getRollId({a,b,c,d,e})];
					}
				}
			}
		}
	}


	vector<int> pow6(7,1);
	for(int i = 1; i < (int)pow6.size(); ++i) {
		pow6[i] = 6 * pow6[i-1];
	}

	for(int roll = 0; roll < (int)allRolls.size(); ++roll) {
		probOfRoll[roll] /= pow6[5];
	}

	for(int startRoll = 0; startRoll < (int)allRolls.size(); ++startRoll) {
		for(int subsetRerolled = 0; subsetRerolled < (1<<5); ++subsetRerolled) {
			int bits = __builtin_popcount(subsetRerolled);
			vector<int> numberOfways(allRolls.size(),0);
			for(int id = 0; id < pow6[bits]; ++id) {
				vector<int> roll = allRolls[startRoll];
				int ptr = 4;
				for(int die = 0; die < 5; ++die) {
					if(subsetRerolled&(1<<die)) {
						roll[die] = allRollsDistinguishable[id][ptr--];
					}
				}
				++numberOfways[getRollId(roll)];
			}
			for(int endRoll = 0; endRoll < (int)allRolls.size(); ++endRoll) {
				prob[startRoll][endRoll][0] = max(prob[startRoll][endRoll][0], numberOfways[endRoll]/double(pow6[bits]));
			}
		}
	}

	for(int startRoll = 0; startRoll < (int)allRolls.size(); ++startRoll) {
		for(int endRoll = 0; endRoll < (int)allRolls.size(); ++endRoll) {
			for(int midRoll = 0; midRoll < (int)allRolls.size(); ++midRoll) {
				prob[startRoll][endRoll][1] = max(prob[startRoll][endRoll][1], prob[startRoll][midRoll][0] * prob[midRoll][endRoll][0]);
			}
		}
	}
}

double maxExpectedValue[1<<13];

void calcExpectedValue() {
	//reminder: This calculates prob[i][j][k] = starting with hand ID = i, this
	//is the probability of getting to hand ID = j with (k+1) re-rolls
	for(int subsetFilled = 0; subsetFilled < (1<<13); ++subsetFilled) {
		for(int startRoll = 0; startRoll < (int)allRolls.size(); ++startRoll) {
			//push-dp seems better here

		}
	}
}

int main() {
	calculateScores();
	initAllRolls();
	calcjRollProbs();
	calcExpectedValue();

	cout << setprecision(5) << fixed;
	cout << "max expected value of yahtzee is: " << maxExpectedValue[(1<<(13))-1] << endl;
}
