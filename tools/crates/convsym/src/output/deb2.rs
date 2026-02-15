/// Output generator for Debug Information format version 2.0 (DEB2).
/// Uses Huffman encoding for symbol names, improved over DEB1.

use std::collections::HashMap;
use std::io::{Seek, SeekFrom, Write};

use anyhow::Result;

trait WriteSeek: Write + Seek {}
impl<T: Write + Seek> WriteSeek for T {}

use crate::bitstream::BitStream;
use crate::huffman;
use crate::opts_parser::{self, RecordTarget};
use crate::output::OutputGenerator;

pub struct Deb2Output;

fn write_be_word(w: &mut impl Write, val: u16) -> Result<()> {
    w.write_all(&val.to_be_bytes())?;
    Ok(())
}

fn write_word(w: &mut impl Write, val: u16) -> Result<()> {
    w.write_all(&val.to_le_bytes())?;
    Ok(())
}

fn swap32(x: u32) -> u32 {
    (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | (x << 24)
}

impl OutputGenerator for Deb2Output {
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
        write_be_word(&mut output.writer, 0xDEB2)?;
        output.pos += 2;

        // Calculate last block
        let last_symbol_offset = symbols.last().unwrap().0;
        let mut last_block = (last_symbol_offset >> 16) as u16;
        if last_block > 0xFF {
            eprintln!(
                "Error: Too many memory blocks (${:X}), truncating to $100 blocks",
                last_block + 1
            );
            last_block = 0xFF;
        }

        let block_count = last_block as usize + 1;
        let block_offsets_size = block_count * 4; // 4 bytes per block offset

        // Write relative pointer to the Huffman table
        write_be_word(&mut output.writer, (block_offsets_size + 2) as u16)?;
        output.pos += 2;

        let loc_block_offsets = output.pos;

        // Reserve space for block offsets
        output.writer.write_all(&vec![0u8; block_offsets_size])?;
        output.pos += block_offsets_size;

        // Build frequency table
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
            if record.code_length > 16 {
                anyhow::bail!(
                    "Some encoding table code lengths exceed 16 bits, try -tolower or -toupper option to reduce entropy"
                );
            }
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
        let mut block_offsets_data: Vec<u32> = vec![0u32; block_count];

        for block in 0..=last_block {
            // Align on even address
            if output.pos & 1 != 0 {
                output.writer.write_all(&[0])?;
                output.pos += 1;
            }

            let loc_block = output.pos;

            let mut symbols_heap = BitStream::new();
            struct SymbolRecord {
                offset: u16,
                symbol_data_ptr: u16,
            }
            let mut offsets_data: Vec<SymbolRecord> = Vec::new();

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

                if symbols_heap.get_current_pos() > 0xFFFF {
                    eprintln!(
                        "Error: Symbols heap for block {:02X} exceeded 64kb limit",
                        block
                    );
                    break;
                }

                if offsets_data.len() > 0x3FFF {
                    eprintln!(
                        "Error: Too many symbols in block {:02X}",
                        block
                    );
                    break;
                }

                offsets_data.push(SymbolRecord {
                    offset: (offset & 0xFFFF) as u16,
                    symbol_data_ptr: symbols_heap.get_current_pos() as u16,
                });

                // Encode symbol characters
                for &b in label.as_bytes() {
                    if let Some((code, code_len)) = char_to_record[b as usize] {
                        symbols_heap.push_code(code, code_len);
                    }
                }
                // Null terminator
                if let Some((code, code_len)) = char_to_record[0] {
                    symbols_heap.push_code(code, code_len);
                    symbols_heap.flush();
                }

                sym_idx += 1;
            }

            if !offsets_data.is_empty() {
                // Write list header: pointer to end of list (where heap starts)
                let list_end_offset = 2 + offsets_data.len() as u16 * 4;
                write_be_word(&mut output.writer, list_end_offset)?;
                output.pos += 2;

                // Write offset/pointer records
                for entry in &offsets_data {
                    write_be_word(&mut output.writer, entry.offset)?;
                    write_be_word(&mut output.writer, entry.symbol_data_ptr)?;
                    output.pos += 4;
                }

                // Write symbols heap data
                output.writer.write_all(symbols_heap.data())?;
                output.pos += symbols_heap.size();

                block_offsets_data[block as usize] =
                    swap32((loc_block - loc_block_offsets) as u32);
            }
        }

        // Write block offsets table at reserved position
        output.writer.seek(SeekFrom::Start(
            (output.base_offset + loc_block_offsets) as u64,
        ))?;
        for off in &block_offsets_data {
            output.writer.write_all(&off.to_ne_bytes())?;
        }

        Ok(())
    }
}

fn should_skip_duplicate(symbols: &[(u32, String)], idx: usize, favor_last: bool) -> bool {
    if favor_last {
        if idx + 1 < symbols.len() && symbols[idx + 1].0 == symbols[idx].0 {
            return true;
        }
    } else {
        if idx > 0 && symbols[idx - 1].0 == symbols[idx].0 {
            return true;
        }
    }
    false
}

struct OutputFile {
    writer: Box<dyn WriteSeek>,
    base_offset: usize,
    pos: usize,
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
