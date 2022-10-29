#ifndef STATS_H
#define STATS_H
#define STDEV_THRESHHOLD 10
#define MEAN_THRESHHOLD 15
#define WINDOW_SIZE 10

#include <windows.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>

bool calculateTiming(DWORD now);

#endif
