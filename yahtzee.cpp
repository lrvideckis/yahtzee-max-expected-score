#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>
#include <iomanip>
#include <chrono>
using namespace std;
using namespace std::chrono;

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
			scoreForRoll[rollId][scores::fullHouse] = 25;
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
}

//maxEV[subset scores filled][num rerolls][roll] = max expected score
double maxEV[1<<13][3][252];
double averageMaxEV[1<<13];
vector<int> distinctSubsetsForReroll[252];
vector<pair<int,int>> cntReroll[252][1<<5];
int tempCnt[252];
vector<int> tempRolls[7776];

struct Move {
	int subsetReroll, scoreTaken;
	double evForMove;
};

bool operator<(const Move &x, const Move &y) {
	return x.evForMove > y.evForMove;
}

vector<Move> transitions[1<<13][3][252];

void calcExpectedValue() {
	for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
		map<vector<int>,int> keptDieToSubset;
		for(int subsetRerolled = 1; subsetRerolled < (1<<5); ++subsetRerolled) {
			vector<int> keptDie;
			for(int die = 0; die < 5; ++die) {
				if((subsetRerolled&(1<<die)) == 0) {
					keptDie.push_back(allRollsIndistinguishable[roll][die]);
				}
			}
			sort(keptDie.begin(), keptDie.end());
			keptDieToSubset[keptDie] = subsetRerolled;
		}
		for(auto &p : keptDieToSubset) {
			distinctSubsetsForReroll[roll].push_back(p.second);
		}
	}

	cout << "hi1" << endl;
	auto start = high_resolution_clock::now();
	for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
		for(int subsetRerolled : distinctSubsetsForReroll[roll]) {
			const int iters = pow6[__builtin_popcount(subsetRerolled)];
			int sz = 0;
			map<vector<int>, int> cnts;
			for(int id = 0; id < iters; ++id) {
				vector<int> newRoll = allRollsIndistinguishable[roll];
				int ptr = 0;
				for(int die = 0; die < 5; ++die) {
					if(subsetRerolled&(1<<die)) {
						newRoll[die] = allRollsDistinguishable[id][ptr++];
					}
				}
				//here, we have a triplet: (start roll, subset die re-rolled, end roll)
				sort(newRoll.begin(), newRoll.end());
				++cnts[newRoll];
				tempRolls[sz++] = newRoll;
			}
			cout << "start roll: ";
			for(int val : allRollsIndistinguishable[roll]) cout << val << " ";
			cout << " subset: ";
			for(int die = 0; die < 5; ++die) {
				if(subsetRerolled&(1<<die)) {
					cout << '1';
				} else cout << '0';
			}
			cout << endl;
			cout << "num distinct: " << cnts.size() << endl;
			sort(tempRolls, tempRolls + sz);
			for(int endRoll = 0; endRoll < 252; ++endRoll) {
				tempCnt[endRoll] = 0;
			}
			int ptr = 0;
			for(int i = 0; i < sz; ++i) {
				while(ptr < (int)allRollsIndistinguishable.size() && allRollsIndistinguishable[ptr] < tempRolls[i]) ++ptr;
				assert(tempRolls[i] == allRollsIndistinguishable[ptr]);
				++tempCnt[ptr];
			}
			for(int endRoll = 0; endRoll < 252; ++endRoll) {
				if(tempCnt[endRoll] > 0) {
					cntReroll[roll][subsetRerolled].push_back({tempCnt[endRoll], endRoll});
				}
			}
		}
	}
	cout << "hi2" << endl;
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	cout << "duration in seconds: " << duration.count()/double(1000 * 1000) << endl;

	for(int subsetFilled = 1; subsetFilled < (1<<13); ++subsetFilled) {
		for(int numberRerolls = 0; numberRerolls <= 2; ++numberRerolls) {
			for(int roll = 0; roll < (int)allRollsIndistinguishable.size(); ++roll) {
				double &currDp = maxEV[subsetFilled][numberRerolls][roll];
				vector<Move> &currTransitions = transitions[subsetFilled][numberRerolls][roll];

				//take roll
				for(int scoreVal = 0; scoreVal < 13; ++scoreVal) {
					if(subsetFilled & (1<<scoreVal)) {
						const double nextScore = scoreForRoll[roll][scoreVal] + averageMaxEV[subsetFilled ^ (1<<scoreVal)];
						currTransitions.push_back({-1, scoreVal, nextScore});
						currDp = max(currDp, nextScore);
					}
				}

				//re-roll
				if(numberRerolls > 0) {
					//for each subset of die that you can re-roll
					for(int subsetRerolled : distinctSubsetsForReroll[roll]) {
						//find average of expected values
						double sum = 0;
						for(const auto &p : cntReroll[roll][subsetRerolled]) {
							sum += p.first * maxEV[subsetFilled][numberRerolls-1][p.second];
						}
						const double nextScore = sum / double(pow6[__builtin_popcount(subsetRerolled)]);
						currTransitions.push_back({subsetRerolled,-1,nextScore});
						currDp = max(currDp, nextScore);
					}
				}

				if(numberRerolls == 2) {
					averageMaxEV[subsetFilled] += numberOfRoll[roll] * currDp / double(pow6[5]);
				}
			}
		}
		cout << "subsetFilled: " << subsetFilled << " averageMaxEV: " << averageMaxEV[subsetFilled] << endl;
	}
}

bool cmpSeconds(const pair<int,int> &x, const pair<int,int> &y) {
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
	while(true) {
		int sumFilledScores = 0;
		int subsetFilled = 0;
		for(int i = 0; i < 13; ++i) {
			cout << "enter in score for " << scoreDescription[i] << " (or -1 for not filled yet): ";
			int score;
			cin >> score;
			if(score >= 0) {
				sumFilledScores += score;
			} else {
				subsetFilled += (1<<i);
			}
		}
		cout << "enter in dice roll (ex: 2 4 6 3 2): " << endl;
		vector<pair<int,int>> roll(5);
		vector<int> rollInt(5);
		for(int i = 0; i < 5; ++i) {
			cin >> roll[i].first;
			roll[i].second = i;
			rollInt[i] = roll[i].first;
		}
		cout << "number of re-rolls left (0, 1, or 2): " << endl;
		int rerolls;
		cin >> rerolls;
		cout << "max expected value: " << double(sumFilledScores) + maxEV[subsetFilled][rerolls][getRollId(rollInt)] << endl << endl;

		vector<Move> &currTransitions = transitions[subsetFilled][rerolls][getRollId(rollInt)];
		sort(currTransitions.begin(), currTransitions.end());

		cout << "options are:" << endl;
		for(const auto &currMove : currTransitions) {
			if(currMove.subsetReroll == -1) {//score roll
				cout << "Score roll as: " << scoreDescription[currMove.scoreTaken] << " giving " << double(sumFilledScores) + currMove.evForMove << " expected points." << endl;
			} else {//re roll
				assert(currMove.scoreTaken == -1);
				cout << "Keep die:";
				sort(roll.begin(), roll.end());
				vector<bool> reroll(5,false);
				for(int die = 0; die < 5; ++die) {
					if(currMove.subsetReroll & (1<<die)) {
						reroll[roll[die].second] = true;
					}
				}
				sort(roll.begin(), roll.end(), cmpSeconds);
				for(int die = 0; die < 5; ++die) {
					if(reroll[die]) {
						cout << " _";
					} else {
						cout << " " << roll[die].first;
					}
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
