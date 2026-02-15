/// Input parser for AS assembler listing format.

use std::collections::HashMap;
use std::io::{BufRead, BufReader, Read};

use anyhow::Result;

use crate::input::InputParser;
use crate::opts_parser::{self, RecordTarget};
use crate::symbol_table::SymbolTable;

pub struct AsListing;

fn is_hex_char(c: u8) -> bool {
    (c.wrapping_sub(b'0') < 10) || (c.wrapping_sub(b'A') < 6)
}

fn is_start_of_label(c: u8) -> bool {
    (c.wrapping_sub(b'A') < 26) || (c.wrapping_sub(b'a') < 26) || c == b'_'
}

fn is_label_char(c: u8, process_locals: bool, local_sym: u8) -> bool {
    (c.wrapping_sub(b'A') < 26)
        || (c.wrapping_sub(b'a') < 26)
        || (process_locals && c == local_sym)
        || (c.wrapping_sub(b'0') < 10)
        || c == b'_'
}

fn is_whitespace(c: u8) -> bool {
    c == b' ' || c == b'\t'
}

impl InputParser for AsListing {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()> {
        // Options
        let mut process_local_labels = true;
        let mut ignore_internal_symbols = true;
        let mut local_label_symbol = '.';

        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("localJoin", RecordTarget::Char(&mut local_label_symbol));
            records.insert(
                "processLocals",
                RecordTarget::Bool(&mut process_local_labels),
            );
            records.insert(
                "ignoreInternalSymbols",
                RecordTarget::Bool(&mut ignore_internal_symbols),
            );
            opts_parser::parse(opts, &mut records);
        }

        let local_sym_byte = local_label_symbol as u8;

        let reader: Box<dyn Read> = if file_name == "-" {
            Box::new(std::io::stdin())
        } else {
            Box::new(std::fs::File::open(file_name)?)
        };
        let reader = BufReader::new(reader);

        let mut found_symbol_table = false;

        for (_line_counter, line_result) in reader.lines().enumerate() {
            let line = match line_result {
                Ok(l) => l,
                Err(_) => break,
            };

            if line.len() < 8 {
                continue;
            }

            // Phase 1: Search for symbol table header
            if !found_symbol_table {
                let trimmed = line.trim_start();
                if trimmed.starts_with("symbol table") || trimmed.starts_with("Symbol Table") {
                    found_symbol_table = true;
                }
                continue;
            }

            // Phase 2: Parse the symbol table
            // Look for '|' separated cells
            let parts: Vec<&str> = line.split('|').collect();
            if parts.len() <= 1 {
                continue;
            }

            for part in &parts[..parts.len() - 1] {
                if let Some((offset, label)) =
                    parse_symbol_table_entry(part, process_local_labels, local_sym_byte)
                {
                    if ignore_internal_symbols && label.starts_with("__") {
                        continue;
                    }
                    symbol_table.add(offset, &label);
                }
            }
        }

        if !found_symbol_table {
            anyhow::bail!("Couldn't find symbols table");
        }

        Ok(())
    }
}

/// Parse a single cell from the AS listing symbol table.
/// Format: "  LABEL : OFFSET C"
fn parse_symbol_table_entry(
    entry: &str,
    process_locals: bool,
    local_sym: u8,
) -> Option<(u32, String)> {
    let bytes = entry.as_bytes();
    let mut pos = 0;

    // Skip whitespace and '*'
    while pos < bytes.len() && (is_whitespace(bytes[pos]) || bytes[pos] == b'*') {
        pos += 1;
    }

    // Capture label
    if pos >= bytes.len() || !is_start_of_label(bytes[pos]) {
        return None;
    }
    let label_start = pos;
    pos += 1;
    while pos < bytes.len() && is_label_char(bytes[pos], process_locals, local_sym) {
        pos += 1;
    }
    let label_end = pos;

    // Skip " : "
    if pos >= bytes.len() || bytes[pos] != b' ' {
        return None;
    }
    pos += 1;
    if pos >= bytes.len() || bytes[pos] != b':' {
        return None;
    }
    pos += 1;
    while pos < bytes.len() && is_whitespace(bytes[pos]) {
        pos += 1;
    }

    // Capture offset
    if pos >= bytes.len() || !is_hex_char(bytes[pos]) {
        return None;
    }
    let mut offset: u32 = 0;
    while pos < bytes.len() && is_hex_char(bytes[pos]) {
        let c = bytes[pos];
        let digit = if c.wrapping_sub(b'0') < 10 {
            (c - b'0') as u32
        } else {
            (c - b'A' + 10) as u32
        };
        offset = offset * 0x10 + digit;
        pos += 1;
    }

    // Check label type: must be " C"
    if pos >= bytes.len() || bytes[pos] != b' ' {
        return None;
    }
    pos += 1;
    if pos >= bytes.len() || bytes[pos] != b'C' {
        return None;
    }

    let label = String::from_utf8_lossy(&bytes[label_start..label_end]).to_string();
    Some((offset, label))
}
