#include <iostream>
#include <algorithm>
#include <vector>
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

int probOfRoll[252];
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
		++probOfRoll[getRollId(roll)];
	}
	int sum = 0;
	for(int i = 0; i < (int)allRollsIndistinguishable.size(); ++i) {
		//probOfRoll[i] /= pow6[5];
		sum += probOfRoll[i];
	}
	cout << "sum (should be 6^5): " << sum << " "<< pow6[5] << endl;
}

double prob[252][252];

//This calculates prob[i][j] = starting with hand ID = i, this is the
//probability of getting to hand ID = j with one re-roll
//0 <= i < 252
//0 <= j < 252
//0 <= k < 2
void calcRollProbs() {
	for(int startRoll = 0; startRoll < (int)allRollsIndistinguishable.size(); ++startRoll) {
		for(int subsetRerolled = 0; subsetRerolled < (1<<5); ++subsetRerolled) {
			int bits = __builtin_popcount(subsetRerolled);
			vector<int> numberOfways(allRollsIndistinguishable.size(),0);
			for(int id = 0; id < pow6[bits]; ++id) {
				vector<int> roll = allRollsIndistinguishable[startRoll];
				int ptr = 0;
				for(int die = 0; die < 5; ++die) {
					if(subsetRerolled&(1<<die)) {
						roll[die] = allRollsDistinguishable[id][ptr++];
					}
				}
				++numberOfways[getRollId(roll)];
			}
			for(int endRoll = 0; endRoll < (int)allRollsIndistinguishable.size(); ++endRoll) {
				prob[startRoll][endRoll] = max(prob[startRoll][endRoll], numberOfways[endRoll]/double(pow6[bits]));
			}
		}
	}
}

/*
   dp[subset scores][roll][num rerolls] = max expected score

   if rerolls == 0:
       try all un-filled scores, and max next dp val

   if rerolls == 1:
       take roll as is

	   re-roll:
	       try every subset of die to re-roll and
		   dp[subset scores][new Re-rolled][num rerolls - 1] = max(itself, currDp)




	after re-rolling 0,1,2 times, we get a probability distribution (prob[rollID] = probability of getting that roll)
	we would use the remaining scores to "guide" the probability distribution towards the high-scoring scores

	then to calculate the dp:
	dp[subset of scores] = max for each end-roll: (max over <=13 unfilled scores: prob[end roll] * scoreForRoll[end roll][score] + dp[new subset scores])
 */

double maxExpectedValue[1<<13][252][3];
double dp[1<<13];

void calcExpectedValue() {
	//reminder: This calculates prob[i][j][k] = starting with hand ID = i, this
	//is the probability of getting to hand ID = j with (k+1) re-rolls
	for(int subsetFilled = 1; subsetFilled < (1<<13); ++subsetFilled) {
		for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
			for(int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
				double &currDp = maxExpectedValue[subsetFilled][roll][numberRerolls];

				//take roll
				for(int scoreVal = 0; scoreVal < 13; ++scoreVal) {
					if(subsetFilled & (1<<scoreVal)) {
						const double nextScore = scoreForRoll[roll][scoreVal] + maxExpectedValue[subsetFilled ^ (1<<scoreVal)][roll][2];
						currDp = max(currDp, nextScore);
					}
				}

				//reroll
				if(numberRerolls > 0) {
					for(int endRoll = 0; endRoll < (int)allRollsIndistinguishable.size(); ++endRoll) {
						for(int scoreVal = 0; scoreVal < 13; ++scoreVal) {
							if(subsetFilled & (1<<scoreVal)) {
								const double nextScore = prob[roll][endRoll] * scoreForRoll[endRoll][scoreVal] + maxExpectedValue[subsetFilled ^ (1<<scoreVal)][endRoll][numberRerolls-1];
								currDp = max(currDp, nextScore);
							}
						}
					}
				}

				//currDp /= 252;
				dp[subsetFilled] = max(dp[subsetFilled], currDp);
			}
		}
		cout << "subsetFilled: " << subsetFilled << " dp: " << dp[subsetFilled] << endl;
	}
}

int main() {
	cout << setprecision(5) << fixed;
	initAllRolls();
	calculateScores();
	cout << "after calculating scores" << endl;
	calcRollProbs();
	cout << "before" << endl;
	calcExpectedValue();
	cout << "after" << endl;

	cout << setprecision(5) << fixed;
	cout << "max expected value of yahtzee is: " << dp[(1<<(13))-1] << endl;
}
