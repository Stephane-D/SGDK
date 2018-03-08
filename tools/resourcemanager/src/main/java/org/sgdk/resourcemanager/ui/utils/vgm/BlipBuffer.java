package org.sgdk.resourcemanager.ui.utils.vgm;

final class BlipBuffer {
	static final boolean muchFaster = false; // speeds synthesis at a cost of quality

	BlipBuffer() {
		setVolume(1.0);
	}

	// Sets sample rate of output and changes buffer length to msec
	public void setSampleRate(int rate, int msec) {
		sampleRate = rate;
		buf = new int[(int) ((long) msec * rate / 1000) + 1024];
	}

	// Sets input clock rate. Must be set after sample rate.
	public void setClockRate(int rate) {
		clockRate_ = rate;
		factor = (int) (sampleRate / (float) clockRate_ * (1 << timeBits) + 0.5);
	}

	// Current clock rate
	public int clockRate() {
		return clockRate_;
	}

	// Removes all samples from buffer
	public void clear() {
		offset = 0;
		accum = 0;
		java.util.Arrays.fill(buf, 0, buf.length, 0);
	}

	// Sets overall volume, where 1.0 is normal
	public void setVolume(double v) {
		final int shift = 15;
		final int round = 1 << (shift - 1);

		volume = (int) ((1 << shift) * v + 0.5) & ~1;

		if (!muchFaster) {
			// build new set of kernels
			int[][] nk = new int[phaseCount + 1][];
			for (int i = nk.length; --i >= 0;)
				nk[i] = new int[halfWidth];

			// must be even since center kernel uses same half twice
			final int mul = volume;

			final int pc = phaseCount;
			for (int p = 17; --p >= 0;) {
				int remain = mul;
				for (int i = 8; --i >= 0;) {
					remain -= (nk[p][i] = (baseKernel[p * halfWidth + i] * mul + round) >> shift);
					remain -= (nk[pc - p][i] = (baseKernel[(pc - p) * halfWidth + i] * mul + round) >> shift);
				}
				nk[p][7] += remain; // each pair of kernel halves must total mul
			}

			// replace kernel atomically
			kernel = nk;
		}
	}

	// Adds delta at given time
	public void addDelta(int time, int delta) {
		final int[] buf = this.buf;
		final int phase = (time = time * factor + offset) >> (timeBits - phaseBits) & (phaseCount - 1);

		if (muchFaster) {
			final int right = ((delta *= volume) >> phaseBits) * phase;
			buf[time >>= timeBits] += delta - right;
			buf[time + 1] += right;
		} else {
			// TODO: use smaller kernel

			// left half
			int[] k = kernel[phase];
			buf[time >>= timeBits] += k[0] * delta;
			buf[time + 1] += k[1] * delta;
			buf[time + 2] += k[2] * delta;
			buf[time + 3] += k[3] * delta;
			buf[time + 4] += k[4] * delta;
			buf[time + 5] += k[5] * delta;
			buf[time + 6] += k[6] * delta;
			buf[time + 7] += k[7] * delta;

			// right half (mirrored version of a left half)
			k = kernel[phaseCount - phase];
			time += 8;
			buf[time] += k[7] * delta;
			buf[time + 1] += k[6] * delta;
			buf[time + 2] += k[5] * delta;
			buf[time + 3] += k[4] * delta;
			buf[time + 4] += k[3] * delta;
			buf[time + 5] += k[2] * delta;
			buf[time + 6] += k[1] * delta;
			buf[time + 7] += k[0] * delta;
		}
	}

	// Number of samples that would be available at time
	public int countSamples(int time) {
		int last_sample = (time * factor + offset) >> timeBits;
		int first_sample = offset >> timeBits;
		return last_sample - first_sample;
	}

	// Ends current time frame and makes samples available for reading
	public void endFrame(int time) {
		offset += time * factor;
		assert samplesAvail() < buf.length;
	}

	// Number of samples available to be read
	public int samplesAvail() {
		return offset >> timeBits;
	}

	// Reads at most count samples into out at offset pos*2 (2 bytes per sample)
	// and returns number of samples actually read.
	public int readSamples(byte[] out, int pos, int count) {
		final int avail = samplesAvail();
		if (count > avail)
			count = avail;

		if (count > 0) {
			// Integrate
			final int[] buf = this.buf;
			int accum = this.accum;
			pos <<= 1;
			int i = 0;
			do {
				int s = (accum += buf[i] - (accum >> 9)) >> 15;

				// clamp to 16 bits
				if ((short) s != s)
					s = (s >> 24) ^ 0x7FFF;

				// write as little-endian
				out[pos] = (byte) (s >> 8);
				out[pos + 1] = (byte) s;
				pos += 2;
			} while (++i < count);
			this.accum = accum;

			removeSamples(count);
		}
		return count;
	}

	// internal

	static final int timeBits = 16;
	static final int phaseBits = (muchFaster ? 8 : 5);
	static final int phaseCount = 1 << phaseBits;
	static final int halfWidth = 8;
	static final int stepWidth = halfWidth * 2;

	int factor;
	int offset;
	int[][] kernel;
	int accum;
	int[] buf;
	int sampleRate;
	int clockRate_;
	int volume;

	void removeSilence(int count) {
		offset -= count << timeBits;
		assert samplesAvail() >= 0;
	}

