use std::collections::HashMap;

use crate::sample::Sample;
use crate::vgm_command::VGMCommand;
use crate::util;

/// Internal sample within a SampleBank
#[derive(Clone, Debug)]
pub struct InternalSample {
    pub id: i32,
    pub addr: i32,
    pub len: i32,
    pub rate: i32,
}

impl InternalSample {
    pub fn new(id: i32, addr: i32, len: i32, rate: i32) -> Self {
        if crate::is_verbose() {
            println!(
                "Sample #{} added     [{:06X}-{:06X}]  len: {:06X}  rate: {} Hz",
                id, addr, addr + (len - 1), len, rate,
            );
        }
        InternalSample { id, addr, len, rate }
    }

    pub fn set_rate(&mut self, value: i32) {
        if value != 0 && self.rate != value {
            if crate::is_verbose() {
                println!(
                    "Sample #{} changed   [{:06X}-{:06X}]  rate: {} --> {}",
                    self.id, self.addr, self.addr + (self.len - 1), self.rate, value,
                );
            }
            self.rate = value;
        }
    }

    pub fn get_frame_size(&self) -> i32 {
        if self.rate > 0 {
            self.rate / 60
        } else {
            4000 / 60
        }
    }

    pub fn match_address(&self, address: i32) -> bool {
        let frame_size = self.get_frame_size();
        (self.addr - address).abs() < frame_size
    }

    pub fn get_set_rate_command(&self, bank_id: i32, r: i32) -> VGMCommand {
        VGMCommand::from_slice(&[
            crate::vgm_command::STREAM_FREQUENCY,
            bank_id as u8,
            (r & 0xFF) as u8,
            ((r >> 8) & 0xFF) as u8,
            0x00,
            0x00,
        ])
    }

    pub fn get_start_long_command_with_len(&self, bank_id: i32, l: i32) -> VGMCommand {
        let adj_len = l.min(self.len);
        VGMCommand::from_slice(&[
            crate::vgm_command::STREAM_START_LONG,
            bank_id as u8,
            (self.addr & 0xFF) as u8,
            ((self.addr >> 8) & 0xFF) as u8,
            ((self.addr >> 16) & 0xFF) as u8,
            0x00,
            0x01,
            (adj_len & 0xFF) as u8,
            ((adj_len >> 8) & 0xFF) as u8,
            ((adj_len >> 16) & 0xFF) as u8,
            0x00,
        ])
    }

    pub fn get_start_long_command(&self, bank_id: i32) -> VGMCommand {
        self.get_start_long_command_with_len(bank_id, self.len)
    }

    pub fn get_stop_command(&self, bank_id: i32) -> VGMCommand {
        VGMCommand::from_slice(&[crate::vgm_command::STREAM_STOP, bank_id as u8])
    }

    pub fn to_isolated_sample(&self, bank_data: &[u8]) -> Sample {
        let addr = self.addr as usize;
        let len = self.len as usize;
        let data = bank_data[addr..addr + len].to_vec();
        Sample::new(data, self.rate)
    }
}

impl PartialEq for InternalSample {
    fn eq(&self, other: &Self) -> bool {
        self.addr == other.addr
    }
}
impl Eq for InternalSample {}

impl PartialOrd for InternalSample {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for InternalSample {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.addr.cmp(&other.addr)
    }
}

impl std::fmt::Display for InternalSample {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "#{} - rate:{} addr:{:06X} len:{:06X}", self.id, self.rate, self.addr, self.len)
    }
}

/// Sample bank holding PCM data and internal sample references
#[derive(Clone, Debug)]
pub struct SampleBank {
    pub samples: Vec<InternalSample>,
    pub data: Vec<u8>,
    pub id: i32,
}

impl SampleBank {
    pub fn from_command(command: &VGMCommand) -> Self {
        if !command.is_data_block() {
            panic!(
                "Incorrect sample data declaration at {:06X}!",
                command.origin_offset
            );
        }

        if crate::is_verbose() {
            println!(
                "Add data bank {:06X}:{:02X}",
                command.origin_offset, command.get_data_bank_id()
            );
        }

        let id = command.get_data_bank_id() as i32;
        let data = command.data[7..7 + command.get_data_block_len()].to_vec();
        let data_len = data.len() as i32;

        let mut samples = Vec::new();
        samples.push(InternalSample::new(0, 0, data_len, 0));

        SampleBank { samples, data, id }
    }

    pub fn get_length(&self) -> usize {
        self.data.len()
    }

    pub fn add_block(&mut self, command: &VGMCommand) {
        if !command.is_data_block() {
            return;
        }

        if crate::is_verbose() {
            println!(
                "Concat data block {:06X} to bank {:02X}",
                command.origin_offset, command.get_data_bank_id()
            );
        }

        let old_len = self.get_length();
        let block_len = command.get_data_block_len();
        let mut new_data = vec![0u8; old_len + block_len];
        new_data[..old_len].copy_from_slice(&self.data);
        new_data[old_len..old_len + block_len].copy_from_slice(&command.data[7..7 + block_len]);

        self.samples.push(InternalSample::new(
            self.samples.len() as i32,
            old_len as i32,
            block_len as i32,
            0,
        ));

        self.data = new_data;
    }

