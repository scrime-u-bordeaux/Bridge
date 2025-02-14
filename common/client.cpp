#include "dsp/ringbuffer.hpp"
#include "bridgeprotocol.hpp"

#include <stdarg.h>
#include <unistd.h>
#ifdef ARCH_WIN
	#include <winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netinet/tcp.h>
	#include <fcntl.h>
#endif

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>


using namespace rack;


void log(const char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[VCV Bridge] ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(args);
}


struct MidiMessage {
	uint8_t cmd;
	uint8_t data1;
	uint8_t data2;
};


struct BridgeClient {
	int server = -1;
	/** Whether the server is ready to accept public API send() calls */
	bool ready = false;
	/** Whether the client should stop attempting to reconnect */
	bool running = false;

	int port = 0;
	bool portDirty = false;
	float params[BRIDGE_NUM_PARAMS] = {};
	bool paramsDirty[BRIDGE_NUM_PARAMS] = {};
	int sampleRate = 44100;
	bool sampleRateDirty = false;
	std::vector<MidiMessage> midiQueue;

	std::thread runThread;
	std::recursive_mutex mutex;

	BridgeClient() {
		running = true;
		runThread = std::thread(&BridgeClient::run, this);
	}

	~BridgeClient() {
		running = false;
		runThread.join();
	}

	void run() {
		initialize();
		while (running) {
			// Wait before connecting or reconnecting
			std::this_thread::sleep_for(std::chrono::duration<double>(0.25));
			// Connect
			connect();
			if (server < 0)
				continue;
			log("client connected");
			welcome();
			ready = true;
			// Wait for server to disconnect
			while (running && ready) {
				std::this_thread::sleep_for(std::chrono::duration<double>(0.25));
			}
			disconnect();
			log("client disconnected");
		}
		log("client destroyed");
	}

	void initialize() {
		// Initialize sockets
#ifdef ARCH_WIN
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
			log("Could not initialize Winsock");
			return;
		}
#endif
		log("client initialized");
	}

	void connect() {
		// Get address
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(BRIDGE_PORT);
		addr.sin_addr.s_addr = inet_addr(BRIDGE_HOST);

		// Open socket
		server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server < 0) {
			log("socket() failed: error %d", errno);
			return;
		}

		// Avoid SIGPIPE
#ifdef ARCH_MAC
		int flag = 1;
		if (setsockopt(server, SOL_SOCKET, SO_NOSIGPIPE, &flag, sizeof(int))) {
			log("setsockopt() failed: error %d", errno);
			return;
		}
