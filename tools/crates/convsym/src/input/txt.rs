/// Input parser for TXT files with configurable format strings.

use std::collections::HashMap;
use std::io::{BufRead, BufReader, Read};

use anyhow::Result;

use crate::input::InputParser;
use crate::opts_parser::{self, RecordTarget};
use crate::symbol_table::SymbolTable;

pub struct TxtInput;

impl InputParser for TxtInput {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()> {
        // Options
        let mut line_format = String::from("%s %X");
        let mut offset_first = false;

        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("fmt", RecordTarget::Str(&mut line_format));
            records.insert("offsetFirst", RecordTarget::Bool(&mut offset_first));
            opts_parser::parse(opts, &mut records);
        }

        let num_specifiers = line_format.chars().filter(|&c| c == '%').count();
        if num_specifiers < 2 {
            eprintln!("Warning: Line format string likely has too few arguments (try '%s %X')");
        }

        let reader: Box<dyn Read> = if file_name == "-" {
            Box::new(std::io::stdin())
        } else {
            Box::new(std::fs::File::open(file_name)?)
        };
        let reader = BufReader::new(reader);

        // Build a simple scanf-like parser from the format string
        // We support: %s (whitespace-delimited string), %X (hex number), %d (decimal number)
        let format_parts = parse_format_string(&line_format);

        for (_line_num, line_result) in reader.lines().enumerate() {
            let line = match line_result {
                Ok(l) => l,
                Err(_) => break,
            };

            if let Some((label, offset)) =
                scan_line(&line, &format_parts, offset_first)
            {
                symbol_table.add(offset, &label);
            }
        }

        Ok(())
    }
}

#[derive(Debug)]
enum FormatPart {
    Literal(String),
    StringSpec,    // %s
    HexSpec,       // %X or %x
    DecimalSpec,   // %d
}

fn parse_format_string(fmt: &str) -> Vec<FormatPart> {
    let mut parts = Vec::new();
    let mut chars = fmt.chars().peekable();
    let mut literal = String::new();

    while let Some(c) = chars.next() {
        if c == '%' {
            if !literal.is_empty() {
                parts.push(FormatPart::Literal(literal.clone()));
                literal.clear();
            }
            match chars.next() {
                Some('s') | Some('S') => parts.push(FormatPart::StringSpec),
                Some('X') | Some('x') => parts.push(FormatPart::HexSpec),
                Some('d') | Some('D') => parts.push(FormatPart::DecimalSpec),
                Some('%') => literal.push('%'),
                Some(other) => {
                    // Unknown specifier, treat as literal
                    literal.push('%');
                    literal.push(other);
                }
                None => literal.push('%'),
            }
        } else {
            literal.push(c);
        }
    }
    if !literal.is_empty() {
        parts.push(FormatPart::Literal(literal));
    }
    parts
}

fn scan_line(
    line: &str,
    format_parts: &[FormatPart],
    _offset_first: bool,
) -> Option<(String, u32)> {
    let mut pos = 0;
    let bytes = line.as_bytes();
    let mut string_val: Option<String> = None;
    let mut offset_val: Option<u32> = None;

    for part in format_parts {
        match part {
            FormatPart::Literal(lit) => {
                // Match literal characters, treating whitespace as "skip whitespace"
                for lc in lit.chars() {
                    if lc.is_whitespace() {
                        while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                            pos += 1;
                        }
                    } else {
                        if pos >= bytes.len() || bytes[pos] != lc as u8 {
                            return None;
                        }
                        pos += 1;
                    }
                }
            }
            FormatPart::StringSpec => {
                // Skip leading whitespace
                while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                    pos += 1;
                }
                let start = pos;
                while pos < bytes.len() && bytes[pos] != b' ' && bytes[pos] != b'\t' {
                    pos += 1;
                }
                if pos == start {
                    return None;
                }
                string_val =
                    Some(String::from_utf8_lossy(&bytes[start..pos]).to_string());
            }
            FormatPart::HexSpec => {
                while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                    pos += 1;
                }
                // Skip optional "0x" or "$" prefix
                if pos + 1 < bytes.len() && bytes[pos] == b'0' && (bytes[pos + 1] == b'x' || bytes[pos + 1] == b'X') {
                    pos += 2;
                } else if pos < bytes.len() && bytes[pos] == b'$' {
                    pos += 1;
                }
                let start = pos;
                let mut val: u32 = 0;
                while pos < bytes.len() {
                    let c = bytes[pos];
                    if c.wrapping_sub(b'0') < 10 {
                        val = val * 16 + (c - b'0') as u32;
                    } else if c.wrapping_sub(b'A') < 6 {
                        val = val * 16 + (c - b'A' + 10) as u32;
                    } else if c.wrapping_sub(b'a') < 6 {
                        val = val * 16 + (c - b'a' + 10) as u32;
                    } else {
                        break;
                    }
                    pos += 1;
                }
                if pos == start {
                    return None;
                }
                offset_val = Some(val);
            }
            FormatPart::DecimalSpec => {
                while pos < bytes.len() && (bytes[pos] == b' ' || bytes[pos] == b'\t') {
                    pos += 1;
                }
                let start = pos;
                let mut val: u32 = 0;
                while pos < bytes.len() && bytes[pos].wrapping_sub(b'0') < 10 {
                    val = val * 10 + (bytes[pos] - b'0') as u32;
                    pos += 1;
                }
                if pos == start {
                    return None;
                }
                offset_val = Some(val);
            }
        }
    }

    match (string_val, offset_val) {
        (Some(s), Some(o)) => Some((s, o)),
        _ => None,
    }
}
