#include "header.h"

int THRESHHOLD = 50;
int WINDOW_SIZE = 3;
int prevTime = 0;
std::vector<int> times;

double calculateMean(std::vector<int> v){
	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	return sum / v.size();
}

double calculateStandardDeviation(std::vector<int> v, double mean) {
	double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return std::sqrt(sq_sum / v.size() - mean * mean);
}

bool calculateTiming(int startTime) {
	int timeDiff = startTime - prevTime;
	prevTime = startTime;
	std::cout << "TimeDiff is " << timeDiff << std::endl;

	times.push_back(timeDiff);
	if (times.size() <= 1) {
		return true;
	}

	if (times.size() > WINDOW_SIZE) {
		times.erase(times.begin());
	}
	double mean = calculateMean(times);

	double stdev = calculateStandardDeviation(times, mean);

	double cv = stdev / mean * 100;
	std::cout << "Mean is " << mean << std::endl;
	std::cout << "stdev is " << stdev << std::endl;
	std::cout << "CV is " << cv << std::endl;

	return cv > THRESHHOLD;
}
