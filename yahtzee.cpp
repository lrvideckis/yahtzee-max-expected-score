#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>
#include <iomanip>
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

vector<vector<int>> allRollsIndistinguishable;
vector<vector<int>> allRollsDistinguishable;

//scoreForRoll[i][j] = the score for roll with rollID=i if it counds for
//scores::x s.t. scores::x==j
void calculateScores() {
	for(int rollId = 0; rollId < (int)allRollsIndistinguishable.size(); ++rollId) {
		const vector<int> &roll = allRollsIndistinguishable[rollId];
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
	auto it = lower_bound(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end(), roll);
	assert(*it == roll);
	return lower_bound(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end(), roll) - allRollsIndistinguishable.begin();
}

int numberOfRoll[252];
int pow6[7];

void initAllRolls() {
	pow6[0] = 1;
	for(int i = 1; i < 7; ++i) {
		pow6[i] = 6 * pow6[i-1];
	}
	for(int rollVal = 0; rollVal < pow6[5]; ++rollVal) {
		int temp = rollVal;
		vector<int> roll(5);
		for(int i = 0; i < 5; ++i) {
			roll[i] = temp%6 + 1;
			temp /= 6;
		}
		allRollsDistinguishable.push_back(roll);
		sort(roll.begin(), roll.end());
		allRollsIndistinguishable.push_back(roll);
	}

	sort(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end());
	allRollsIndistinguishable.erase(unique(allRollsIndistinguishable.begin(), allRollsIndistinguishable.end()), allRollsIndistinguishable.end());
	assert(allRollsIndistinguishable.size() == 252);
	for(auto &roll : allRollsDistinguishable) {
		++numberOfRoll[getRollId(roll)];
	}
	int sum = 0;
	for(int i = 0; i < (int)allRollsIndistinguishable.size(); ++i) {
		sum += numberOfRoll[i];
	}
	cout << "sum (should be 6^5): " << sum << " "<< pow6[5] << endl;
}

/*
   dp[subset scores filled][num rerolls][roll] = max expected score

   if rerolls == 0:
       try all scores in "subset scores filled", and max with previous dp value

   if rerolls > 0:
       try all scores in "subset scores filled", and max with previous dp value

	   and

	   try every subset of die to re-roll and
	   dp[subset scores][num rerolls - 1][new Re-rolled] = max(itself, average of dp values)
 */

double maxEV[1<<13][3][252];
double averageMaxEV[1<<13];

vector<pair<int,int>> cntReroll[252][1<<5];
int tempCnt[252];

void calcExpectedValue() {

	bool debug = false;

	cout << "hi1" << endl;
	for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
		for(int subsetRerolled = 0; subsetRerolled < (1<<5); ++subsetRerolled) {
			for(int endRoll = 0; endRoll < 252; ++endRoll) {
				tempCnt[endRoll] = 0;
			}
			const int iters = pow6[__builtin_popcount(subsetRerolled)];
			for(int id = 0; id < iters; ++id) {
				vector<int> newRoll = allRollsIndistinguishable[roll];
				int ptr = 0;
				for(int die = 0; die < 5; ++die) {
					if(subsetRerolled&(1<<die)) {
						newRoll[die] = allRollsDistinguishable[id][ptr++];
					}
				}
				//here, we have a triplet: (start roll, subset die re-rolled, end roll)
				++tempCnt[getRollId(newRoll)];
			}
			for(int endRoll = 0; endRoll < 252; ++endRoll) {
				if(tempCnt[endRoll] > 0) {
					cntReroll[roll][subsetRerolled].push_back({tempCnt[endRoll], endRoll});
				}
			}
		}
	}
	cout << "hi2" << endl;

	const int testSubsetFilled =
		(1<<0) +
		(1<<2) +
		(1<<5) +
		(1<<6) +
		(1<<8) +
		(1<<10) +
		(1<<12);

	vector<int> testRoll = {4,1,6,1,6};
	int testRerolls = 2;
	cout << "testSubsetFilled: " << testSubsetFilled << endl;

	for(int subsetFilled = 1; subsetFilled < (1<<13); ++subsetFilled) {
		for(int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
			for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
				if(debug) {
					cout << "subsetFilled: " << subsetFilled << endl;
					cout << "here, rollVector: ";
					for(int val : allRollsIndistinguishable[roll]) cout << val << " ";
					cout << endl;
					cout << "numberRerolls: " << numberRerolls << endl;
				}


				double &currDp = maxEV[subsetFilled][numberRerolls][roll];

				//take roll
				for(int scoreVal = 0; scoreVal < 13; ++scoreVal) {
					if(subsetFilled & (1<<scoreVal)) {
						const double nextScore = scoreForRoll[roll][scoreVal] + averageMaxEV[subsetFilled ^ (1<<scoreVal)];
						currDp = max(currDp, nextScore);
					}
				}

				//re-roll
				if(numberRerolls > 0) {
					//for each subset of die that you can re-roll
					for(int subsetRerolled = 0; subsetRerolled < (1<<5); ++subsetRerolled) {
						//find average of expected values
						double sum = 0;
						const int bits = __builtin_popcount(subsetRerolled);
						for(const auto &p : cntReroll[roll][subsetRerolled]) {
							sum += p.first * maxEV[subsetFilled][numberRerolls-1][p.second];
						}
						currDp = max(currDp, sum / double(pow6[bits]));
					}
				}
				if(debug) {
					cout << "dp: " << currDp << endl;
					cout << endl << endl;
				}
				if(subsetFilled == testSubsetFilled && numberRerolls == testRerolls && roll == getRollId(testRoll)) {
					cout << "asdfasdfsaldkfjsldkfjasdlfjasdfj: " << currDp << endl;
				}

				if(numberRerolls == 2) {
					averageMaxEV[subsetFilled] += numberOfRoll[roll] * currDp / double(pow6[5]);
				}
			}
		}
		//if(debug) {
		cout << "subset values filled: " << subsetFilled << " averageMaxEV: " << averageMaxEV[subsetFilled] << endl;
		//}
	}
}

int main() {
	cout << setprecision(5) << fixed;
	initAllRolls();
	calculateScores();
	cout << "after calculating scores" << endl;
	cout << "before" << endl;
	calcExpectedValue();
	cout << "after" << endl;
}
