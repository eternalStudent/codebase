#define TICKS_PER_MILLISECOND	1000
#define FREQUENCY				((float64)(stopwatch.frequency.QuadPart))

struct {
	LARGE_INTEGER frequency;
	LARGE_INTEGER startTimeStamp;
	LONGLONG elapsed;
	BOOL isRunning; // for debug
} stopwatch;

void Win32TimeInit() {                       
	BOOL succeeded = QueryPerformanceFrequency(&stopwatch.frequency);            
	if (!succeeded) {
		LOG("failed querying performance counter");
	}  
}
 
void Win32TimeStart() {
	ASSERT(!stopwatch.isRunning);
	QueryPerformanceCounter(&stopwatch.startTimeStamp);                 
	stopwatch.isRunning = TRUE;
}
 
float64 Win32TimePause() {
	ASSERT(stopwatch.isRunning);
	LARGE_INTEGER endTimeStamp;
	QueryPerformanceCounter(&endTimeStamp);                 
	stopwatch.elapsed += endTimeStamp.QuadPart - stopwatch.startTimeStamp.QuadPart;
	stopwatch.isRunning = FALSE;

	LONGLONG elapsedMilliseconds = stopwatch.elapsed*TICKS_PER_MILLISECOND;
	return ((float64)elapsedMilliseconds)/FREQUENCY;
}
 
float64 Win32TimeStop() {
	ASSERT(stopwatch.isRunning);
	LONGLONG elapsedMilliseconds = stopwatch.elapsed*TICKS_PER_MILLISECOND;
	stopwatch.elapsed = 0;
	stopwatch.isRunning = FALSE;
	stopwatch.startTimeStamp.QuadPart = 0;

	return ((float64)elapsedMilliseconds)/FREQUENCY;
}
 
float64 Win32TimeRestart() {
	ASSERT(stopwatch.isRunning);
	LARGE_INTEGER endTimeStamp;
	QueryPerformanceCounter(&endTimeStamp);                 
	stopwatch.elapsed += endTimeStamp.QuadPart - stopwatch.startTimeStamp.QuadPart;
	LONGLONG elapsedMilliseconds = stopwatch.elapsed*TICKS_PER_MILLISECOND;
	stopwatch.elapsed = 0;
	stopwatch.startTimeStamp = endTimeStamp;

	return ((float64)elapsedMilliseconds)/FREQUENCY;
}