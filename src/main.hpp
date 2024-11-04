#pragma once

struct Challenge {
	bool active = false;

	int lives = 1000;
	int practiceRuns = 300;
	int skips = 300;
	int levels = 0;
	int coins = 0;

	int tempCoins = 0;
};

enum SkipRes {
	DontSkip,
	Skip,
	OutOfSkips,
	SkipAhead,
	Rebeat
};
