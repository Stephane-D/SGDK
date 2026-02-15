use crate::sample_bank::InternalSample;
use crate::util;

/// XGM Sample have fixed sample rate of 13.3 or 6.65 Khz.
/// Size and Offset are 64 bytes boundary.
pub const XGM_FULL_RATE: i32 = 13300;
pub const XGM_HALF_RATE: i32 = XGM_FULL_RATE / 2;

#[derive(Clone, Debug)]
pub struct XGMSample {
    pub id: i32,
    pub data: Vec<u8>,
    pub half_rate: bool,
    pub origin_id: i32,
    pub origin_addr: i32,
}

impl XGMSample {
    pub fn new(id: i32, data: Vec<u8>, half_rate: bool, origin_id: i32, origin_addr: i32) -> Self {
        XGMSample { id, data, half_rate, origin_id, origin_addr }
    }

    pub fn new_simple(id: i32, data: Vec<u8>) -> Self {
        Self::new(id, data, false, -1, 0)
    }

    pub fn new_from_slice(id: i32, data: &[u8], offset: usize, len: usize) -> Self {
        Self::new_simple(id, data[offset..offset + len].to_vec())
    }

    pub fn create_from_vgm_sample(id: i32, sample: &InternalSample, bank_data: &[u8]) -> Option<Self> {
        let half = sample.rate < 10000;
        let target_rate = if half { XGM_HALF_RATE } else { XGM_FULL_RATE };

        let resampled = util::resample(
            bank_data,
            sample.addr as usize,
            sample.len as usize,
            sample.rate as usize,
            target_rate as usize,
            256,
        );

        Some(XGMSample::new(id, resampled, half, sample.id, sample.addr))
    }

    pub fn get_length(&self) -> usize {
        self.data.len()
    }

    /// Get similarity score between this sample and another.
    /// Returns 1.0 for exact match (on min-size prefix), 0.0 otherwise.
    /// Advanced fingerprint comparison is not implemented (would require musicg port).
    pub fn get_similarity_score(&self, origin_sample: &XGMSample) -> f64 {
        let delta_size = origin_sample.data.len() as i64 - self.data.len() as i64;
        // origin sample is longer ? cannot use a shorter sample to replace it...
        if delta_size > 150 {
            return 0.0;
        }

        let min_size = self.data.len().min(origin_sample.data.len());

        if crate::sample_advanced_compare() {
            // Advanced comparison not implemented (requires musicg fingerprint library).
            // Fall through to simple comparison.
            if crate::is_verbose() {
                println!(
                    "Warning: advanced sample comparison not available, using simple comparison for #{} and #{}",
                    self.id, origin_sample.id
                );
            }
        }

        // simple sample compare (exact match)
        if self.data[..min_size] == origin_sample.data[..min_size] {
            1.0
        } else {
            0.0
        }
    }
}
