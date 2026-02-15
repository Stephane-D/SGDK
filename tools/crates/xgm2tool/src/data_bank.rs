use crate::vgm_command::VGMCommand;

/// Raw data bank extracted from a VGM data block command
#[derive(Clone, Debug)]
pub struct DataBank {
    pub id: u8,
    pub data: Vec<u8>,
}

impl DataBank {
    pub fn new(command: &VGMCommand) -> Self {
        assert!(command.is_data_block(), "Incorrect sample data declaration at {:06X}!", command.origin_offset());
        let id = command.get_data_bank_id();
        let len = command.get_data_block_len();
        let data = command.data[7..7 + len].to_vec();
        DataBank { id, data }
    }

    pub fn get_sample(&self, offset: usize) -> u8 {
        self.data[offset]
    }
}

use crate::command::CommandTrait;
