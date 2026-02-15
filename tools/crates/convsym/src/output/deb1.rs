/// Output generator for Debug Information format version 1.0 (DEB1).
/// Uses Huffman encoding for symbol names.

use std::collections::HashMap;
use std::io::{Seek, SeekFrom, Write};

use anyhow::Result;

trait WriteSeek: Write + Seek {}
impl<T: Write + Seek> WriteSeek for T {}

use crate::bitstream::BitStream;
use crate::huffman;
use crate::opts_parser::{self, RecordTarget};
use crate::output::OutputGenerator;

pub struct Deb1Output;

/// Helper to write big-endian values to a writer.
fn write_be_word(w: &mut impl Write, val: u16) -> Result<()> {
    w.write_all(&val.to_be_bytes())?;
    Ok(())
}

fn write_word(w: &mut impl Write, val: u16) -> Result<()> {
    w.write_all(&val.to_le_bytes())?;
    Ok(())
}

impl OutputGenerator for Deb1Output {
    fn generate(
        &self,
        symbols: &[(u32, String)],
        file_name: &str,
        append_offset: i64,
        pointer_offset: u32,
        opts: &str,
        align_on_append: bool,
    ) -> Result<()> {
        if symbols.is_empty() {
            anyhow::bail!("No symbols to output");
        }

        let mut output = setup_output(file_name, append_offset, pointer_offset, align_on_append)?;

        // Options
        let mut favor_last_labels = false;
        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert(
                "favorLastLabels",
                RecordTarget::Bool(&mut favor_last_labels),
            );
            opts_parser::parse(opts, &mut records);
        }

        // Write format version token
        write_be_word(&mut output.writer, 0xDEB1)?;
        output.pos += 2;

        // Calculate last block
        let last_symbol_offset = symbols.last().unwrap().0;
        let mut last_block = (last_symbol_offset >> 16) as u16;
        if last_block > 63 {
            eprintln!(
                "Error: Too many memory blocks (${:X}), truncating to $40 blocks",
                last_block + 1
            );
            last_block = 0x3F;
        }

        // Reserve space for block offsets tables
        let mut block_offsets = vec![0u16; 0x40];
        let mut data_offsets = vec![0u16; 0x40];

        let loc_block_offsets = output.current_offset();
        // Reserve space: 0x40 * 2 bytes for block_offsets + 0x40 * 2 bytes for data_offsets
        let reserved_size = 0x40 * 2 + 0x40 * 2;
        output.writer.write_all(&vec![0u8; reserved_size])?;
        output.pos += reserved_size;

        // Build frequency table from all symbol names
        let mut freq_table = [0u32; 256];
        for (_, label) in symbols {
            for &b in label.as_bytes() {
                freq_table[b as usize] += 1;
            }
            freq_table[0] += 1; // null terminator
        }

        // Generate Huffman codes
        let codes_table = huffman::encode(&freq_table, 16);

        // Build character-to-record LUT
        let mut char_to_record: [Option<(u32, u8)>; 256] = [None; 256];
        for record in &codes_table {
            char_to_record[record.data as usize] = Some((record.code, record.code_length));

            // Write Huffman table entry
            write_be_word(&mut output.writer, record.code as u16)?;
            output.writer.write_all(&[record.code_length])?;
            output.writer.write_all(&[record.data as u8])?;
            output.pos += 4;
        }
        // Write 0xFFFF terminator
        write_word(&mut output.writer, 0xFFFF)?;
        output.pos += 2;

        // Generate per-block symbol information
        let mut sym_idx = 0;

