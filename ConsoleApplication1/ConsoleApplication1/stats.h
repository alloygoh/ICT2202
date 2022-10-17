#ifndef STATS_H
#define STATS_H

#include <windows.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>

double calculateMean(std::vector<int> v);
double calculateStandardDeviation(std::vector<int> v, double mean);
bool calculateTiming(DWORD now);

#endif