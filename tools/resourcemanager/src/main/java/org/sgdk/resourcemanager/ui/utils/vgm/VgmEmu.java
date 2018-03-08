package org.sgdk.resourcemanager.ui.utils.vgm;

final class VgmEmu extends ClassicEmu {
	protected int loadFile_(byte[] data) {
		if (!isHeader(data, "Vgm "))
			error("Not a VGM file");

		// TODO: use custom noise taps if present

		// Data and loop
		this.data = data;
		loopBegin = getLE32(data, 28) + 28;
		if (loopBegin <= 28) {
			loopBegin = data.length;
		} else if (data[data.length - 1] != cmd_end) {
			data = DataReader.resize(data, data.length + 1);
			data[data.length - 1] = cmd_end;
		}

		// PSG clock rate
		int clockRate = getLE32(data, 0x0C);
		if (clockRate == 0)
			clockRate = 3579545;
		psgFactor = (int) ((float) psgTimeUnit / vgmRate * clockRate + 0.5);

		// FM clock rate
		fm_clock_rate = getLE32(data, 0x2C);
		fm = null;
		if (fm_clock_rate != 0) {
			fm = new YM2612();
			buf.setVolume(0.7);
			fm.init(fm_clock_rate, sampleRate());
		} else {
			buf.setVolume(1.0);
		}

		setClockRate(clockRate);
		apu.setOutput(buf.center(), buf.left(), buf.right());

		return 1;
	}

	// private

	static final int vgmRate = 44100;
	static final int psgTimeBits = 12;
	static final int psgTimeUnit = 1 << psgTimeBits;

	final SmsApu apu = new SmsApu();
	YM2612 fm;
	int fm_clock_rate;
	int pos;
	byte[] data;
	int delay;
	int psgFactor;
	int loopBegin;
	final int[] fm_buf_lr = new int[48000 / 10 * 2];
	int fm_pos;
	int dac_disabled; // -1 if disabled
	int pcm_data;
	int pcm_pos;
	int dac_amp;

	static final int cmd_gg_stereo = 0x4F;
	static final int cmd_psg = 0x50;
	static final int cmd_ym2612_port0 = 0x52;
	static final int cmd_ym2612_port1 = 0x53;
	static final int cmd_delay = 0x61;
	static final int cmd_delay_735 = 0x62;
	static final int cmd_delay_882 = 0x63;
	static final int cmd_end = 0x66;
	static final int cmd_data_block = 0x67;
	static final int cmd_short_delay = 0x70;
	static final int cmd_pcm_delay = 0x80;
	static final int cmd_pcm_seek = 0xE0;
	static final int ym2612_dac_port = 0x2A;
	static final int pcm_block_type = 0x00;

	public void startTrack(int track) {
		super.startTrack(track);

		pos = 0x40;
		delay = 0;
		pcm_data = pos;
		pcm_pos = pos;
		dac_amp = -1;

		apu.reset();
		if (fm != null)
			fm.reset();
	}

	private int toPSGTime(int vgmTime) {
		return (vgmTime * psgFactor + psgTimeUnit / 2) >> psgTimeBits;
	}

	private int toFMTime(int vgmTime) {
		return countSamples(toPSGTime(vgmTime));
	}

	private void runFM(int vgmTime) {
		int count = toFMTime(vgmTime) - fm_pos;
		if (count > 0) {
			fm.update(fm_buf_lr, fm_pos, count);
			fm_pos += count;
		}
	}

	private void write_pcm(int vgmTime, int amp) {
		int blip_time = toPSGTime(vgmTime);
		int old = dac_amp;
		int delta = amp - old;
		dac_amp = amp;
		if (old >= 0) // first write is ignored, to avoid click
			buf.center().addDelta(blip_time, delta * 300);
		else
			dac_amp |= dac_disabled;
	}

	protected int runMsec(int msec) {
		final int duration = vgmRate / 100 * msec / 10;

		{
			int sampleCount = toFMTime(duration);
			java.util.Arrays.fill(fm_buf_lr, 0, sampleCount * 2, 0);
		}
		fm_pos = 0;

		int time = delay;
		while (time < duration) {
			int cmd = cmd_end;
			if (pos < data.length)
				cmd = data[pos++] & 0xFF;
			switch (cmd) {
			case cmd_end:
				pos = loopBegin;
				break;

			case cmd_delay_735:
				time += 735;
				break;

			case cmd_delay_882:
				time += 882;
				break;

			case cmd_gg_stereo:
				apu.writeGG(toPSGTime(time), data[pos++] & 0xFF);
				break;

			case cmd_psg:
				apu.writeData(toPSGTime(time), data[pos++] & 0xFF);
				break;

			case cmd_ym2612_port0:
				if (fm != null) {
					int port = data[pos++] & 0xFF;
					int val = data[pos++] & 0xFF;
					if (port == ym2612_dac_port) {
						write_pcm(time, val);
					} else {
						if (port == 0x2B) {
							dac_disabled = (val >> 7 & 1) - 1;
							dac_amp |= dac_disabled;
						}
						runFM(time);
						fm.write0(port, val);
					}
				}
				break;

			case cmd_ym2612_port1:
				if (fm != null) {
					runFM(time);
					int port = data[pos++] & 0xFF;
					fm.write1(port, data[pos++] & 0xFF);
				}
				break;

			case cmd_delay:
				time += (data[pos + 1] & 0xFF) * 0x100 + (data[pos] & 0xFF);
				pos += 2;
				break;

			case cmd_data_block:
				if (data[pos++] != cmd_end)
					logError();
				int type = data[pos++];
				long size = getLE32(data, pos);
				pos += 4;
				if (type == pcm_block_type)
					pcm_data = pos;
				pos += size;
				break;

			case cmd_pcm_seek:
				pcm_pos = pcm_data + getLE32(data, pos);
				pos += 4;
				break;

			default:
				switch (cmd & 0xF0) {
				case cmd_pcm_delay:
					write_pcm(time, data[pcm_pos++] & 0xFF);
					time += cmd & 0x0F;
					break;

				case cmd_short_delay:
					time += (cmd & 0x0F) + 1;
					break;

				case 0x50:
					pos += 2;
					break;

				default:
					logError();
					break;
				}
			}
		}

		if (fm != null)
			runFM(duration);

		int endTime = toPSGTime(duration);
		delay = time - duration;
		apu.endFrame(endTime);
		if (pos >= data.length) {
			setTrackEnded();
			if (pos > data.length) {
				pos = data.length;
				logError(); // went past end
			}
		}

		fm_pos = 0;

		return endTime;
	}

	protected void mixSamples(byte[] out, int out_off, int count) {
		if (fm == null)
			return;

		out_off *= 2;
		int in_off = fm_pos;

		while (--count >= 0) {
			int s = (out[out_off] << 8) + (out[out_off + 1] & 0xFF);
			s = (s >> 2) + fm_buf_lr[in_off];
			in_off++;
			if ((short) s != s)
				s = (s >> 31) ^ 0x7FFF;
			out[out_off] = (byte) (s >> 8);
			out_off++;
			out[out_off] = (byte) s;
			out_off++;
		}

		fm_pos = in_off;
	}
}