        for block in 0..=last_block {
            // Align on even address
            if output.current_offset() & 1 != 0 {
                output.writer.write_all(&[0])?;
                output.pos += 1;
            }

            let loc_block = output.current_offset();

            let mut offsets_data: Vec<u16> = Vec::new();
            let mut symbols_data: Vec<u8> = Vec::new();

            while sym_idx < symbols.len() {
                let (offset, ref label) = symbols[sym_idx];
                let sym_block = (offset >> 16) as u16;

                if sym_block > block {
                    break;
                }
                if sym_block < block {
                    sym_idx += 1;
                    continue;
                }

                // Handle duplicate offsets
                if should_skip_duplicate(symbols, sym_idx, favor_last_labels) {
                    sym_idx += 1;
                    continue;
                }

                let mut symbol_heap = BitStream::new();

                // Add offset to offsets block (big-endian)
                offsets_data.push((offset & 0xFFFF) as u16);

                // Encode symbol characters
                for &b in label.as_bytes() {
                    if let Some((code, code_len)) = char_to_record[b as usize] {
                        symbol_heap.push_code(code, code_len);
                    }
                }
                // Null terminator
                if let Some((code, code_len)) = char_to_record[0] {
                    symbol_heap.push_code(code, code_len);
                }

                // Push to data buffer
                symbols_data.push(symbol_heap.size() as u8 + 1);
                symbols_data.extend_from_slice(symbol_heap.data());

                sym_idx += 1;
            }

            if !offsets_data.is_empty() {
                // Add zero offset to finalize
                offsets_data.push(0x0000);

                // Check pointer capacity
                let block_rel = (loc_block - loc_block_offsets) >> 1;
                let data_rel = (loc_block + offsets_data.len() * 2 - loc_block_offsets) >> 1;
                if block_rel > 0xFFFF || data_rel > 0xFFFF {
                    eprintln!(
                        "Error: Block {:02X} exceeds size limits",
                        block
                    );
                    continue;
                }

                block_offsets[block as usize] = swap16(block_rel as u16);
                data_offsets[block as usize] = swap16(data_rel as u16);

                // Write offsets (big-endian)
                for off in &offsets_data {
                    write_be_word(&mut output.writer, *off)?;
                    output.pos += 2;
                }
                // Write symbol data
                output.writer.write_all(&symbols_data)?;
                output.pos += symbols_data.len();
            }
        }

        // Write block offsets table at the reserved position
        output.writer.seek(SeekFrom::Start(
            (output.base_offset + loc_block_offsets) as u64,
        ))?;
        for off in &block_offsets {
            output.writer.write_all(&off.to_ne_bytes())?;
        }
        for off in &data_offsets {
            output.writer.write_all(&off.to_ne_bytes())?;
        }

        Ok(())
    }
}

fn swap16(x: u16) -> u16 {
    (x >> 8) | (x << 8)
}

fn should_skip_duplicate(symbols: &[(u32, String)], idx: usize, favor_last: bool) -> bool {
    if favor_last {
        // Skip if next symbol has the same offset
        if idx + 1 < symbols.len() && symbols[idx + 1].0 == symbols[idx].0 {
            return true;
        }
    } else {
        // Skip if previous symbol has the same offset
        if idx > 0 && symbols[idx - 1].0 == symbols[idx].0 {
            return true;
        }
    }
    false
}

/// Output file wrapper with tracking for append mode, base offset, etc.
struct OutputFile {
    writer: Box<dyn WriteSeek>,
    base_offset: usize,
    pos: usize,
}

impl OutputFile {
    fn current_offset(&self) -> usize {
        self.pos
    }
}

fn setup_output(
    file_name: &str,
    append_offset: i64,
    pointer_offset: u32,
    align_on_append: bool,
) -> Result<OutputFile> {
    if append_offset != 0 {
        let mut file = std::fs::OpenOptions::new()
            .read(true)
            .write(true)
            .open(file_name)?;

        let actual_append;
        if append_offset == -1 {
            // Append to end
            let end = file.seek(SeekFrom::End(0))? as usize;
            actual_append = if align_on_append && (end & 1) != 0 {
                file.write_all(&[0])?;
                end + 1
            } else {
                end
            };
        } else {
            actual_append = append_offset as usize;
            if align_on_append && (actual_append & 1) != 0 {
                eprintln!("Warning: An odd append offset is specified; the offset wasn't auto-aligned.");
            }
            file.seek(SeekFrom::Start(actual_append as u64))?;
        }

        // Write pointer if specified
        if pointer_offset != 0 {
            file.seek(SeekFrom::Start(pointer_offset as u64))?;
            file.write_all(&(actual_append as u32).to_be_bytes())?;
        }

        file.seek(SeekFrom::Start(actual_append as u64))?;

        Ok(OutputFile {
            writer: Box::new(file),
            base_offset: actual_append,
            pos: 0,
        })
    } else {
        let file: Box<dyn WriteSeek> = if file_name == "-" {
            Box::new(std::io::Cursor::new(Vec::new()))
        } else {
            Box::new(std::fs::File::create(file_name)?)
        };
        Ok(OutputFile {
            writer: file,
            base_offset: 0,
            pos: 0,
        })
    }
}
