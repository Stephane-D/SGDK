use crate::sample_bank::{SampleBank, Sample};

/// Represents a sample in XGM format (resampled to 14000 Hz, aligned to 256)
#[derive(Clone, Debug)]
pub struct XGMSample {
    pub index: usize,
    pub data: Vec<u8>,
    pub data_size: usize,
    pub origin_addr: i32,
}

impl XGMSample {
    pub fn new(index: usize, data: Vec<u8>, origin_addr: i32) -> Self {
        let data_size = data.len();
        XGMSample { index, data, data_size, origin_addr }
    }

    pub fn create_from_vgm_sample(bank: &SampleBank, sample: &Sample) -> Option<Self> {
        if sample.rate == 0 {
            return None;
        }

        let bank_data_offset = 7; // data starts after 7-byte header in bank data
        let start = bank_data_offset + sample.data_offset;
        let len = sample.len.saturating_sub(1);

        if start + len > bank.data.len() {
            return None;
        }

        let data = resample(
            &bank.data[start..start + len],
            sample.rate as usize,
            14000,
            256,
        );

        Some(XGMSample::new(0, data, sample.data_offset as i32))
    }
}

/// Resample PCM data from input_rate to output_rate, with alignment
pub fn resample(data: &[u8], input_rate: usize, output_rate: usize, align: usize) -> Vec<u8> {
    let step = input_rate as f64 / output_rate as f64;
    let mut result = Vec::new();

    let mut value: f64 = 0.0;
    let mut last_sample: f64 = 0.0;
    let mut sample: f64 = 0.0;
    let mut off: usize = 0;
    let mut d_off: f64 = 0.0;

    while d_off < data.len() as f64 {
        sample = 0.0;

        if step >= 1.0 {
            if value < 0.0 {
                sample += last_sample * -value;
            }
            value += step;
            while value > 0.0 {
                if off < data.len() {
                    last_sample = (data[off] as f64) - 128.0;
                    off += 1;
                }
                if value >= 1.0 {
                    sample += last_sample;
                } else {
                    sample += last_sample * value;
                }
                value -= 1.0;
            }
            sample /= step;
        } else {
            let idx = d_off as usize;
            if idx < data.len() {
                sample = (data[idx] as f64) - 128.0;
            }
        }

        result.push(sample.round() as u8);
        d_off += step;
    }

    // Alignment
    if align > 1 {
        let mask = align - 1;
        let pad = align - (result.len() & mask);
        if pad != align {
            let reduce = sample / pad as f64;
            let mut s = sample;
            for _ in 0..pad {
                s -= reduce;
                result.push(s.round() as u8);
            }
        }
    }

    result
}
