/// Single sample class (time is always expressed in 1/44100 of second)
#[derive(Clone, Debug)]
pub struct SingleSample {
    pub sample: i32,
    pub time: i64,
    pub new_sample: bool,
}

impl SingleSample {
    pub const BASE_RATE: i32 = 44100;

    pub fn new(sample: i32, time: i64, new_sample: bool) -> Self {
        SingleSample { sample, time, new_sample }
    }

    pub fn new_simple(sample: i32, time: i64) -> Self {
        Self::new(sample, time, false)
    }
}

impl PartialEq for SingleSample {
    fn eq(&self, other: &Self) -> bool { self.time == other.time }
}
impl Eq for SingleSample {}

impl PartialOrd for SingleSample {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for SingleSample {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.time.cmp(&other.time)
    }
}