#endif

		// Connect socket
		if (::connect(server, (struct sockaddr*) &addr, sizeof(addr))) {
			log("connect() failed: error %d", errno);
			disconnect();
			return;
		}
	}

	void disconnect() {
		ready = false;
		if (server >= 0) {
			if (close(server)) {
				log("close() failed: error %d", errno);
			}
		}
		server = -1;
	}

	/** Returns true if successful */
	bool send(const void *buffer, int length) {
		if (length <= 0)
			return false;

#ifdef ARCH_LIN
		int flags = MSG_NOSIGNAL;
#else
		int flags = 0;
#endif
		ssize_t remaining = 0;
		while (remaining < length) {
			ssize_t actual = ::send(server, (const char*) buffer, length, flags);
			if (actual <= 0) {
				ready = false;
				return false;
			}
			remaining += actual;
		}
		return true;
	}

	template <typename T>
	bool send(T x) {
		return send(&x, sizeof(x));
	}

	/** Returns true if successful */
	bool recv(void *buffer, int length) {
		if (length <= 0)
			return false;

#ifdef ARCH_LIN
		int flags = MSG_NOSIGNAL;
#else
		int flags = 0;
#endif
		ssize_t remaining = 0;
		while (remaining < length) {
			ssize_t actual = ::recv(server, (char*) buffer + remaining, length - remaining, flags);
			if (actual <= 0) {
				ready = false;
				return false;
			}
			remaining += actual;
		}
		return true;
	}

	template <typename T>
	bool recv(T *x) {
		return recv(x, sizeof(*x));
	}

	void flush() {
		// Turn off Nagle
		int flag = 1;
		setsockopt(server, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
		// Turn on Nagle
		flag = 0;
		setsockopt(server, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
	}

	// Private API

	void welcome() {
		std::lock_guard<std::recursive_mutex> lock(mutex);
		send<uint32_t>(BRIDGE_HELLO);
		sendSetPort();
	}

	void sendSetPort() {
		std::lock_guard<std::recursive_mutex> lock(mutex);
		send<uint8_t>(PORT_SET_COMMAND);
		send<uint8_t>(port);
		for (int i = 0; i < BRIDGE_NUM_PARAMS; i++)
			sendSetParam(i);
		sendSetSampleRate();
	}

	void sendSetSampleRate() {
		std::lock_guard<std::recursive_mutex> lock(mutex);
		send<uint8_t>(AUDIO_SAMPLE_RATE_SET_COMMAND);
		send<uint32_t>(sampleRate);
	}

	void sendSetParam(int i) {
		std::lock_guard<std::recursive_mutex> lock(mutex);
		send<uint8_t>(MIDI_MESSAGE_COMMAND);
		uint8_t msg[3];
		msg[0] = 0xb0;
		msg[1] = i;
		msg[2] = roundf(params[i] * 0x7f);
		send(msg, 3);
	}

	void sendMidi() {
		std::lock_guard<std::recursive_mutex> lock(mutex);
		for (MidiMessage msg : midiQueue) {
			send<uint8_t>(MIDI_MESSAGE_COMMAND);
			send<MidiMessage>(msg);
		}
		midiQueue.clear();
	}

	// Public API

	void setPort(int port) {
		if (port == this->port)
			return;
		this->port = port;
		portDirty = true;
	}

	void setSampleRate(int sampleRate) {
		if (sampleRate == this->sampleRate)
			return;
		this->sampleRate = sampleRate;
		sampleRateDirty = true;
	}

	int getPort() {
		return port;
	}

	void setParam(int i, float param) {
		if (!(0 <= i && i < BRIDGE_NUM_PARAMS))
			return;
		if (params[i] == param)
			return;
		params[i] = param;
		paramsDirty[i] = true;
	}

	float getParam(int i) {
		if (0 <= i && i < BRIDGE_NUM_PARAMS)
			return params[i];
		else
			return 0.f;
	}

	void pushMidi(MidiMessage msg) {
		midiQueue.push_back(msg);
	}

	void pushClock() {
		MidiMessage msg;
		msg.cmd = 0xf8;
		msg.data1 = 0x00;
		msg.data2 = 0x00;
		pushMidi(msg);
	}

	void pushStart() {
		MidiMessage msg;
		msg.cmd = 0xfa;
		msg.data1 = 0x00;
		msg.data2 = 0x00;
		pushMidi(msg);
	}

	void pushContinue() {
		MidiMessage msg;
		msg.cmd = 0xfb;
		msg.data1 = 0x00;
		msg.data2 = 0x00;
		pushMidi(msg);
	}

	void pushStop() {
		MidiMessage msg;
		msg.cmd = 0xfc;
		msg.data1 = 0x00;
		msg.data2 = 0x00;
		pushMidi(msg);
	}

	void processStream(const float *input, float *output, int frames) {
		// Zero output buffer
		memset(output, 0, BRIDGE_OUTPUTS * frames * sizeof(float));
		if (!ready) {
			return;
		}

		std::lock_guard<std::recursive_mutex> lock(mutex);
		// Send commands if they need to be updated (are dirty)
		if (portDirty) {
			portDirty = false;
			sendSetPort();
		}
		if (sampleRateDirty) {
			sampleRateDirty = false;
			sendSetSampleRate();
		}
		for (int i = 0; i < BRIDGE_NUM_PARAMS; i++) {
			if (paramsDirty[i]) {
				paramsDirty[i] = false;
				sendSetParam(i);
			}
		}

		sendMidi();

		// Send audio
		send<uint8_t>(AUDIO_PROCESS_COMMAND);
		send<uint32_t>(frames);

		if (!send(input, BRIDGE_INPUTS * frames * sizeof(float))) {
			log("send() failed");
			return;
		}
		flush();

		// Receive audio
		if (!recv(output, BRIDGE_OUTPUTS * frames * sizeof(float))) {
			log("recv() failed");
			return;
		}
	}
};
