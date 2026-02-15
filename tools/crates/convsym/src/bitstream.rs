/// BitStream helper for Huffman encoding.
/// Stores bits packed into bytes (MSB-first within each byte).

pub struct BitStream {
    buffer: Vec<u8>,
    current_byte_pos: usize,
    current_bit_pos: u8,
}

impl BitStream {
    pub fn new() -> Self {
        Self {
            buffer: vec![0u8],
            current_byte_pos: 0,
            current_bit_pos: 8,
        }
    }

    /// Returns the current byte position within the stream.
    pub fn get_current_pos(&self) -> usize {
        self.current_byte_pos
    }

    /// Flush current byte and start a new one.
    pub fn flush(&mut self) {
        self.buffer.push(0u8);
        self.current_bit_pos = 8;
        self.current_byte_pos += 1;
    }

    /// Push a code with a given number of bits.
    pub fn push_code(&mut self, code: u32, code_length: u8) {
        if code_length == 0 {
            return;
        }
        if self.current_bit_pos == 0 {
            self.flush();
        }

        let remaining_bits = if code_length > self.current_bit_pos {
            code_length - self.current_bit_pos
        } else {
            0
        };
        let bits_to_write = std::cmp::min(self.current_bit_pos, code_length);
        self.current_bit_pos -= bits_to_write;
        self.buffer[self.current_byte_pos] |= ((code >> remaining_bits) as u8) << self.current_bit_pos;

        if remaining_bits > 0 {
            let mask = (1u32 << remaining_bits) - 1;
            self.push_code(code & mask, remaining_bits);
        }
    }

    /// Returns a slice of the buffer.
    pub fn data(&self) -> &[u8] {
        &self.buffer
    }

    /// Returns the buffer size in bytes.
    pub fn size(&self) -> usize {
        self.buffer.len()
    }
}

impl Default for BitStream {
    fn default() -> Self {
        Self::new()
    }
}
