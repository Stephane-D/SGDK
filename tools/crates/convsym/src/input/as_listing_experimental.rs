/// Input parser for AS listing format (experimental).

use std::io::{BufRead, BufReader, Read};

use anyhow::Result;

use crate::input::InputParser;
use crate::symbol_table::SymbolTable;

pub struct AsListingExperimental;

fn is_hex_char(c: u8) -> bool {
    (c.wrapping_sub(b'0') < 10) || (c.wrapping_sub(b'A') < 6)
}

fn is_digit(c: u8) -> bool {
    c.wrapping_sub(b'0') < 10
}

fn decode_hex(s: &[u8]) -> u32 {
    let mut offset: u32 = 0;
    for &c in s {
        let digit = if c.wrapping_sub(b'0') < 10 {
            (c - b'0') as u32
        } else {
            (c - b'A' + 10) as u32
        };
        offset = offset * 0x10 + digit;
    }
    offset
}

impl InputParser for AsListingExperimental {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()> {
        if !opts.is_empty() {
            eprintln!("Warning: -inopt is not supported by this parser");
        }
        let _ = opts;

        let reader: Box<dyn Read> = if file_name == "-" {
            Box::new(std::io::stdin())
        } else {
            Box::new(std::fs::File::open(file_name)?)
        };
        let reader = BufReader::new(reader);

        let mut last_symbol_offset: u32 = u32::MAX;

        for line_result in reader.lines() {
            let line = match line_result {
                Ok(l) => l,
                Err(_) => break,
            };
            let bytes = line.as_bytes();
            let len = bytes.len();
            let mut pos = 0;

            // Check for file indentation number: "(d)"
            if pos < len && bytes[pos] == b'(' {
                pos += 1;
                let mut found_digits = false;
                while pos < len && is_digit(bytes[pos]) {
                    found_digits = true;
                    pos += 1;
                }
                if !found_digits || pos >= len || bytes[pos] != b')' {
                    continue;
                }
                pos += 1;
            }

            // Ensure line has proper line number: "ddddd/"
            {
                while pos < len && bytes[pos] == b' ' {
                    pos += 1;
                }
                let mut found_digits = false;
                while pos < len && is_digit(bytes[pos]) {
                    found_digits = true;
                    pos += 1;
                }
                if !found_digits || pos >= len || bytes[pos] != b'/' {
                    continue;
                }
                pos += 1;
            }

            // Ensure line has proper offset: "hhhh :"
            while pos < len && bytes[pos] == b' ' {
                pos += 1;
            }
            let offset_start = pos;
            {
                let mut found_hex = false;
                while pos < len && is_hex_char(bytes[pos]) {
                    found_hex = true;
                    pos += 1;
                }
                if pos >= len || !found_hex {
                    continue;
                }
            }
            let offset_end = pos;
            pos += 1; // skip separator
            while pos < len && bytes[pos] == b' ' {
                pos += 1;
            }
            if pos >= len || bytes[pos] != b':' {
                continue;
            }
            pos += 1;

            // Check if line matches a label definition
            if pos < len && bytes[pos] == b' ' && pos + 1 < len && bytes[pos + 1] == b'(' {
                continue;
            }

            // Align to column 40
            while pos < len && (pos - 0) < 40 {
                pos += 1;
            }
            // In the original, it aligns to column 40 from the start of sBuffer
            // We approximate by skipping to a reasonable position
            let col40 = if 40 < len { 40 } else { pos };
            pos = std::cmp::max(pos, col40);

            // Skip spaces/tabs
            while pos < len && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                pos += 1;
            }
            let label_start = pos;

            // First character should be a latin letter
            if pos >= len
                || !((bytes[pos].wrapping_sub(b'A') < 26) || (bytes[pos].wrapping_sub(b'a') < 26))
            {
                continue;
            }
            pos += 1;

            // Other characters: letters, digits, or '_'
            while pos < len
                && ((bytes[pos].wrapping_sub(b'A') < 26)
                    || (bytes[pos].wrapping_sub(b'a') < 26)
                    || (bytes[pos].wrapping_sub(b'0') < 10)
                    || bytes[pos] == b'_')
            {
                pos += 1;
            }

            // Must end with ':'
            if pos >= len || bytes[pos] != b':' {
                continue;
            }
            let label_end = pos;
            pos += 1;

            let offset = decode_hex(&bytes[offset_start..offset_end]);
            let label = String::from_utf8_lossy(&bytes[label_start..label_end]).to_string();

            // Check if offset is monotonically increasing
            if last_symbol_offset == u32::MAX || offset >= last_symbol_offset {
                // Check for ds. or rs. after label
                while pos < len && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                    pos += 1;
                }
                if pos + 2 < len
                    && ((bytes[pos] == b'd' && bytes[pos + 1] == b's' && bytes[pos + 2] == b'.')
                        || (bytes[pos] == b'r'
                            && bytes[pos + 1] == b's'
                            && bytes[pos + 2] == b'.'))
                {
                    continue;
                }

                if symbol_table.add(offset, &label) {
                    last_symbol_offset = offset;
                }
            } else if symbol_table.debug {
                eprintln!(
                    "Symbol {} at offset {:X} ignored: offset less than previous",
                    label, offset
                );
            }
        }

        Ok(())
    }
}
