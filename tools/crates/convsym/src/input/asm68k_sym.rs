/// Input parser for ASM68K compiler's binary symbol format.

use std::collections::BTreeMap;
use std::collections::HashMap;
use std::io::{Read, Seek, SeekFrom};

use anyhow::Result;

use crate::input::InputParser;
use crate::opts_parser::{self, RecordTarget};
use crate::symbol_table::SymbolTable;

pub struct Asm68kSym;

impl InputParser for Asm68kSym {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()> {
        let data = if file_name == "-" {
            let mut buf = Vec::new();
            std::io::stdin().read_to_end(&mut buf)?;
            buf
        } else {
            std::fs::read(file_name)?
        };

        let mut cursor = std::io::Cursor::new(&data);

        // Options
        let mut process_locals = true;
        let mut local_label_symbol = '@';
        let mut local_label_ref = '.';

        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("localSign", RecordTarget::Char(&mut local_label_symbol));
            records.insert("localJoin", RecordTarget::Char(&mut local_label_ref));
            records.insert("processLocals", RecordTarget::Bool(&mut process_locals));
            opts_parser::parse(opts, &mut records);
        }

        // Read all symbols (they may be out of order)
        let mut unfiltered: BTreeMap<u32, Vec<String>> = BTreeMap::new();

        cursor.seek(SeekFrom::Start(0x0008))?;

        loop {
            // Read 32-bit offset (big-endian, same as original binary format)
            let mut buf4 = [0u8; 4];
            if cursor.read_exact(&mut buf4).is_err() {
                break;
            }
            let offset = u32::from_be_bytes(buf4);

            // Skip 1 byte
            let mut skip = [0u8; 1];
            if cursor.read_exact(&mut skip).is_err() {
                break;
            }

            // Read label length
            let mut len_buf = [0u8; 1];
            if cursor.read_exact(&mut len_buf).is_err() {
                break;
            }
            let label_length = len_buf[0] as usize;

            // Read label
            let mut label_buf = vec![0u8; label_length];
            if cursor.read_exact(&mut label_buf).is_err() {
                break;
            }
            let label = String::from_utf8_lossy(&label_buf).to_string();

            unfiltered.entry(offset).or_default().push(label);
        }

        // Process symbols in order
        let all_entries: Vec<(u32, String)> = unfiltered
            .into_iter()
            .flat_map(|(off, labels)| labels.into_iter().map(move |l| (off, l)))
            .collect();

        let mut last_global_label = if let Some((_, ref l)) = all_entries.first() {
            l.clone()
        } else {
            String::new()
        };

        for (offset, label) in &all_entries {
            if label.starts_with(local_label_symbol) {
                if !process_locals {
                    continue;
                }
                let full_label = format!(
                    "{}{}{}",
                    last_global_label,
                    local_label_ref,
                    &label[local_label_symbol.len_utf8()..]
                );
                symbol_table.add(*offset, &full_label);
            } else {
                last_global_label = label.clone();
                symbol_table.add(*offset, label);
            }
        }

        Ok(())
    }
}
