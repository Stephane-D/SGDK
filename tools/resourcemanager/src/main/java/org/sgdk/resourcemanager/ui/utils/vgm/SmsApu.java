package org.sgdk.resourcemanager.ui.utils.vgm;

class SmsOsc {
	static final int masterVolume = (int) (0.40 * 65536 / 128);

	BlipBuffer output;
	int outputSelect;
	final BlipBuffer[] outputs = new BlipBuffer[4];
	int delay;
	int lastAmp;
	int volume;

	void reset() {
		delay = 0;
		lastAmp = 0;
		volume = 0;
		outputSelect = 3;
		output = outputs[outputSelect];
	}
}

final class SmsSquare extends SmsOsc {
	int period;
	int phase;

	void reset() {
		period = 0;
		phase = 0;
		super.reset();
	}

	void run(int time, int endTime) {
		final int period = this.period;

		int amp = volume;
		if (period > 128)
			amp = (amp * 2) & -phase;

		{
			int delta = amp - lastAmp;
			if (delta != 0) {
				lastAmp = amp;
				output.addDelta(time, delta * masterVolume);
			}
		}

		time += delay;
		delay = 0;
		if (period != 0) {
			if (time < endTime) {
				if (volume == 0 || period <= 128) // ignore 16kHz and higher
				{
					// keep calculating phase
					int count = (endTime - time + period - 1) / period;
					phase = (phase + count) & 1;
					time += count * period;
				} else {
					final BlipBuffer output = this.output;
					int delta = (amp - volume) * (2 * masterVolume);
					do {
						output.addDelta(time, delta = -delta);
					} while ((time += period) < endTime);

					phase = (delta >= 0 ? 1 : 0);
					lastAmp = volume * (phase << 1);
				}
			}
			delay = time - endTime;
		}
	}
}

final class SmsNoise extends SmsOsc {
	int shifter;
	int feedback;
	int select;

	void reset() {
		select = 0;
		shifter = 0x8000;
		feedback = 0x9000;
		super.reset();
	}

	void run(int time, int endTime, int period) {
		// TODO: probably also not zero-centered
		final BlipBuffer output = this.output;

		int amp = volume;
		if ((shifter & 1) != 0)
			amp = -amp;

		{
			int delta = amp - lastAmp;
			if (delta != 0) {
				lastAmp = amp;
				output.addDelta(time, delta * masterVolume);
			}
		}

		time += delay;
		if (volume == 0)
			time = endTime;

		if (time < endTime) {
			final int feedback = this.feedback;
			int shifter = this.shifter;
			int delta = amp * (2 * masterVolume);
			if ((period *= 2) == 0)
				period = 16;

			do {
				int changed = shifter + 1;
				shifter = (feedback & -(shifter & 1)) ^ (shifter >> 1);
				if ((changed & 2) != 0) // true if bits 0 and 1 differ
					output.addDelta(time, delta = -delta);
			} while ((time += period) < endTime);

			this.shifter = shifter;
			lastAmp = (delta < 0 ? -volume : volume);
		}
		delay = time - endTime;
	}
}

final class SmsApu {
	int lastTime;
	int latch;
	int noiseFeedback;
	int loopedFeedback;

	static final int oscCount = 4;
	final SmsSquare[] squares = new SmsSquare[3];
	final SmsNoise noise = new SmsNoise();
	final SmsOsc[] oscs = new SmsOsc[oscCount];

	static final int[] noisePeriods = { 0x100, 0x200, 0x400 };

	private void runUntil(int endTime) {
		if (endTime > lastTime) {
			// run oscillators
			for (int i = oscCount; --i >= 0;) {
				SmsOsc osc = oscs[i];
				if (osc.output != null) {
					if (i < 3) {
						squares[i].run(lastTime, endTime);
					} else {
						int period = squares[2].period;
						if (noise.select < 3)
							period = noisePeriods[noise.select];
						noise.run(lastTime, endTime, period);
					}
				}
			}

			lastTime = endTime;
		}
	}

	public SmsApu() {
		for (int i = 0; i < 3; i++)
			oscs[i] = squares[i] = new SmsSquare();
		oscs[3] = noise;
	}

	public void setOutput(BlipBuffer center, BlipBuffer left, BlipBuffer right) {
		for (int i = 0; i < oscCount; i++) {
			SmsOsc osc = oscs[i];
			osc.outputs[1] = right;
			osc.outputs[2] = left;
			osc.outputs[3] = center;
			osc.output = osc.outputs[osc.outputSelect];
		}
	}

	public void reset(int feedback, int noiseWidth) {
		lastTime = 0;
		latch = 0;

		// convert to "Galios configuration"
		loopedFeedback = 1 << (noiseWidth - 1);
		noiseFeedback = 0;
		while (--noiseWidth >= 0) {
			noiseFeedback = (noiseFeedback << 1) | (feedback & 1);
			feedback >>= 1;
		}

		squares[0].reset();
		squares[1].reset();
		squares[2].reset();
		noise.reset();
	}

	public void reset() {
		reset(0x0009, 16);
	}

	public void writeGG(int time, int data) {
		runUntil(time);

		for (int i = 0; i < oscCount; i++) {
			SmsOsc osc = oscs[i];
			int flags = data >> i;
			BlipBuffer oldOutput = osc.output;
			osc.outputSelect = (flags >> 3 & 2) | (flags & 1);
			osc.output = osc.outputs[osc.outputSelect];
			if (osc.output != oldOutput && osc.lastAmp != 0) {
				if (oldOutput != null)
					oldOutput.addDelta(time, -osc.lastAmp * SmsOsc.masterVolume);
				osc.lastAmp = 0;
			}
		}
	}

	static final int[] volumes = { 64, 50, 39, 31, 24, 19, 15, 12, 9, 7, 5, 4, 3, 2, 1, 0 };

	public void writeData(int time, int data) {
		runUntil(time);

		if ((data & 0x80) != 0)
			latch = data;

		int index = (latch >> 5) & 3;
		if ((latch & 0x10) != 0) {
			oscs[index].volume = volumes[data & 15];
		} else if (index < 3) {
			SmsSquare sq = squares[index];
			if ((data & 0x80) != 0)
				sq.period = (sq.period & 0xFF00) | (data << 4 & 0x00FF);
			else
				sq.period = (sq.period & 0x00FF) | (data << 8 & 0x3F00);
		} else {
			noise.select = data & 3;
			noise.feedback = ((data & 0x04) != 0) ? noiseFeedback : loopedFeedback;
			noise.shifter = 0x8000;
		}
	}

	public void endFrame(int endTime) {
		if (endTime > lastTime)
			runUntil(endTime);

		lastTime -= endTime;
	}
}
