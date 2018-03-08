package org.sgdk.resourcemanager.ui.utils.vgm;

public final class YM2612 {
	static final int NULL_RATE_SIZE = 32;

	// YM2612 Hardware
	private final class cSlot {
		int[] DT;
		int MUL;
		int TL;
		int TLL;
		int SLL;
		int KSR_S;
		int KSR;
		int SEG;
		int AR;
		int DR;
		int SR;
		int RR;
		int Fcnt;
		int Finc;
		int Ecurp;
		int Ecnt;
		int Einc;
		int Ecmp;
		int EincA;
		int EincD;
		int EincS;
		int EincR;
		int ChgEnM;
		int AMS;
		int AMSon;
	};

	private final class cChannel {
		final int[] S0_OUT = new int[4];
		int OUTd;
		int LEFT;
		int RIGHT;
		int ALGO;
		int FB;
		int FMS;
		int AMS;
		final int[] FNUM = new int[4];
		final int[] FOCT = new int[4];
		final int[] KC = new int[4];
		final cSlot[] SLOT = new cSlot[4];

		public cChannel() {
			for (int i = 0; i < 4; i++)
				SLOT[i] = new cSlot();
		}
	};

	// Constants ( taken from MAME YM2612 core )
	private static final int UPD_SIZE = 4000;
	private static final int OUTP_BITS = 16;
	private static final double PI = Math.PI;

	private static final int ATTACK = 0;
	private static final int DECAY = 1;
	private static final int SUSTAIN = 2;
	private static final int RELEASE = 3;

	private static final int SIN_HBITS = 12;
	private static final int SIN_LBITS = (26 - SIN_HBITS);

	private static final int ENV_HBITS = 12;
	private static final int ENV_LBITS = (28 - ENV_HBITS);

	private static final int LFO_HBITS = 10;
	private static final int LFO_LBITS = (28 - LFO_HBITS);

	private static final int SINLEN = (1 << SIN_HBITS);
	private static final int ENVLEN = (1 << ENV_HBITS);
	private static final int LFOLEN = (1 << LFO_HBITS);

	private static final int TLLEN = (ENVLEN * 3);

	private static final int SIN_MSK = (SINLEN - 1);
	private static final int ENV_MSK = (ENVLEN - 1);
	private static final int LFO_MSK = (LFOLEN - 1);

	private static final double ENV_STEP = (96.0 / ENVLEN);

	private static final int ENV_ATTACK = ((ENVLEN * 0) << ENV_LBITS);
	private static final int ENV_DECAY = ((ENVLEN * 1) << ENV_LBITS);
	private static final int ENV_END = ((ENVLEN * 2) << ENV_LBITS);

	private static final int MAX_OUT_BITS = (SIN_HBITS + SIN_LBITS + 2);
	private static final int MAX_OUT = ((1 << MAX_OUT_BITS) - 1);

	private static final int OUT_BITS = (OUTP_BITS - 2);
	private static final int FINAL_SHFT = (MAX_OUT_BITS - OUT_BITS) + 1;
	private static final int LIMIT_CH_OUT = ((int) (((1 << OUT_BITS) * 1.5) - 1));

	private static final int PG_CUT_OFF = ((int) (78.0 / ENV_STEP));
	// private static final int ENV_CUT_OFF = ((int) (68.0 / ENV_STEP));

	private static final int AR_RATE = 399128;
	private static final int DR_RATE = 5514396;

	private static final int LFO_FMS_LBITS = 9;
	private static final int LFO_FMS_BASE = ((int) (0.05946309436 * 0.0338 * (double) (1 << LFO_FMS_LBITS)));

	private static final int S0 = 0;
	private static final int S1 = 2;
	private static final int S2 = 1;
	private static final int S3 = 3;

	private static final int[] DT_DEF_TAB = {
			// FD = 0
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

			// FD = 1
			0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,

			// FD = 2
			1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 16, 16, 16, 16,

			// FD = 3
			2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 20, 22, 22, 22,
			22 };

	private static final int[] FKEY_TAB = { 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3 };

	private static final int[] LFO_AMS_TAB = { 31, 4, 1, 0 };

	private static final int[] LFO_FMS_TAB = { LFO_FMS_BASE * 0, LFO_FMS_BASE * 1, LFO_FMS_BASE * 2, LFO_FMS_BASE * 3,
			LFO_FMS_BASE * 4, LFO_FMS_BASE * 6, LFO_FMS_BASE * 12, LFO_FMS_BASE * 24 };

	// Variables
	private final int[] SIN_TAB = new int[SINLEN];
	private final int[] TL_TAB = new int[TLLEN * 2];
	private final int[] ENV_TAB = new int[2 * ENVLEN + 8]; // uint
	private final int[] DECAY_TO_ATTACK = new int[ENVLEN]; // uint
	private final int[] FINC_TAB = new int[2048]; // uint
	static final int AR_NULL_RATE = 128;
	private final int[] AR_TAB = new int[AR_NULL_RATE + NULL_RATE_SIZE]; // uint
	static final int DR_NULL_RATE = 96;
	private final int[] DR_TAB = new int[DR_NULL_RATE + NULL_RATE_SIZE]; // uint
	private final int[][] DT_TAB = new int[8][32]; // uint
	private final int[] SL_TAB = new int[16]; // uint
	private final int[] LFO_ENV_TAB = new int[LFOLEN];
	private final int[] LFO_FREQ_TAB = new int[LFOLEN];
	private final int[] LFO_ENV_UP = new int[UPD_SIZE];
	private final int[] LFO_FREQ_UP = new int[UPD_SIZE];
	private final int[] LFO_INC_TAB = new int[8];
	private int in0, in1, in2, in3;
	private int en0, en1, en2, en3;
	private int int_cnt;

	// Emultaion State
	private boolean EnableSSGEG = false;

	private static final int MAIN_SHIFT = FINAL_SHFT;

	int YM2612_Clock;
	int YM2612_Rate;
	int YM2612_TimerBase;
	int YM2612_Status;
	int YM2612_LFOcnt;
	int YM2612_LFOinc;
	int YM2612_TimerA;
	int YM2612_TimerAL;
	int YM2612_TimerAcnt;
	int YM2612_TimerB;
	int YM2612_TimerBL;
	int YM2612_TimerBcnt;
	int YM2612_Mode;
	int YM2612_DAC;
	double YM2612_Frequency;
	long YM2612_Inter_Cnt; // UINT
	long YM2612_Inter_Step; // UINT
	final cChannel[] YM2612_CHANNEL = new cChannel[6];
	final int[][] YM2612_REG = new int[2][0x100];

