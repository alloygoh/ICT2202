#include "stats.h"
#include <string>

time_t prevTime = 0;
std::vector<int> times;
BOOL seenOutlier = false;

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
	if (times.size() > 5) {
		times.erase(times.begin());
		OutputDebugStringA("[set end]\n");
	}
	std::string temp = "[timenow] " + std::to_string(now) + "\n";
	temp += "[timeprev] " + std::to_string(prevTime) + "\n";
	temp += "[timediff] " + std::to_string(timeDiff) + "\n";
	if (prevTime > 0) {
		if (!seenOutlier && times.size() && timeDiff > times.back() * 20) {
			seenOutlier = true;
		}
		else {
			times.push_back(timeDiff);
		}
	}
	//times.push_back(timeDiff);

	prevTime = now;

	if (times.size() < 2) {
		OutputDebugStringA(temp.c_str());
		return true;
	}

	//if (times.size() > WINDOW_SIZE) {
	//	times.erase(times.begin());
	//	temp += "[set end]\n";
	//}
	double mean = calculateMean(times);

	double stdev = calculateStandardDeviation(times, mean);

	// double cv = stdev / mean * 100;
	std::cout << "stdev is " << stdev << std::endl;
	temp += "[stdev] " + std::to_string(stdev) + "\n";
	temp += "[cv] " + std::to_string(stdev/mean) + "\n";
	
	OutputDebugStringA(temp.c_str());

	return stdev > THRESHOLD;
}
