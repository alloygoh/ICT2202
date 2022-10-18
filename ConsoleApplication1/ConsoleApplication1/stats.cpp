#include "stats.h"

time_t prevTime = 0;
std::vector<int> times;

double calculateMean(std::vector<int> v) {
	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	return sum / v.size();
}

double calculateStandardDeviation(std::vector<int> v, double mean) {
	double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return std::sqrt(sq_sum / v.size() - mean * mean);
}

bool calculateTiming(DWORD now) {
	DWORD timeDiff = now - prevTime;

	if (prevTime > 0) {
		times.push_back(timeDiff);
	}

	prevTime = now;

	if (times.size() <= 1) {
		return true;
	}

	if (times.size() > WINDOW_SIZE) {
		times.erase(times.begin());
	}
	double mean = calculateMean(times);

	double stdev = calculateStandardDeviation(times, mean);

	// double cv = stdev / mean * 100;
	std::cout << "stdev is " << stdev << std::endl;

	return stdev > THRESHOLD;
}