	void removeSamples(int count) {
		int remain = samplesAvail() - count + stepWidth;
		System.arraycopy(buf, count, buf, 0, remain);
		java.util.Arrays.fill(buf, remain, remain + count, 0);
		removeSilence(count);
	}

	// TODO: compute at run-time
	static final int[] baseKernel = { 10, -61, 284, -615, 1359, -1753, 5911, 22498, 14, -71, 295, -616, 1314, -1615,
			5259, 22472, 17, -80, 304, -611, 1260, -1468, 4626, 22402, 21, -88, 309, -603, 1200, -1313, 4015, 22285, 23,
			-94, 313, -589, 1134, -1151, 3426, 22122, 26, -100, 313, -572, 1063, -986, 2861, 21915, 28, -104, 312, -550,
			986, -818, 2322, 21663, 30, -108, 308, -525, 906, -648, 1810, 21369, 31, -110, 302, -497, 823, -478, 1326,
			21034, 33, -112, 295, -466, 737, -309, 871, 20657, 34, -112, 285, -433, 649, -143, 446, 20242, 34, -111,
			274, -397, 561, 19, 51, 19790, 34, -110, 261, -359, 472, 176, -313, 19302, 35, -108, 247, -320, 383, 327,
			-646, 18781, 34, -105, 232, -280, 296, 472, -948, 18230, 34, -101, 216, -240, 210, 608, -1219, 17651, 33,
			-97, 199, -199, 126, 736, -1459, 17045, 32, -92, 182, -158, 45, 855, -1668, 16413, 31, -86, 164, -117, -33,
			964, -1847, 15761, 30, -80, 145, -77, -107, 1063, -1996, 15091, 28, -74, 127, -38, -177, 1151, -2117, 14405,
			26, -67, 108, 0, -243, 1228, -2211, 13706, 24, -60, 90, 37, -304, 1294, -2277, 12996, 22, -53, 72, 72, -360,
			1349, -2318, 12278, 20, -46, 54, 105, -410, 1392, -2334, 11556, 18, -39, 37, 136, -455, 1425, -2327, 10831,
			15, -31, 21, 164, -495, 1446, -2298, 10107, 13, -24, 5, 191, -529, 1456, -2249, 9385, 10, -17, -10, 215,
			-557, 1456, -2182, 8669, 8, -10, -24, 236, -580, 1446, -2096, 7962, 5, -3, -37, 255, -597, 1426, -1996,
			7265, 3, 4, -50, 271, -608, 1397, -1881, 6580, 0, 10, -61, 284, -615, 1359, -1753, 5911, };
}

// Stereo sound buffer with center channel

final class StereoBuffer {
	private BlipBuffer[] bufs = new BlipBuffer[3];

	// Same behavior as in BlipBuffer unless noted

	public StereoBuffer() {
		for (int i = bufs.length; --i >= 0;)
			bufs[i] = new BlipBuffer();
	}

	public void setSampleRate(int rate, int msec) {
		for (int i = bufs.length; --i >= 0;)
			bufs[i].setSampleRate(rate, msec);
	}

	public void setClockRate(int rate) {
		for (int i = bufs.length; --i >= 0;)
			bufs[i].setClockRate(rate);
	}

	public int clockRate() {
		return bufs[0].clockRate();
	}

	public int countSamples(int time) {
		return bufs[0].countSamples(time);
	}

	public void clear() {
		for (int i = bufs.length; --i >= 0;)
			bufs[i].clear();
	}

	public void setVolume(double v) {
		for (int i = bufs.length; --i >= 0;)
			bufs[i].setVolume(v);
	}

	// The three channels that are mixed together
	// left output = left + center
	// right output = right + center
	public BlipBuffer center() {
		return bufs[2];
	}

	public BlipBuffer left() {
		return bufs[0];
	}

	public BlipBuffer right() {
		return bufs[1];
	}

	public void endFrame(int time) {
		for (int i = bufs.length; --i >= 0;)
			bufs[i].endFrame(time);
	}

	public int samplesAvail() {
		return bufs[2].samplesAvail() << 1;
	}

	// Output is in stereo, so count must always be a multiple of 2
	public int readSamples(byte[] out, int start, int count) {
		assert (count & 1) == 0;

		final int avail = samplesAvail();
		if (count > avail)
			count = avail;

		if ((count >>= 1) > 0) {
			// TODO: optimize for mono case

			// calculate center in place
			final int[] mono = bufs[2].buf;
			{
				int accum = bufs[2].accum;
				int i = 0;
				do {
					mono[i] = (accum += mono[i] - (accum >> 9));
				} while (++i < count);
				bufs[2].accum = accum;
			}

			// calculate left and right
			for (int ch = 2; --ch >= 0;) {
				// add right and output
				final int[] buf = bufs[ch].buf;
				int accum = bufs[ch].accum;
				int pos = (start + ch) << 1;
				int i = 0;
				do {
					int s = ((accum += buf[i] - (accum >> 9)) + mono[i]) >> 15;

					// clamp to 16 bits
					if ((short) s != s)
						s = (s >> 24) ^ 0x7FFF;

					// write as big endian
					out[pos] = (byte) (s >> 8);
					out[pos + 1] = (byte) s;
					pos += 4;
				} while (++i < count);
				bufs[ch].accum = accum;
			}

			for (int i = bufs.length; --i >= 0;)
				bufs[i].removeSamples(count);
		}
		return count << 1;
	}
}