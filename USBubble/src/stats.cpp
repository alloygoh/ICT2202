#include "stats.h"
#include "utils.h"
#include <string>
#include <algorithm>

#define WINDOW_TIMEOUT 10000 // In milliseconds
#define WINDOW_SIZE_MILLISECONDS 1000
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
		return THRESHOLD + 1;
	}
	MyOutputDebugStringW(L"[squared sum] %lf\n", sq_sum);
	return std::sqrt(sq_sum / v.size() - mean * mean);
}

// Remove elements in the sliding window that is too long ago
void removeStaleSlidingWindowElements(std::vector<double>& v) {
	// Ensure that the window size stays the same
	auto it = v.rbegin();
	double sum = 0;
	while (it != v.rend()) {
		double value = *it;
		sum += value;
		if (sum > WINDOW_SIZE_MILLISECONDS) {
			it = decltype(it)(v.erase(std::next(it).base()));
		} else {
			++it;
		}
	}
}

void addSlidingWindowElement(std::vector<double>& v, double item) {
	if (item > WINDOW_SIZE_MILLISECONDS) {
		v.clear();
		return;
	}

	v.push_back(item);
	removeStaleSlidingWindowElements(v);
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

	return stdev > THRESHOLD;
}
