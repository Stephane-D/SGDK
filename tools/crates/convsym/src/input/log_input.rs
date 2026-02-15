/// Input parser for log files (offset:label format).

use std::collections::HashMap;
use std::io::{BufRead, BufReader, Read};

use anyhow::Result;

use crate::input::InputParser;
use crate::opts_parser::{self, RecordTarget};
use crate::symbol_table::SymbolTable;

pub struct LogInput;

fn is_hex_char(c: u8) -> bool {
    (c.wrapping_sub(b'0') < 10) || (c.wrapping_sub(b'A') < 6) || (c.wrapping_sub(b'a') < 6)
}

fn is_digit(c: u8) -> bool {
    c.wrapping_sub(b'0') < 10
}

impl InputParser for LogInput {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()> {
        // Options
        let mut label_separator = ':';
        let mut use_decimal = false;

        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("separator", RecordTarget::Char(&mut label_separator));
            records.insert("useDecimal", RecordTarget::Bool(&mut use_decimal));
            opts_parser::parse(opts, &mut records);
        }

        let sep_byte = label_separator as u8;

        let reader: Box<dyn Read> = if file_name == "-" {
            Box::new(std::io::stdin())
        } else {
            Box::new(std::fs::File::open(file_name)?)
        };
        let reader = BufReader::new(reader);

        for (_line_num, line_result) in reader.lines().enumerate() {
            let line = match line_result {
                Ok(l) => l,
                Err(_) => break,
            };
            let bytes = line.as_bytes();
            let mut pos = 0;

            // Skip spaces
            while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                pos += 1;
            }

            // Decode offset
            let mut offset: u32 = 0;
            if use_decimal {
                while pos < bytes.len() && is_digit(bytes[pos]) {
                    offset = offset * 10 + (bytes[pos] - b'0') as u32;
                    pos += 1;
                }
            } else {
                while pos < bytes.len() && is_hex_char(bytes[pos]) {
                    let c = bytes[pos];
                    let digit = if c.wrapping_sub(b'0') < 10 {
                        (c - b'0') as u32
                    } else if c.wrapping_sub(b'A') < 6 {
                        (c - b'A' + 10) as u32
                    } else {
                        (c - b'a' + 10) as u32
                    };
                    offset = offset * 0x10 + digit;
                    pos += 1;
                }
            }

            // Check for separator
            if pos >= bytes.len() || bytes[pos] != sep_byte {
                continue;
            }
            pos += 1;

            // Skip spaces
            while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                pos += 1;
            }

            // Fetch label
            let label_start = pos;
            while pos < bytes.len()
                && bytes[pos] != b'\t'
                && bytes[pos] != b' '
                && bytes[pos] != 0
            {
                pos += 1;
            }

            if pos > label_start {
                let label = String::from_utf8_lossy(&bytes[label_start..pos]).to_string();
                symbol_table.add(offset, &label);
            }
        }

        Ok(())
    }
}
