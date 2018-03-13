package org.sgdk.resourcemanager.ui.utils.vgm;

class ClassicEmu extends MusicEmu {
	protected int setSampleRate_(int rate) {
		buf.setSampleRate(rate, 1000 / bufLength);
		return rate;
	}

	public void startTrack(int track) {
		super.startTrack(track);
		buf.clear();
	}

	protected int play_(byte[] out, int count) {
		int pos = 0;
		while (true) {
			int n = buf.readSamples(out, pos, count);
			mixSamples(out, pos, n);

			pos += n;
			count -= n;
			if (count <= 0)
				break;

			if (trackEnded_) {
				java.util.Arrays.fill(out, pos, pos + count, (byte) 0);
				break;
			}

			int clocks = runMsec(bufLength);
			buf.endFrame(clocks);
		}
		return pos;
	}

	protected final int countSamples(int time) {
		return buf.countSamples(time);
	}

	protected void mixSamples(byte[] out, int offset, int count) {
		// derived class can override and mix its own samples here
	}

	// internal

	static final int bufLength = 32;
	protected StereoBuffer buf = new StereoBuffer();

	protected void setClockRate(int rate) {
		buf.setClockRate(rate);
	}

	// Subclass should run here for at most clockCount and return actual
	// number of clocks emulated (can be less)
	protected int runClocks(int clockCount) {
		return 0;
	}

	// Subclass can also get number of msec to run, and return number of clocks
	// emulated
	protected int runMsec(int msec) {
		return runClocks(buf.clockRate() >> 5);
	}
}
