use crate::vgm_sample_data::VGMSampleData;
use std::collections::HashMap;

/// VGM Sample (time is always expressed in 1/44100 of second)
#[derive(Clone, Debug)]
pub struct VGMSample {
    pub sample_data_list: Vec<VGMSampleData>,
}

impl VGMSample {
    pub fn new() -> Self {
        VGMSample { sample_data_list: Vec::new() }
    }

    pub fn add_sample_data(&mut self, offset: i64, data: u8) {
        self.sample_data_list.push(VGMSampleData::new(offset, data));
    }

    pub fn sort(&mut self) {
        self.sample_data_list.sort();
    }

    pub fn get_mean_sample_rate(&self) -> i32 {
        if self.sample_data_list.len() <= 1 {
            return 0;
        }
        let mut sum_delta = 0.0f64;
        for i in 0..self.sample_data_list.len() - 1 {
            let s0 = &self.sample_data_list[i];
            let s1 = &self.sample_data_list[i + 1];
            sum_delta += (s1.time - s0.time) as f64;
        }
        sum_delta /= (self.sample_data_list.len() - 1) as f64;
        (44100.0 / sum_delta).round() as i32
    }

    pub fn get_wanted_sample_rate(&self) -> i32 {
        let mut gap_histo: HashMap<i64, i32> = HashMap::new();

        for i in 0..self.sample_data_list.len().saturating_sub(1) {
            let s0 = &self.sample_data_list[i];
            let s1 = &self.sample_data_list[i + 1];
            let key = s1.time - s0.time;
            *gap_histo.entry(key).or_insert(0) += 1;
        }

        let max_count = gap_histo.values().copied().max().unwrap_or(0);
        let minimum_gap_count = max_count / 3;

        let mut counted_gap = 0.0f64;
        let mut time_sum = 0.0f64;

        for (&key, &count) in &gap_histo {
            if count >= minimum_gap_count {
                time_sum += key as f64 * count as f64;
                counted_gap += count as f64;
            }
        }

        if counted_gap == 0.0 { return 0; }
        (time_sum / counted_gap).round() as i32
    }
}
