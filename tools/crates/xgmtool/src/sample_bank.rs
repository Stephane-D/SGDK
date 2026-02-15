use crate::vgm_command::*;

/// Represents a PCM sample within a VGM data bank
#[derive(Clone, Debug)]
pub struct Sample {
    pub id: usize,
    pub data_offset: usize,
    pub len: usize,
    pub rate: u32,
}

impl Sample {
    pub fn new(id: usize, data_offset: usize, len: usize, rate: u32) -> Self {
        Sample { id, data_offset, len, rate }
    }

    pub fn get_frame_size(&self) -> usize {
        if self.rate > 0 { self.rate as usize / 60 } else { 4000 / 60 }
    }

    pub fn set_rate(&mut self, value: u32) {
        if value != 0 && self.rate != value {
            if crate::is_verbose() {
                println!("Sample modified  [{:06X}-{:06X}]  rate: {} --> {} Hz",
                    self.data_offset, self.data_offset + self.len - 1, self.rate, value);
            }
            self.rate = value;
        }
    }

    pub fn get_set_rate_command(&self, bank_id: u8, value: u32) -> VGMCommand {
        let data = vec![
            VGM_STREAM_FREQUENCY, bank_id,
            (value & 0xFF) as u8, ((value >> 8) & 0xFF) as u8, 0x00, 0x00,
        ];
        VGMCommand::from_owned(data, -1)
    }

    pub fn get_start_long_command_ex(&self, bank_id: u8, len: usize) -> VGMCommand {
        let adj_len = len.min(self.len);
        let data = vec![
            VGM_STREAM_START_LONG, bank_id,
            (self.data_offset & 0xFF) as u8, ((self.data_offset >> 8) & 0xFF) as u8,
            ((self.data_offset >> 16) & 0xFF) as u8, 0x00, 0x01,
            (adj_len & 0xFF) as u8, ((adj_len >> 8) & 0xFF) as u8,
            ((adj_len >> 16) & 0xFF) as u8, 0x00,
        ];
        VGMCommand::from_owned(data, -1)
    }

    pub fn get_start_long_command(&self, bank_id: u8) -> VGMCommand {
        self.get_start_long_command_ex(bank_id, self.len)
    }

    pub fn get_stop_command(&self, bank_id: u8) -> VGMCommand {
        VGMCommand::from_owned(vec![VGM_STREAM_STOP, bank_id], -1)
    }
}

/// A bank of PCM sample data from a VGM file
#[derive(Clone, Debug)]
pub struct SampleBank {
    pub samples: Vec<Sample>,
    pub data: Vec<u8>,
    pub len: usize,
    pub id: u8,
}

impl SampleBank {
    pub fn create(command: &VGMCommand) -> Option<Self> {
        if !command.is_data_block() {
            println!("Error: incorrect sample data declaration at {:06X} !", command.offset);
            return None;
        }

        let id = command.get_data_bank_id();
        let block_len = command.get_data_block_len() as usize;

        if crate::is_verbose() {
            println!("Initial bank sample added [{:06X}-{:06X}]   rate: 0 Hz", 0, block_len - 1);
        }

        // The data starts at offset 7 in the command (after 0x67 0x66 type len[4])
        let data = command.data.clone();
        let samples = vec![Sample::new(0, 0, block_len, 0)];

        Some(SampleBank { samples, data, len: block_len, id })
    }

    pub fn add_block(&mut self, command: &VGMCommand) {
        if !command.is_data_block() { return; }

        let block_len = command.get_data_block_len() as usize;
        let new_len = self.len + block_len;

        // Build new data: existing header+data + new block data
        let mut new_data = self.data[..self.len + 7].to_vec();
        new_data.extend_from_slice(&command.data[7..7 + block_len]);
        crate::set_u32(&mut new_data, 3, new_len as u32);

        if crate::is_verbose() {
            println!("Initial block sample added [{:06X}-{:06X}]   rate: 0 Hz", self.len, new_len - 1);
        }

        let sample_id = self.samples.len();
        self.samples.push(Sample::new(sample_id, self.len, block_len, 0));
        self.data = new_data;
        self.len = new_len;
    }

    pub fn get_data_block_command(&self) -> VGMCommand {
        VGMCommand::from_owned(self.data.clone(), -1)
    }

    pub fn get_declaration_commands(&self) -> Vec<VGMCommand> {
        vec![
            VGMCommand::from_owned(vec![VGM_STREAM_CONTROL, self.id, 0x02, 0x00, 0x2A], -1),
            VGMCommand::from_owned(vec![VGM_STREAM_DATA, self.id, self.id, 0x01, 0x00], -1),
        ]
    }

    pub fn get_sample_by_offset(&self, data_offset: usize) -> Option<&Sample> {
        self.samples.iter().find(|s| {
            let frame_size = s.get_frame_size();
            (s.data_offset as i64 - data_offset as i64).unsigned_abs() < frame_size as u64
        })
    }

    pub fn get_sample_by_offset_mut(&mut self, data_offset: usize) -> Option<&mut Sample> {
        self.samples.iter_mut().find(|s| {
            let frame_size = s.get_frame_size();
            (s.data_offset as i64 - data_offset as i64).unsigned_abs() < frame_size as u64
        })
    }

    pub fn get_sample_by_id(&self, id: usize) -> Option<&Sample> {
        self.samples.iter().find(|s| s.id == id)
    }

    pub fn add_sample(&mut self, data_offset: usize, len: usize, rate: u32) -> usize {
        // Try to find existing sample at this offset
        if let Some(idx) = self.samples.iter().position(|s| {
            let frame_size = s.get_frame_size();
            (s.data_offset as i64 - data_offset as i64).unsigned_abs() < frame_size as u64
        }) {
            let sample = &mut self.samples[idx];
            if sample.rate == 0 {
                if crate::is_verbose() {
                    println!("Sample confirmed [{:06X}-{:06X}]  len: {:06X} --> {:06X}   rate: {} --> {} Hz",
                        data_offset, data_offset + len - 1, sample.len, len, sample.rate, rate);
                }
                sample.rate = rate;
                sample.len = len;
            } else if sample.len < len {
                if crate::is_verbose() {
                    println!("Sample modified  [{:06X}-{:06X}]  len: {:06X} --> {:06X}",
                        data_offset, data_offset + len - 1, sample.len, len);
                }
                sample.len = len;
            }
            return idx;
        }

        // New sample
        if crate::is_verbose() {
            println!("Sample added     [{:06X}-{:06X}]  len: {:06X}  rate: {} Hz",
                data_offset, data_offset + len - 1, len, rate);
        }
        let id = self.samples.len();
        self.samples.push(Sample::new(id, data_offset, len, rate));
        id
    }
}
