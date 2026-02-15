//! Array and byte manipulation utilities.

use byteorder::{BigEndian, ByteOrder, LittleEndian};

/// Read a big-endian u16 from a byte slice.
pub fn read_u16_be(data: &[u8], offset: usize) -> u16 {
    BigEndian::read_u16(&data[offset..])
}

/// Read a little-endian u16 from a byte slice.
pub fn read_u16_le(data: &[u8], offset: usize) -> u16 {
    LittleEndian::read_u16(&data[offset..])
}

/// Read a big-endian u32 from a byte slice.
pub fn read_u32_be(data: &[u8], offset: usize) -> u32 {
    BigEndian::read_u32(&data[offset..])
}

/// Read a little-endian u32 from a byte slice.
pub fn read_u32_le(data: &[u8], offset: usize) -> u32 {
    LittleEndian::read_u32(&data[offset..])
}

/// Write a big-endian u16 to a byte slice.
pub fn write_u16_be(data: &mut [u8], offset: usize, value: u16) {
    BigEndian::write_u16(&mut data[offset..], value);
}

/// Write a little-endian u16 to a byte slice.
pub fn write_u16_le(data: &mut [u8], offset: usize, value: u16) {
    LittleEndian::write_u16(&mut data[offset..], value);
}

/// Write a big-endian u32 to a byte slice.
pub fn write_u32_be(data: &mut [u8], offset: usize, value: u32) {
    BigEndian::write_u32(&mut data[offset..], value);
}

/// Pad a byte array to word (2-byte) alignment.
pub fn pad_to_word(data: &[u8]) -> Vec<u8> {
    if data.len() & 1 == 1 {
        let mut result = Vec::with_capacity(data.len() + 1);
        result.extend_from_slice(data);
        result.push(0);
        result
    } else {
        data.to_vec()
    }
}

/// Convert a byte array to an array of u16 (big-endian).
pub fn bytes_to_u16_be(data: &[u8]) -> Vec<u16> {
    data.chunks_exact(2)
        .map(|c| u16::from_be_bytes([c[0], c[1]]))
        .collect()
}

/// Convert a byte array to an array of u16 (little-endian).
pub fn bytes_to_u16_le(data: &[u8]) -> Vec<u16> {
    data.chunks_exact(2)
        .map(|c| u16::from_le_bytes([c[0], c[1]]))
        .collect()
}
