package org.sgdk.resourcemanager.ui.utils.vgm;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

class MusicEmu {
	// enables performance-intensive assertions
	protected static final boolean debug = false;
	private static final Logger logger = LogManager.getLogger("UILogger");

	public MusicEmu() {
		trackCount_ = 0;
		trackEnded_ = true;
		currentTrack_ = 0;
	}

	// Requests change of sample rate and returns sample rate used, which might be
	// different
	public final int setSampleRate(int rate) {
		return sampleRate_ = setSampleRate_(rate);
	}

	public final int sampleRate() {
		return sampleRate_;
	}

	// Loads music file into emulator. Might keep reference to data.
	public void loadFile(byte[] data) {
		trackEnded_ = true;
		currentTrack_ = 0;
		currentTime_ = 0;
		trackCount_ = loadFile_(data);
	}

	// Number of tracks
	public final int trackCount() {
		return trackCount_;
	}

	// Starts track, where 0 is first track
	public void startTrack(int track) {
		if (track < 0 || track > trackCount_)
			error("Invalid track");

		trackEnded_ = false;
		currentTrack_ = track;
		currentTime_ = 0;
		fadeStart = 0x40000000; // far into the future
		fadeStep = 1;
	}

	// Currently started track
	public final int currentTrack() {
		return currentTrack_;
	}

	// Generates at most count samples into out and returns
	// number of samples written. If track has ended, fills
	// buffer with silence.
	public final int play(byte[] out, int count) {
		if (!trackEnded_) {
			count = play_(out, count);
			if ((currentTime_ += count >> 1) > fadeStart)
				applyFade(out, count);
		} else {
			java.util.Arrays.fill(out, 0, count * 2, (byte) 0);
		}
		return count;
	}

	// Sets fade start and length, in seconds. Must be set after call to
	// startTrack().
	public final void setFade(int start, int length) {
		fadeStart = sampleRate_ * start;
		fadeStep = sampleRate_ * length / (fadeBlockSize * fadeShift);
		if (fadeStep < 1)
			fadeStep = 1;
	}

	// Number of seconds current track has been played
	public final int currentTime() {
		return currentTime_ / sampleRate_;
	}

	// True if track has reached end or setFade()'s fade has finished
	public final boolean trackEnded() {
		return trackEnded_;
	}

	// protected

	// must be defined in derived class
	protected int setSampleRate_(int rate) {
		return rate;
	}

	protected int loadFile_(byte[] in) {
		return 0;
	}

	protected int play_(byte[] out, int count) {
		return 0;
	}

	// Reports error string as exception
	protected void error(String str) {
		throw new Error(str);
	}

	// Sets end of track flag and stops emulating file
	protected void setTrackEnded() {
		trackEnded_ = true;
	}

	// Stops track and notes emulation error
	protected void logError() {
		if (!trackEnded_) {
			trackEnded_ = true;
			logger.info("emulation error");
		}
	}

	// Reads 16 bit little endian int starting at in [pos]
	protected static int getLE16(byte[] in, int pos) {
		return (in[pos] & 0xFF) | (in[pos + 1] & 0xFF) << 8;
	}

	// Reads 32 bit little endian int starting at in [pos]
	protected static int getLE32(byte[] in, int pos) {
		return (in[pos] & 0xFF) | (in[pos + 1] & 0xFF) << 8 | (in[pos + 2] & 0xFF) << 16 | (in[pos + 3] & 0xFF) << 24;
	}

	// True if first bytes of file match expected string
	protected static boolean isHeader(byte[] header, String expected) {
		for (int i = expected.length(); --i >= 0;)
			if ((byte) expected.charAt(i) != header[i])
				return false;
		return true;
	}

	// private

	int sampleRate_;
	int trackCount_;
	int currentTrack_;
	int currentTime_;
	int fadeStart;
	int fadeStep;
	boolean trackEnded_;

	static final int fadeBlockSize = 512;
	static final int fadeShift = 8; // fade ends with gain at 1.0 / (1 << fadeShift)

	// unit / pow( 2.0, (double) x / step )
	static int int_log(int x, int step, int unit) {
		int shift = x / step;
		int fraction = (x - shift * step) * unit / step;
		return ((unit - fraction) + (fraction >> 1)) >> shift;
	}

	static final int gainShift = 14;
	static final int gainUnit = 1 << gainShift;

	// Scales count big-endian 16-bit samples from io [pos*2] by gain/gainUnit
	static void scaleSamples(byte[] io, int pos, int count, int gain) {
		pos <<= 1;
		count = (count << 1) + pos;
		do {
			int s;
			io[pos + 1] = (byte) (s = ((io[pos] << 8 | (io[pos + 1] & 0xFF)) * gain) >> gainShift);
			io[pos] = (byte) (s >> 8);
		} while ((pos += 2) < count);
	}

	private void applyFade(byte[] io, int count) {
		// Apply successively smaller gains based on time since fade start
		for (int i = 0; i < count; i += fadeBlockSize) {
			// logarithmic progression
			int gain = int_log((currentTime_ + i - fadeStart) / fadeBlockSize, fadeStep, gainUnit);
			if (gain < (gainUnit >> fadeShift))
				setTrackEnded();

			int n = count - i;
			if (n > fadeBlockSize)
				n = fadeBlockSize;
			scaleSamples(io, i, n, gain);
		}
	}
}