	/**
	 * Creates a new instance of YM2612
	 */
	public YM2612() {
		for (int i = 0; i < 6; i++)
			YM2612_CHANNEL[i] = new cChannel();
	}

	// YM2612 Emulation Methods

	/***********************************************
	 *
	 * Public Access
	 *
	 ***********************************************/

	static private double log10(double x) {
		return Math.log(x) / Math.log(10.0);
	}

	public final int init(int Clock, int Rate) {
		int i, j;
		double x;

		if ((Rate == 0) || (Clock == 0))
			return 1;

		YM2612_Clock = Clock;
		YM2612_Rate = Rate;

		YM2612_Frequency = ((double) YM2612_Clock / (double) YM2612_Rate) / 144.0;
		YM2612_TimerBase = (int) (YM2612_Frequency * 4096.0);

		YM2612_Inter_Step = 0x4000;
		YM2612_Inter_Cnt = 0;

		// TL Table :
		// [0 - 4095] = +output [4095 - ...] = +output overflow (fill with 0)
		// [12288 - 16383] = -output [16384 - ...] = -output overflow (fill with 0)

		for (i = 0; i < TLLEN; i++) {
			if (i >= PG_CUT_OFF) {
				TL_TAB[TLLEN + i] = TL_TAB[i] = 0;
			} else {
				x = MAX_OUT; // Max output
				x /= Math.pow(10, (ENV_STEP * i) / 20);
				TL_TAB[i] = (int) x;
				TL_TAB[TLLEN + i] = -TL_TAB[i];
			}
		}

		// SIN Table :
		// SIN_TAB[x][y] = sin(x) * y;
		// x = phase and y = volume

		SIN_TAB[0] = PG_CUT_OFF;
		SIN_TAB[SINLEN / 2] = PG_CUT_OFF;

		for (i = 1; i <= SINLEN / 4; i++) {
			x = Math.sin(2.0 * PI * (double) (i) / (double) (SINLEN)); // Sinus
			x = 20 * log10(1 / x); // convert to dB

			j = (int) (x / ENV_STEP); // Get TL range

			if (j > PG_CUT_OFF)
				j = (int) PG_CUT_OFF;

			SIN_TAB[i] = j;
			SIN_TAB[(SINLEN / 2) - i] = j;
			SIN_TAB[(SINLEN / 2) + i] = TLLEN + j;
			SIN_TAB[SINLEN - i] = TLLEN + j;
		}

		// LFO Table (LFO wav) :

		for (i = 0; i < LFOLEN; i++) {
			x = Math.sin(2.0 * PI * (double) (i) / (double) (LFOLEN)); // Sinus
			x += 1.0;
			x /= 2.0;
			x *= 11.8 / ENV_STEP;
			LFO_ENV_TAB[i] = (int) x;
			x = Math.sin(2.0 * PI * (double) (i) / (double) (LFOLEN)); // Sinus
			x *= (double) ((1 << (LFO_HBITS - 1)) - 1);
			LFO_FREQ_TAB[i] = (int) x;
		}

		for (i = 0; i < ENVLEN; i++) {
			x = Math.pow(((double) ((ENVLEN - 1) - i) / (double) (ENVLEN)), 8);
			x *= ENVLEN;
			ENV_TAB[i] = (int) x;
			x = Math.pow(((double) (i) / (double) (ENVLEN)), 1);
			x *= ENVLEN;
			ENV_TAB[ENVLEN + i] = (int) x;
		}

		ENV_TAB[ENV_END >> ENV_LBITS] = ENVLEN - 1;

		// Table Decay and Decay

		for (i = 0, j = ENVLEN - 1; i < ENVLEN; i++) {
			while (j != 0 && (ENV_TAB[j] < i))
				j--;
			DECAY_TO_ATTACK[i] = j << ENV_LBITS;
		}

		// Sustain Level Table

		for (i = 0; i < 15; i++) {
			x = i * 3;
			x /= ENV_STEP;

			j = (int) x;
			j <<= ENV_LBITS;
			SL_TAB[i] = j + ENV_DECAY;
		}

		j = ENVLEN - 1; // special case : volume off
		j <<= ENV_LBITS;
		SL_TAB[15] = j + ENV_DECAY;

		// Frequency Step Table

		for (i = 0; i < 2048; i++) {
			x = (double) i * YM2612_Frequency;

			x *= (double) (1 << (SIN_LBITS + SIN_HBITS - (21 - 7)));
			x /= 2.0; // because MUL = value * 2
			FINC_TAB[i] = (int) x; // (unsigned int) x;
		}

		// Attack & Decay Rate Table

		for (i = 0; i < 4; i++) {
			AR_TAB[i] = 0;
			DR_TAB[i] = 0;
		}

		for (i = 0; i < 60; i++) {
			x = YM2612_Frequency;
			x *= 1.0 + ((i & 3) * 0.25); // bits 0-1 : x1.00, x1.25, x1.50, x1.75
			x *= (double) (1 << ((i >> 2))); // bits 2-5 : shift bits (x2^0 - x2^15)
			x *= (double) (ENVLEN << ENV_LBITS); // on ajuste pour le tableau ENV_TAB

			AR_TAB[i + 4] = (int) (x / AR_RATE); // (unsigned int) (x / AR_RATE);
			DR_TAB[i + 4] = (int) (x / DR_RATE); // (unsigned int) (x / DR_RATE);
		}

		for (i = 64; i < 96; i++) {
			AR_TAB[i] = AR_TAB[63];
			DR_TAB[i] = DR_TAB[63];
			AR_TAB[i - 64 + AR_NULL_RATE] = 0;
			DR_TAB[i - 64 + DR_NULL_RATE] = 0;
		}

		// Detune Table
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 32; j++) {
				x = (double) DT_DEF_TAB[(i << 5) + j] * YM2612_Frequency * (double) (1 << (SIN_LBITS + SIN_HBITS - 21));
				DT_TAB[i + 0][j] = (int) x;
				DT_TAB[i + 4][j] = (int) -x;
			}
		}

		// LFO Table
		j = (int) ((YM2612_Rate * YM2612_Inter_Step) / 0x4000);

		LFO_INC_TAB[0] = (int) (3.98 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[1] = (int) (5.56 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[2] = (int) (6.02 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[3] = (int) (6.37 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[4] = (int) (6.88 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[5] = (int) (9.63 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[6] = (int) (48.1 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);
		LFO_INC_TAB[7] = (int) (72.2 * (double) (1 << (LFO_HBITS + LFO_LBITS)) / j);

		reset();
		return 0;
	}

	public final int reset() {
		int i, j;

		YM2612_LFOcnt = 0;
		YM2612_TimerA = 0;
		YM2612_TimerAL = 0;
		YM2612_TimerAcnt = 0;
		YM2612_TimerB = 0;
		YM2612_TimerBL = 0;
		YM2612_TimerBcnt = 0;
		YM2612_DAC = 0;

		YM2612_Status = 0;

		YM2612_Inter_Cnt = 0;

		for (i = 0; i < 6; i++) {
			YM2612_CHANNEL[i].OUTd = 0;
			YM2612_CHANNEL[i].LEFT = 0xFFFFFFFF;
			YM2612_CHANNEL[i].RIGHT = 0xFFFFFFFF;
			YM2612_CHANNEL[i].ALGO = 0;
			YM2612_CHANNEL[i].FB = 31;
			YM2612_CHANNEL[i].FMS = 0;
			YM2612_CHANNEL[i].AMS = 0;

			for (j = 0; j < 4; j++) {
				YM2612_CHANNEL[i].S0_OUT[j] = 0;
				YM2612_CHANNEL[i].FNUM[j] = 0;
				YM2612_CHANNEL[i].FOCT[j] = 0;
				YM2612_CHANNEL[i].KC[j] = 0;

				YM2612_CHANNEL[i].SLOT[j].Fcnt = 0;
				YM2612_CHANNEL[i].SLOT[j].Finc = 0;
				YM2612_CHANNEL[i].SLOT[j].Ecnt = ENV_END; // Put it at the end of Decay phase...
				YM2612_CHANNEL[i].SLOT[j].Einc = 0;
				YM2612_CHANNEL[i].SLOT[j].Ecmp = 0;
				YM2612_CHANNEL[i].SLOT[j].Ecurp = RELEASE;

				YM2612_CHANNEL[i].SLOT[j].ChgEnM = 0;
			}
		}

		for (i = 0; i < 0x100; i++) {
			YM2612_REG[0][i] = -1;
			YM2612_REG[1][i] = -1;
		}

		for (i = 0xB6; i >= 0xB4; i--) {
			write0(i, 0xC0);
			write1(i, 0xC0);
		}

		for (i = 0xB2; i >= 0x22; i--) {
			write0(i, 0);
			write1(i, 0);
		}

		write0(0x2A, 0x80);

		return 0;
	}

	public final int read() {
		return (YM2612_Status);
	}

	public final void write0(int addr, int data) {
		if (addr < 0x30) {
			YM2612_REG[0][addr] = data;
			setYM(addr, data);
		} else if (YM2612_REG[0][addr] != data) {
			YM2612_REG[0][addr] = data;

			if (addr < 0xA0)
				setSlot(addr, data);
			else
				setChannel(addr, data);
		}
	}

	public final void write1(int addr, int data) {
		if (addr >= 0x30 && YM2612_REG[1][addr] != data) {
			YM2612_REG[1][addr] = data;

			if (addr < 0xA0)
				setSlot(addr + 0x100, data);
			else
				setChannel(addr + 0x100, data);
		}
	}

	public final void update(int[] buf_lr, int offset, int end) {
		offset *= 2;
		end = end * 2 + offset;

		if (YM2612_CHANNEL[0].SLOT[0].Finc == -1)
			calc_FINC_CH(YM2612_CHANNEL[0]);
		if (YM2612_CHANNEL[1].SLOT[0].Finc == -1)
			calc_FINC_CH(YM2612_CHANNEL[1]);
		if (YM2612_CHANNEL[2].SLOT[0].Finc == -1) {
			if ((YM2612_Mode & 0x40) != 0) {
				calc_FINC_SL((YM2612_CHANNEL[2].SLOT[S0]),
						FINC_TAB[YM2612_CHANNEL[2].FNUM[2]] >> (7 - YM2612_CHANNEL[2].FOCT[2]),
						YM2612_CHANNEL[2].KC[2]);
				calc_FINC_SL((YM2612_CHANNEL[2].SLOT[S1]),
						FINC_TAB[YM2612_CHANNEL[2].FNUM[3]] >> (7 - YM2612_CHANNEL[2].FOCT[3]),
						YM2612_CHANNEL[2].KC[3]);
				calc_FINC_SL((YM2612_CHANNEL[2].SLOT[S2]),
						FINC_TAB[YM2612_CHANNEL[2].FNUM[1]] >> (7 - YM2612_CHANNEL[2].FOCT[1]),
						YM2612_CHANNEL[2].KC[1]);
				calc_FINC_SL((YM2612_CHANNEL[2].SLOT[S3]),
						FINC_TAB[YM2612_CHANNEL[2].FNUM[0]] >> (7 - YM2612_CHANNEL[2].FOCT[0]),
						YM2612_CHANNEL[2].KC[0]);
			} else {
				calc_FINC_CH(YM2612_CHANNEL[2]);
			}
		}
		if (YM2612_CHANNEL[3].SLOT[0].Finc == -1)
			calc_FINC_CH(YM2612_CHANNEL[3]);
		if (YM2612_CHANNEL[4].SLOT[0].Finc == -1)
			calc_FINC_CH(YM2612_CHANNEL[4]);
		if (YM2612_CHANNEL[5].SLOT[0].Finc == -1)
			calc_FINC_CH(YM2612_CHANNEL[5]);

		// if(YM2612_Inter_Step & 0x04000) algo_type = 0;
		// else algo_type = 16;
		int algo_type = 0;

		if ((YM2612_LFOinc) != 0) {
			// Precalculate LFO wave
			for (int o = offset; o < end; o += 2) {
				int i = o >> 1;
				int j = ((YM2612_LFOcnt += YM2612_LFOinc) >> LFO_LBITS) & LFO_MSK;

				LFO_ENV_UP[i] = LFO_ENV_TAB[j];
				LFO_FREQ_UP[i] = LFO_FREQ_TAB[j];
			}

			algo_type |= 8;
		}

		updateChannel((YM2612_CHANNEL[0].ALGO + algo_type), (YM2612_CHANNEL[0]), buf_lr, offset, end);
		updateChannel((YM2612_CHANNEL[1].ALGO + algo_type), (YM2612_CHANNEL[1]), buf_lr, offset, end);
		updateChannel((YM2612_CHANNEL[2].ALGO + algo_type), (YM2612_CHANNEL[2]), buf_lr, offset, end);
		updateChannel((YM2612_CHANNEL[3].ALGO + algo_type), (YM2612_CHANNEL[3]), buf_lr, offset, end);
		updateChannel((YM2612_CHANNEL[4].ALGO + algo_type), (YM2612_CHANNEL[4]), buf_lr, offset, end);
		if (YM2612_DAC == 0)
			updateChannel(YM2612_CHANNEL[5].ALGO + algo_type, YM2612_CHANNEL[5], buf_lr, offset, end);

		YM2612_Inter_Cnt = int_cnt;
	}

	public final void synchronizeTimers(int length) {
		int i;

		i = YM2612_TimerBase * length;

		if ((YM2612_Mode & 1) != 0) {
			// if((YM2612_TimerAcnt -= 14073) <= 0){ // 13879=NTSC (old: 14475=NTSC
			// 14586=PAL)
			if ((YM2612_TimerAcnt -= i) <= 0) {
				YM2612_Status |= (YM2612_Mode & 0x04) >> 2;
				YM2612_TimerAcnt += YM2612_TimerAL;

				if ((YM2612_Mode & 0x80) != 0)
					CSM_Key_Control();
			}
		}
		if ((YM2612_Mode & 2) != 0) {
			// if((YM2612_TimerBcnt -= 14073) <= 0){ // 13879=NTSC (old: 14475=NTSC
			// 14586=PAL)
			if ((YM2612_TimerBcnt -= i) <= 0) {
				YM2612_Status |= (YM2612_Mode & 0x08) >> 2;
				YM2612_TimerBcnt += YM2612_TimerBL;
			}
		}
	}

	/***********************************************
	 *
	 * Parameter Calculation
	 *
	 ***********************************************/

	private final void calc_FINC_SL(cSlot SL, int finc, int kc) {
		int ksr;
		SL.Finc = (finc + SL.DT[kc]) * SL.MUL;
		ksr = kc >> SL.KSR_S;
		if (SL.KSR != ksr) {
			SL.KSR = ksr;
			SL.EincA = AR_TAB[SL.AR + ksr];
			SL.EincD = DR_TAB[SL.DR + ksr];
			SL.EincS = DR_TAB[SL.SR + ksr];
			SL.EincR = DR_TAB[SL.RR + ksr];
			if (SL.Ecurp == ATTACK)
				SL.Einc = SL.EincA;
			else if (SL.Ecurp == DECAY)
				SL.Einc = SL.EincD;
			else if (SL.Ecnt < ENV_END) {
				if (SL.Ecurp == SUSTAIN)
					SL.Einc = SL.EincS;
				else if (SL.Ecurp == RELEASE)
					SL.Einc = SL.EincR;
			}
		}
	}

	private final void calc_FINC_CH(cChannel CH) {
		int finc, kc;
		finc = (int) (FINC_TAB[CH.FNUM[0]] >> (7 - CH.FOCT[0]));
		kc = CH.KC[0];
		calc_FINC_SL(CH.SLOT[0], finc, kc);
		calc_FINC_SL(CH.SLOT[1], finc, kc);
		calc_FINC_SL(CH.SLOT[2], finc, kc);
		calc_FINC_SL(CH.SLOT[3], finc, kc);
	}

	/***********************************************
	 *
	 * Settings
	 *
	 ***********************************************/

	private final void KEY_ON(cChannel CH, int nsl) {
		cSlot SL = CH.SLOT[nsl];
		if (SL.Ecurp == RELEASE) {
			SL.Fcnt = 0;
			// Fix Ecco 2 splash sound
			SL.Ecnt = (DECAY_TO_ATTACK[ENV_TAB[SL.Ecnt >> ENV_LBITS]] + ENV_ATTACK) & SL.ChgEnM;
			SL.ChgEnM = 0xFFFFFFFF;
			SL.Einc = SL.EincA;
			SL.Ecmp = ENV_DECAY;
			SL.Ecurp = ATTACK;
		}
	}

	private final void KEY_OFF(cChannel CH, int nsl) {
		cSlot SL = CH.SLOT[nsl];
		if (SL.Ecurp != RELEASE) {
			if (SL.Ecnt < ENV_DECAY) {
				SL.Ecnt = (ENV_TAB[SL.Ecnt >> ENV_LBITS] << ENV_LBITS) + ENV_DECAY;
			}
			SL.Einc = SL.EincR;
			SL.Ecmp = ENV_END;
			SL.Ecurp = RELEASE;
		}
	}

	private final void CSM_Key_Control() {
		KEY_ON(YM2612_CHANNEL[2], 0);
		KEY_ON(YM2612_CHANNEL[2], 1);
		KEY_ON(YM2612_CHANNEL[2], 2);
		KEY_ON(YM2612_CHANNEL[2], 3);
	}

	private final int setSlot(int address, int data) { // INT, UCHAR
		data &= 0xFF; // unsign
		cChannel CH;
		cSlot SL;
		int nch, nsl;

		if ((nch = address & 3) == 3)
			return 1;
		nsl = (address >> 2) & 3;

		if ((address & 0x100) != 0)
			nch += 3;

		CH = YM2612_CHANNEL[nch];
		SL = CH.SLOT[nsl];

		switch (address & 0xF0) {
		case 0x30:
			if ((SL.MUL = (data & 0x0F)) != 0)
				SL.MUL <<= 1;
			else
				SL.MUL = 1;
			SL.DT = DT_TAB[(data >> 4) & 7]; // = DT_TAB[(data >> 4) & 7];
			CH.SLOT[0].Finc = -1;
			break;
		case 0x40:
			SL.TL = data & 0x7F;
			SL.TLL = SL.TL << (ENV_HBITS - 7);
			break;
		case 0x50:
			SL.KSR_S = 3 - (data >> 6);
			CH.SLOT[0].Finc = -1;
			if ((data &= 0x1F) != 0)
				SL.AR = data << 1; // = &AR_TAB[data << 1];
			else
				SL.AR = AR_NULL_RATE; // &NULL_RATE[0];

			SL.EincA = AR_TAB[SL.AR + SL.KSR]; // SL.AR[SL.KSR];
			if (SL.Ecurp == ATTACK)
				SL.Einc = SL.EincA;
			break;
		case 0x60:
			if ((SL.AMSon = (data & 0x80)) != 0)
				SL.AMS = CH.AMS;
			else
				SL.AMS = 31;

			if ((data &= 0x1F) != 0)
				SL.DR = data << 1; // = &DR_TAB[data << 1];
			else
				SL.DR = DR_NULL_RATE; // = &NULL_RATE[0];

			SL.EincD = DR_TAB[SL.DR + SL.KSR]; // SL.DR[SL.KSR];
			if (SL.Ecurp == DECAY)
				SL.Einc = SL.EincD;
			break;
		case 0x70:
			if ((data &= 0x1F) != 0)
				SL.SR = data << 1; // = &DR_TAB[data << 1];
			else
				SL.SR = DR_NULL_RATE; // = &NULL_RATE[0];
			SL.EincS = DR_TAB[SL.SR + SL.KSR];
			if ((SL.Ecurp == SUSTAIN) && (SL.Ecnt < ENV_END))
				SL.Einc = SL.EincS;
			break;
		case 0x80:
			SL.SLL = SL_TAB[data >> 4];
			SL.RR = ((data & 0xF) << 2) + 2; // = &DR_TAB[((data & 0xF) << 2) + 2];
			SL.EincR = DR_TAB[SL.RR + SL.KSR]; // [SL.KSR];
			if ((SL.Ecurp == RELEASE) && (SL.Ecnt < ENV_END))
				SL.Einc = SL.EincR;
			break;
		case 0x90:
			if (EnableSSGEG) {
				if ((data & 0x08) != 0)
					SL.SEG = data & 0x0F;
				else
					SL.SEG = 0;
			}
			break;
		}
		return 0;
	}

	private final int setChannel(int address, int data) { // INT,UCHAR
		data &= 0xFF; // unsign
		cChannel CH;
		int num;

		if ((num = address & 3) == 3)
			return 1;

		switch (address & 0xFC) {
		case 0xA0:
			if ((address & 0x100) != 0)
				num += 3;
			CH = YM2612_CHANNEL[num];
			CH.FNUM[0] = (CH.FNUM[0] & 0x700) + data;
			CH.KC[0] = (CH.FOCT[0] << 2) | FKEY_TAB[CH.FNUM[0] >> 7];
			CH.SLOT[0].Finc = -1;
			break;
		case 0xA4:
			if ((address & 0x100) != 0)
				num += 3;
			CH = YM2612_CHANNEL[num];
			CH.FNUM[0] = (CH.FNUM[0] & 0x0FF) + ((int) (data & 0x07) << 8);
			CH.FOCT[0] = (data & 0x38) >> 3;
			CH.KC[0] = (CH.FOCT[0] << 2) | FKEY_TAB[CH.FNUM[0] >> 7];
			CH.SLOT[0].Finc = -1;
			break;
		case 0xA8:
			if (address < 0x100) {
				num++;
				YM2612_CHANNEL[2].FNUM[num] = (YM2612_CHANNEL[2].FNUM[num] & 0x700) + data;
				YM2612_CHANNEL[2].KC[num] = (YM2612_CHANNEL[2].FOCT[num] << 2)
						| FKEY_TAB[YM2612_CHANNEL[2].FNUM[num] >> 7];
				YM2612_CHANNEL[2].SLOT[0].Finc = -1;
			}
			break;
		case 0xAC:
			if (address < 0x100) {
				num++;
				YM2612_CHANNEL[2].FNUM[num] = (YM2612_CHANNEL[2].FNUM[num] & 0x0FF) + ((int) (data & 0x07) << 8);
				YM2612_CHANNEL[2].FOCT[num] = (data & 0x38) >> 3;
				YM2612_CHANNEL[2].KC[num] = (YM2612_CHANNEL[2].FOCT[num] << 2)
						| FKEY_TAB[YM2612_CHANNEL[2].FNUM[num] >> 7];
				YM2612_CHANNEL[2].SLOT[0].Finc = -1;
			}
			break;
		case 0xB0:
			if ((address & 0x100) != 0)
				num += 3;
			CH = YM2612_CHANNEL[num];
			if (CH.ALGO != (data & 7)) {
				CH.ALGO = data & 7;
				CH.SLOT[0].ChgEnM = 0;
				CH.SLOT[1].ChgEnM = 0;
				CH.SLOT[2].ChgEnM = 0;
				CH.SLOT[3].ChgEnM = 0;
			}
			CH.FB = 9 - ((data >> 3) & 7); // Real thing ?
			// if(CH.FB = ((data >> 3) & 7)) CH.FB = 9 - CH.FB; // Thunder force 4 (music
			// stage 8), Gynoug, Aladdin bug sound...
			// else CH.FB = 31;
			break;
		case 0xB4:
			if ((address & 0x100) != 0)
				num += 3;
			CH = YM2612_CHANNEL[num];
			if ((data & 0x80) != 0)
				CH.LEFT = 0xFFFFFFFF;
			else
				CH.LEFT = 0;
			if ((data & 0x40) != 0)
				CH.RIGHT = 0xFFFFFFFF;
			else
				CH.RIGHT = 0;
			CH.AMS = LFO_AMS_TAB[(data >> 4) & 3];
			CH.FMS = LFO_FMS_TAB[data & 7];
			if (CH.SLOT[0].AMSon != 0)
				CH.SLOT[0].AMS = CH.AMS;
			else
				CH.SLOT[0].AMS = 31;
			if (CH.SLOT[1].AMSon != 0)
				CH.SLOT[1].AMS = CH.AMS;
			else
				CH.SLOT[1].AMS = 31;
			if (CH.SLOT[2].AMSon != 0)
				CH.SLOT[2].AMS = CH.AMS;
			else
				CH.SLOT[2].AMS = 31;
			if (CH.SLOT[3].AMSon != 0)
				CH.SLOT[3].AMS = CH.AMS;
			else
				CH.SLOT[3].AMS = 31;
			break;
		}
		return 0;
	}

	private final int setYM(int address, int data) { // INT, UCHAR
		cChannel CH;
		int nch;

		switch (address) {
		case 0x22:
			if ((data & 8) != 0) {
				// Cool Spot music 1, LFO modified severals time which
				// distorts the sound, have to check that on a real genesis...
				YM2612_LFOinc = LFO_INC_TAB[data & 7];
			} else {
				YM2612_LFOinc = YM2612_LFOcnt = 0;
			}
			break;
		case 0x24:
			YM2612_TimerA = (YM2612_TimerA & 0x003) | (((int) data) << 2);
			if (YM2612_TimerAL != ((1024 - YM2612_TimerA) << 12)) {
				YM2612_TimerAcnt = YM2612_TimerAL = (1024 - YM2612_TimerA) << 12;
			}
			// System.out.println("Timer AH: " + Integer.toHexString(YM2612_TimerA));
			break;
		case 0x25:
			YM2612_TimerA = (YM2612_TimerA & 0x3fc) | (data & 3);
			if (YM2612_TimerAL != ((1024 - YM2612_TimerA) << 12)) {
				YM2612_TimerAcnt = YM2612_TimerAL = (1024 - YM2612_TimerA) << 12;
			}
			// System.out.println("Timer AL: " + Integer.toHexString(YM2612_TimerA));
			break;
		case 0x26:
			YM2612_TimerB = data;
			if (YM2612_TimerBL != ((256 - YM2612_TimerB) << (4 + 12))) {
				YM2612_TimerBcnt = YM2612_TimerBL = (256 - YM2612_TimerB) << (4 + 12);
			}
			// System.out.println("Timer B : " + Integer.toHexString(YM2612_TimerB));
			break;
		case 0x27:
			if (((data ^ YM2612_Mode) & 0x40) != 0) {
				// We changed the channel 2 mode, so recalculate phase step
				// This fix the punch sound in Street of Rage 2
				YM2612_CHANNEL[2].SLOT[0].Finc = -1; // recalculate phase step
			}
			YM2612_Status &= (~data >> 4) & (data >> 2); // Reset Status
			YM2612_Mode = data;
			break;
		case 0x28:
			if ((nch = data & 3) == 3)
				return 1;
			if ((data & 4) != 0)
				nch += 3;
			CH = YM2612_CHANNEL[nch];
			if ((data & 0x10) != 0)
				KEY_ON(CH, S0);
			else
				KEY_OFF(CH, S0);
			if ((data & 0x20) != 0)
				KEY_ON(CH, S1);
			else
				KEY_OFF(CH, S1);
			if ((data & 0x40) != 0)
				KEY_ON(CH, S2);
			else
				KEY_OFF(CH, S2);
			if ((data & 0x80) != 0)
				KEY_ON(CH, S3);
			else
				KEY_OFF(CH, S3);
			break;
		case 0x2A:
			break;
		case 0x2B:
			YM2612_DAC = data & 0x80;
			break;
		}
		return 0;
	}

	private final void Env_Attack_Next(cSlot SL) {
		SL.Ecnt = ENV_DECAY;
		SL.Einc = SL.EincD;
		SL.Ecmp = SL.SLL;
		SL.Ecurp = DECAY;
	}

	private final void Env_Decay_Next(cSlot SL) {
		SL.Ecnt = SL.SLL;
		SL.Einc = SL.EincS;
		SL.Ecmp = ENV_END;
		SL.Ecurp = SUSTAIN;
	}

	private final void Env_Sustain_Next(cSlot SL) {
		if (EnableSSGEG) {
			if ((SL.SEG & 8) != 0) {
				if ((SL.SEG & 1) != 0) {
					SL.Ecnt = ENV_END;
					SL.Einc = 0;
					SL.Ecmp = ENV_END + 1;
				} else {
					SL.Ecnt = 0;
					SL.Einc = SL.EincA;
					SL.Ecmp = ENV_DECAY;
					SL.Ecurp = ATTACK;
				}
				SL.SEG ^= (SL.SEG & 2) << 1;
			} else {
				SL.Ecnt = ENV_END;
				SL.Einc = 0;
				SL.Ecmp = ENV_END + 1;
			}
		} else {
			SL.Ecnt = ENV_END;
			SL.Einc = 0;
			SL.Ecmp = ENV_END + 1;
		}
	}

	private final void Env_Release_Next(cSlot SL) {
		SL.Ecnt = ENV_END;
		SL.Einc = 0;
		SL.Ecmp = ENV_END + 1;
	}

	private final void ENV_NEXT_EVENT(int which, cSlot SL) {
		switch (which) {
		case 0:
			Env_Attack_Next(SL);
			return;
		case 1:
			Env_Decay_Next(SL);
			return;
		case 2:
			Env_Sustain_Next(SL);
			return;
		case 3:
			Env_Release_Next(SL);
			return;
		// default: Env_NULL_Next(SL); return;
		}
	}

	private final void calcChannel(int ALGO, cChannel CH) {
		// DO_FEEDBACK
		in0 += (CH.S0_OUT[0] + CH.S0_OUT[1]) >> CH.FB;
		CH.S0_OUT[1] = CH.S0_OUT[0];
		CH.S0_OUT[0] = TL_TAB[SIN_TAB[(in0 >> SIN_LBITS) & SIN_MSK] + en0];
		switch (ALGO) {
		case 0:
			in1 += CH.S0_OUT[1];
			in2 += TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1];
			in3 += TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2];
			CH.OUTd = TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3] >> MAIN_SHIFT;
			break;
		case 1:
			in2 += CH.S0_OUT[1] + TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1];
			in3 += TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2];
			CH.OUTd = TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3] >> MAIN_SHIFT;
			break;
		case 2:
			in2 += TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1];
			in3 += CH.S0_OUT[1] + TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2];
			CH.OUTd = TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3] >> MAIN_SHIFT;
			break;
		case 3:
			in1 += CH.S0_OUT[1];
			in3 += TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1]
					+ TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2];
			CH.OUTd = TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3] >> MAIN_SHIFT;
			break;
		case 4:
			in1 += CH.S0_OUT[1];
			in3 += TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2];
			CH.OUTd = (TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3]
					+ TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1]) >> MAIN_SHIFT;
			break;
		case 5:
			in1 += CH.S0_OUT[1];
			in2 += CH.S0_OUT[1];
			in3 += CH.S0_OUT[1];
			CH.OUTd = (TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3]
					+ TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1]
					+ TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2]) >> MAIN_SHIFT;
			break;
		case 6:
			in1 += CH.S0_OUT[1];
			CH.OUTd = (TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3]
					+ TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1]
					+ TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2]) >> MAIN_SHIFT;
			break;
		case 7:
			CH.OUTd = (TL_TAB[SIN_TAB[(in3 >> SIN_LBITS) & SIN_MSK] + en3]
					+ TL_TAB[SIN_TAB[(in1 >> SIN_LBITS) & SIN_MSK] + en1]
					+ TL_TAB[SIN_TAB[(in2 >> SIN_LBITS) & SIN_MSK] + en2] + CH.S0_OUT[1]) >> MAIN_SHIFT;
			break;
		}
		// DO_LIMIT
		if (CH.OUTd > LIMIT_CH_OUT)
			CH.OUTd = LIMIT_CH_OUT;
		else if (CH.OUTd < -LIMIT_CH_OUT)
			CH.OUTd = -LIMIT_CH_OUT;
	}

	private final void processChannel(cChannel CH, int[] buf_lr, int OFFSET, int END, int ALGO) {
		if (ALGO < 4) {
			if (CH.SLOT[S3].Ecnt == ENV_END)
				return;
		} else if (ALGO == 4) {
			if ((CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		} else if (ALGO < 7) {
			if ((CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S2].Ecnt == ENV_END) && (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		} else {
			if ((CH.SLOT[S0].Ecnt == ENV_END) && (CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S2].Ecnt == ENV_END)
					&& (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		}

		do {
			// GET_CURRENT_PHASE
			in0 = CH.SLOT[S0].Fcnt;
			in1 = CH.SLOT[S1].Fcnt;
			in2 = CH.SLOT[S2].Fcnt;
			in3 = CH.SLOT[S3].Fcnt;
			// UPDATE_PHASE
			CH.SLOT[S0].Fcnt += CH.SLOT[S0].Finc;
			CH.SLOT[S1].Fcnt += CH.SLOT[S1].Finc;
			CH.SLOT[S2].Fcnt += CH.SLOT[S2].Finc;
			CH.SLOT[S3].Fcnt += CH.SLOT[S3].Finc;
			// GET_CURRENT_ENV
			if ((CH.SLOT[S0].SEG & 4) != 0) {
				if ((en0 = ENV_TAB[(CH.SLOT[S0].Ecnt >> ENV_LBITS)] + CH.SLOT[S0].TLL) > ENV_MSK)
					en0 = 0;
				else
					en0 ^= ENV_MSK;
			} else
				en0 = ENV_TAB[(CH.SLOT[S0].Ecnt >> ENV_LBITS)] + CH.SLOT[S0].TLL;
			if ((CH.SLOT[S1].SEG & 4) != 0) {
				if ((en1 = ENV_TAB[(CH.SLOT[S1].Ecnt >> ENV_LBITS)] + CH.SLOT[S1].TLL) > ENV_MSK)
					en1 = 0;
				else
					en1 ^= ENV_MSK;
			} else
				en1 = ENV_TAB[(CH.SLOT[S1].Ecnt >> ENV_LBITS)] + CH.SLOT[S1].TLL;
			if ((CH.SLOT[S2].SEG & 4) != 0) {
				if ((en2 = ENV_TAB[(CH.SLOT[S2].Ecnt >> ENV_LBITS)] + CH.SLOT[S2].TLL) > ENV_MSK)
					en2 = 0;
				else
					en2 ^= ENV_MSK;
			} else
				en2 = ENV_TAB[(CH.SLOT[S2].Ecnt >> ENV_LBITS)] + CH.SLOT[S2].TLL;
			if ((CH.SLOT[S3].SEG & 4) != 0) {
				if ((en3 = ENV_TAB[(CH.SLOT[S3].Ecnt >> ENV_LBITS)] + CH.SLOT[S3].TLL) > ENV_MSK)
					en3 = 0;
				else
					en3 ^= ENV_MSK;
			} else
				en3 = ENV_TAB[(CH.SLOT[S3].Ecnt >> ENV_LBITS)] + CH.SLOT[S3].TLL;
			// UPDATE_ENV
			if ((CH.SLOT[S0].Ecnt += CH.SLOT[S0].Einc) >= CH.SLOT[S0].Ecmp) {
				ENV_NEXT_EVENT(CH.SLOT[S0].Ecurp, CH.SLOT[S0]);
			}
			if ((CH.SLOT[S1].Ecnt += CH.SLOT[S1].Einc) >= CH.SLOT[S1].Ecmp) {
				ENV_NEXT_EVENT(CH.SLOT[S1].Ecurp, CH.SLOT[S1]);
			}
			if ((CH.SLOT[S2].Ecnt += CH.SLOT[S2].Einc) >= CH.SLOT[S2].Ecmp) {
				ENV_NEXT_EVENT(CH.SLOT[S2].Ecurp, CH.SLOT[S2]);
			}
			if ((CH.SLOT[S3].Ecnt += CH.SLOT[S3].Einc) >= CH.SLOT[S3].Ecmp) {
				ENV_NEXT_EVENT(CH.SLOT[S3].Ecurp, CH.SLOT[S3]);
			}
			calcChannel(ALGO, CH);
			// DO_OUTPUT
			buf_lr[OFFSET] += (CH.OUTd & CH.LEFT);
			buf_lr[OFFSET + 1] += (CH.OUTd & CH.RIGHT);
			OFFSET += 2;
		} while (OFFSET < END);
	}

	private final void processChannel_LFO(cChannel CH, int[] buf_lr, int OFFSET, int END, int ALGO) {
		if (ALGO < 4) {
			if (CH.SLOT[S3].Ecnt == ENV_END)
				return;
		} else if (ALGO == 4) {
			if ((CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		} else if (ALGO < 7) {
			if ((CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S2].Ecnt == ENV_END) && (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		} else {
			if ((CH.SLOT[S0].Ecnt == ENV_END) && (CH.SLOT[S1].Ecnt == ENV_END) && (CH.SLOT[S2].Ecnt == ENV_END)
					&& (CH.SLOT[S3].Ecnt == ENV_END))
				return;
		}

		do {
			final int i = OFFSET >> 1;

			// GET_CURRENT_PHASE
			in0 = CH.SLOT[S0].Fcnt;
			in1 = CH.SLOT[S1].Fcnt;
			in2 = CH.SLOT[S2].Fcnt;
			in3 = CH.SLOT[S3].Fcnt;
			// UPDATE_PHASE_LFO
			int freq_LFO = (CH.FMS * LFO_FREQ_UP[i]) >> (LFO_HBITS - 1);
			if (freq_LFO != 0) {
				CH.SLOT[S0].Fcnt += CH.SLOT[S0].Finc + ((CH.SLOT[S0].Finc * freq_LFO) >> LFO_FMS_LBITS);
				CH.SLOT[S1].Fcnt += CH.SLOT[S1].Finc + ((CH.SLOT[S1].Finc * freq_LFO) >> LFO_FMS_LBITS);
				CH.SLOT[S2].Fcnt += CH.SLOT[S2].Finc + ((CH.SLOT[S2].Finc * freq_LFO) >> LFO_FMS_LBITS);
				CH.SLOT[S3].Fcnt += CH.SLOT[S3].Finc + ((CH.SLOT[S3].Finc * freq_LFO) >> LFO_FMS_LBITS);
			} else {
				CH.SLOT[S0].Fcnt += CH.SLOT[S0].Finc;
				CH.SLOT[S1].Fcnt += CH.SLOT[S1].Finc;
				CH.SLOT[S2].Fcnt += CH.SLOT[S2].Finc;
				CH.SLOT[S3].Fcnt += CH.SLOT[S3].Finc;
			}
			// GET_CURRENT_ENV_LFO
			int env_LFO = LFO_ENV_UP[i];
			if ((CH.SLOT[S0].SEG & 4) != 0) {
				if ((en0 = ENV_TAB[(CH.SLOT[S0].Ecnt >> ENV_LBITS)] + CH.SLOT[S0].TLL) > ENV_MSK)
					en0 = 0;
				else
					en0 = (en0 ^ ENV_MSK) + (env_LFO >> CH.SLOT[S0].AMS);
			} else
				en0 = ENV_TAB[(CH.SLOT[S0].Ecnt >> ENV_LBITS)] + CH.SLOT[S0].TLL + (env_LFO >> CH.SLOT[S0].AMS);
			if ((CH.SLOT[S1].SEG & 4) != 0) {
				if ((en1 = ENV_TAB[(CH.SLOT[S1].Ecnt >> ENV_LBITS)] + CH.SLOT[S1].TLL) > ENV_MSK)
					en1 = 0;
				else
					en1 = (en1 ^ ENV_MSK) + (env_LFO >> CH.SLOT[S1].AMS);
			} else
				en1 = ENV_TAB[(CH.SLOT[S1].Ecnt >> ENV_LBITS)] + CH.SLOT[S1].TLL + (env_LFO >> CH.SLOT[S1].AMS);
			if ((CH.SLOT[S2].SEG & 4) != 0) {
				if ((en2 = ENV_TAB[(CH.SLOT[S2].Ecnt >> ENV_LBITS)] + CH.SLOT[S2].TLL) > ENV_MSK)
					en2 = 0;
				else
					en2 = (en2 ^ ENV_MSK) + (env_LFO >> CH.SLOT[S2].AMS);
			} else
				en2 = ENV_TAB[(CH.SLOT[S2].Ecnt >> ENV_LBITS)] + CH.SLOT[S2].TLL + (env_LFO >> CH.SLOT[S2].AMS);
			if ((CH.SLOT[S3].SEG & 4) != 0) {
				if ((en3 = ENV_TAB[(CH.SLOT[S3].Ecnt >> ENV_LBITS)] + CH.SLOT[S3].TLL) > ENV_MSK)
					en3 = 0;
				else
					en3 = (en3 ^ ENV_MSK) + (env_LFO >> CH.SLOT[S3].AMS);
			} else
				en3 = ENV_TAB[(CH.SLOT[S3].Ecnt >> ENV_LBITS)] + CH.SLOT[S3].TLL + (env_LFO >> CH.SLOT[S3].AMS);

			// UPDATE_ENV
			if ((CH.SLOT[S0].Ecnt += CH.SLOT[S0].Einc) >= CH.SLOT[S0].Ecmp)
				ENV_NEXT_EVENT(CH.SLOT[S0].Ecurp, CH.SLOT[S0]);

			if ((CH.SLOT[S1].Ecnt += CH.SLOT[S1].Einc) >= CH.SLOT[S1].Ecmp)
				ENV_NEXT_EVENT(CH.SLOT[S1].Ecurp, CH.SLOT[S1]);

			if ((CH.SLOT[S2].Ecnt += CH.SLOT[S2].Einc) >= CH.SLOT[S2].Ecmp)
				ENV_NEXT_EVENT(CH.SLOT[S2].Ecurp, CH.SLOT[S2]);

			if ((CH.SLOT[S3].Ecnt += CH.SLOT[S3].Einc) >= CH.SLOT[S3].Ecmp)
				ENV_NEXT_EVENT(CH.SLOT[S3].Ecurp, CH.SLOT[S3]);

			calcChannel(ALGO, CH);
			// DO_OUTPUT
			int left = (CH.OUTd & CH.LEFT);
			int right = (CH.OUTd & CH.RIGHT);

			buf_lr[OFFSET] += left;
			buf_lr[OFFSET + 1] += right;
			OFFSET += 2;
		} while (OFFSET < END);
	}

	private final void updateChannel(int ALGO, cChannel CH, int[] buf_lr, int OFFSET, int END) {
		if (ALGO < 8) {
			processChannel(CH, buf_lr, OFFSET, END, ALGO);
		} else {
			processChannel_LFO(CH, buf_lr, OFFSET, END, ALGO - 8);
		}
	}
}
