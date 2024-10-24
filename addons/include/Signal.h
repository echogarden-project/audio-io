#include <mutex>
#include <condition_variable>

class Signal {
public:
	// Constructor
	Signal() : signalTransmitted(false) {}

	// Method to notify that the callback has completed
	void send() {
		std::lock_guard<std::mutex> lock(mtx);
		signalTransmitted = true;
		cv.notify_one();
	}

	// Method to wait for the callback to complete
	void wait() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return signalTransmitted; });
		signalTransmitted = false;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	bool signalTransmitted;
};
