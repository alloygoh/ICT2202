#include "stats.h"
#include "utils.h"
#include <string>
#include <algorithm>

#define WINDOW_TIMEOUT 10000 // In milliseconds
time_t prevTime = 0;
std::vector<double> times;
BOOL seenOutlier = false;

double calculateMedian(std::vector<double> v) {
	size_t n = v.size() / 2;
	std::nth_element(v.begin(), v.begin()+n, v.end());
	return v[n];
}

double calculateMean(std::vector<double> v) {
	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	return sum / v.size();
}

double calculateStandardDeviation(std::vector<double> v, double mean) {
	double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	if (sq_sum <= 0) {
		// Detect overflow
		return STDEV_THRESHHOLD + 1;
	}
	MyOutputDebugStringW(L"[squared sum] %lf\n", sq_sum);
	return std::sqrt(sq_sum / v.size() - mean * mean);
}

void addSlidingWindowElement(std::vector<double>& v, double item) {
	v.push_back(item);
	if (v.size() > WINDOW_SIZE) {
		v.erase(v.begin());
	}
}

bool calculateTiming(DWORD now) {
	DWORD timeDiff = now - prevTime;
	if (prevTime <= 0) {
		prevTime = now;
		return true;
	}
	
	MyOutputDebugStringW(L"[timeprev] %lu\n", now);
	MyOutputDebugStringW(L"[timenow] %lu\n", prevTime);
	MyOutputDebugStringW(L"[timediff] %lu\n", timeDiff);

	if (!seenOutlier && times.size() && timeDiff > times.back() * 20) {
		seenOutlier = true;
	}
	else {
		addSlidingWindowElement(times, timeDiff);
	}

	prevTime = now;

	std::wstring temp = L"[sliding window] ";
	for (double t : times) {
		temp += std::to_wstring(t) + L" ";
	}
	temp += L"\n";
	MyOutputDebugStringW(temp.c_str());

	if (times.size() < 2) {
		return true;
	}

	double mean = calculateMean(times);
	double median = calculateMedian(times);
	double stdev = calculateStandardDeviation(times, mean);

	double cv = stdev / mean * 100;
	MyOutputDebugStringW(L"[median] %lf\n", median);
	MyOutputDebugStringW(L"[mean] %lf\n", mean);
	MyOutputDebugStringW(L"[stdev] %lf\n", stdev);
	MyOutputDebugStringW(L"[cv] %lf\n", cv);

	return stdev > STDEV_THRESHHOLD || mean > MEAN_THRESHHOLD;
}
