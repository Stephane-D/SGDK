/// Single byte of VGM sample data (time is expressed in 1/44100 of second)
#[derive(Clone, Debug)]
pub struct VGMSampleData {
    pub time: i64,
    pub data: u8,
}

impl VGMSampleData {
    pub fn new(time: i64, data: u8) -> Self {
        VGMSampleData { time, data }
    }
}

impl PartialEq for VGMSampleData {
    fn eq(&self, other: &Self) -> bool { self.time == other.time }
}
impl Eq for VGMSampleData {}

impl PartialOrd for VGMSampleData {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for VGMSampleData {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.time.cmp(&other.time)
    }
}
