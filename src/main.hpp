#pragma once

struct Challenge {
	bool active = false;

	int lives = 100;
	int practiceRuns = 3;
	int skips = 3;
	int levels = 0;
	int coins = 0;

	int tempCoins = 0;
};

enum SkipRes {
	DontSkip,
	Skip,
	OutOfSkips,
	SkipAhead
};