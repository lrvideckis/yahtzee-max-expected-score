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

vector<vector<int>> allRolls;

int getRollId(const vector<int> &roll) {
	assert(roll.size() == 5);
	for(int i = 1; i < (int)roll.size(); ++i) {
		assert(roll[i-1] <= roll[i]);
	}
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
}

int main() {
	initAllRolls();
}
