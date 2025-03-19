struct {
	struct timespec startTimeStamp;
	float64 elapsed;
	bool isRunning; // for debug
} stopwatch;

void LinuxTimeInit() {
	stopwatch = {};
}

void LinuxTimeStart() {
	ASSERT(!stopwatch.isRunning);
	clock_gettime(CLOCK_MONOTONIC, &stopwatch.startTimeStamp);                 
	stopwatch.isRunning = true;
}

float64 ToMilliseconds(struct timespec start, struct timespec end) {
	return (float64)((end.tv_sec - start.tv_sec)*1000) +  (float64)((end.tv_nsec - start.tv_nsec)/1000);
}

float64 LinuxTimePause() {
	ASSERT(stopwatch.isRunning);
	struct timespec endTimeStamp;
	clock_gettime(CLOCK_MONOTONIC, &endTimeStamp);                 
	stopwatch.elapsed += ToMilliseconds(stopwatch.startTimeStamp, endTimeStamp);
	stopwatch.isRunning = false;
	return stopwatch.elapsed;
}

float64 LinuxTimeStop() {
	ASSERT(stopwatch.isRunning);
	struct timespec endTimeStamp;
	clock_gettime(CLOCK_MONOTONIC, &endTimeStamp);                 
	float64 elapsed = stopwatch.elapsed + ToMilliseconds(stopwatch.startTimeStamp, endTimeStamp);
	stopwatch.elapsed = 0.0;
	stopwatch.isRunning = false;
	return elapsed;
}

float64 LinuxTimeRestart() {
	ASSERT(stopwatch.isRunning);
	struct timespec endTimeStamp;
	clock_gettime(CLOCK_MONOTONIC, &endTimeStamp);              
	float64 elapsed = stopwatch.elapsed + ToMilliseconds(stopwatch.startTimeStamp, endTimeStamp);
	stopwatch.elapsed = 0.0;
	stopwatch.startTimeStamp = endTimeStamp;

	return elapsed;
}