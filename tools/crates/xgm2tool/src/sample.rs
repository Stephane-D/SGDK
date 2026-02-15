/// Signed 8-bit sample with sample rate
#[derive(Clone, Debug)]
pub struct Sample {
    pub data: Vec<u8>,
    pub sample_rate: i32,
}

impl Sample {
    pub fn new(data: Vec<u8>, sample_rate: i32) -> Self {
        Sample { data, sample_rate }
    }

    pub fn get_sample(&self, index: usize) -> i8 {
        self.data[index] as i8
    }

    pub fn get_size(&self) -> usize {
        self.data.len()
    }

    pub fn matches(&self, other: &Sample, start_index: usize) -> bool {
        if start_index >= self.data.len() {
            return false;
        }
        for (i, si) in (start_index..self.data.len()).zip(0..other.data.len()) {
            if self.data[i] != other.data[si] {
                return false;
            }
        }
        true
    }

    /// A sample rate of 0 means invalid
    pub fn is_valid(&self) -> bool {
        self.sample_rate != 0
    }
}