    pub fn get_data_block_command(&self) -> VGMCommand {
        let len = self.get_length();
        let mut com_data = vec![0u8; len + 7];

        com_data[0] = 0x67;
        com_data[1] = 0x66;
        com_data[2] = 0x00;
        util::set_i32(&mut com_data, 3, len as i32);
        com_data[7..7 + len].copy_from_slice(&self.data);

        VGMCommand::new(com_data)
    }

    pub fn get_declaration_commands(&self) -> Vec<VGMCommand> {
        let mut result = Vec::new();
        let id = self.id as u8;

        result.push(VGMCommand::from_slice(&[
            crate::vgm_command::STREAM_CONTROL, id, 0x02, 0x00, 0x2A,
        ]));
        result.push(VGMCommand::from_slice(&[
            crate::vgm_command::STREAM_DATA, id, id, 0x01, 0x00,
        ]));

        result
    }

    pub fn get_sample_by_address(&self, address: i32) -> Option<&InternalSample> {
        self.samples.iter().find(|s| s.match_address(address))
    }

    pub fn get_sample_by_address_mut(&mut self, address: i32) -> Option<&mut InternalSample> {
        self.samples.iter_mut().find(|s| s.match_address(address))
    }

    pub fn get_sample_by_id(&self, id: i32) -> Option<&InternalSample> {
        self.samples.iter().find(|s| s.id == id)
    }

    pub fn get_sample_by_id_mut(&mut self, id: i32) -> Option<&mut InternalSample> {
        self.samples.iter_mut().find(|s| s.id == id)
    }

    pub fn add_sample(&mut self, address: i32, len: i32, rate: i32) -> usize {
        let mut adj_len = len;

        // end outside bank ? --> bound it to bank size
        if (address + adj_len) > self.get_length() as i32 {
            adj_len = self.get_length() as i32 - address;
        }

        // search for existing sample at this address
        let found_idx = self.samples.iter().position(|s| s.match_address(address));

        if let Some(idx) = found_idx {
            let delta_addr = address - self.samples[idx].addr;
            adj_len -= delta_addr;
            if (self.samples[idx].addr + adj_len) > self.get_length() as i32 {
                adj_len = self.get_length() as i32 - self.samples[idx].addr;
            }

            // new defined sample ?
            if self.samples[idx].rate == 0 {
                if crate::is_verbose() {
                    println!(
                        "Sample #{} confirmed [{:06X}-{:06X}]  len: {:06X} --> {:06X}   rate: {} --> {} Hz",
                        self.samples[idx].id, self.samples[idx].addr,
                        self.samples[idx].addr + (adj_len - 1),
                        self.samples[idx].len, adj_len,
                        self.samples[idx].rate, rate,
                    );
                }
                self.samples[idx].rate = rate;
                self.samples[idx].len = adj_len;
            } else {
                // adjust sample info
                if util::is_diff_rate(self.samples[idx].rate, rate) {
                    if crate::is_verbose() {
                        print!(
                            "Sample #{} changed   [{:06X}-{:06X}]  rate: {} --> {}",
                            self.samples[idx].id, self.samples[idx].addr,
                            self.samples[idx].addr + (self.samples[idx].len - 1),
                            self.samples[idx].rate, rate,
                        );
                    }
                    // smooth sample rate change
                    self.samples[idx].rate = (self.samples[idx].rate + rate) / 2;
                    if crate::is_verbose() {
                        println!(" - adjusted to {} Hz", self.samples[idx].rate);
                    }
                }

                // adjust length only if longer
                if self.samples[idx].len < adj_len {
                    if crate::is_verbose() {
                        println!(
                            "Sample #{} modified  [{:06X}-{:06X}]  len: {:06X} --> {:06X}",
                            self.samples[idx].id, self.samples[idx].addr,
                            self.samples[idx].addr + (adj_len - 1),
                            self.samples[idx].len, adj_len,
                        );
                    }
                    self.samples[idx].len = adj_len;
                }
            }

            idx
        } else {
            // not found --> create new sample
            let new_id = self.samples.len() as i32;
            let sample = InternalSample::new(new_id, address, adj_len, rate);
            self.samples.push(sample);
            self.samples.len() - 1
        }
    }

    /// Remove unused data, pack samples contiguously, update addresses.
    /// Returns map of old_addr -> new_addr.
    pub fn optimize(&mut self) -> HashMap<i32, i32> {
        let mut sample_addr_changes = HashMap::new();

        // sort samples by address
        self.samples.sort();

        let mut new_data = vec![0u8; self.data.len()];
        let mut addr: i32 = 0;
        let mut cumulated_off: i32 = 0;

        for sample in &mut self.samples {
            if sample.addr <= addr + cumulated_off {
                addr = sample.addr - cumulated_off;
            } else {
                cumulated_off += sample.addr - (addr + cumulated_off);
            }

            let src_start = sample.addr as usize;
            let dst_start = addr as usize;
            let len = sample.len as usize;

            if src_start + len <= self.data.len() && dst_start + len <= new_data.len() {
                new_data[dst_start..dst_start + len].copy_from_slice(&self.data[src_start..src_start + len]);
            }

            sample_addr_changes.insert(sample.addr, addr);
            sample.addr = addr;
            addr += sample.len;
        }

        self.data = new_data[..addr as usize].to_vec();

        sample_addr_changes
    }
}
