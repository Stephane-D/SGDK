/// Output generator for assembly equates format.

use std::collections::HashMap;
use std::io::Write;

use anyhow::Result;

use crate::opts_parser::{self, RecordTarget};
use crate::output::OutputGenerator;

pub struct AsmOutput;

impl OutputGenerator for AsmOutput {
    fn generate(
        &self,
        symbols: &[(u32, String)],
        file_name: &str,
        append_offset: i64,
        pointer_offset: u32,
        opts: &str,
        align_on_append: bool,
    ) -> Result<()> {
        if append_offset != 0 || pointer_offset != 0 || !align_on_append {
            eprintln!("Warning: Append options aren't supported by the \"asm\" output parser.");
        }

        // Options
        let mut line_format = String::from("%s:\tequ\t$%X");

        if !opts.is_empty() && opts.starts_with('/') {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("fmt", RecordTarget::Str(&mut line_format));
            opts_parser::parse(opts, &mut records);
        } else if !opts.is_empty() {
            line_format = opts.to_string();
        }

        let mut writer: Box<dyn Write> = if file_name == "-" {
            Box::new(std::io::stdout())
        } else {
            Box::new(std::fs::File::create(file_name)?)
        };

        for (offset, label) in symbols {
            // Apply format: replace first %s with label, first %X with offset
            let line = format_line(&line_format, label, *offset);
            writeln!(writer, "{}", line)?;
        }

        Ok(())
    }
}

/// Simple printf-style formatter for "%s" and "%X" specifiers.
fn format_line(fmt: &str, label: &str, offset: u32) -> String {
    let mut result = String::new();
    let mut chars = fmt.chars().peekable();
    let mut s_used = false;
    let mut x_used = false;

    while let Some(c) = chars.next() {
        if c == '%' {
            match chars.next() {
                Some('s') | Some('S') if !s_used => {
                    result.push_str(label);
                    s_used = true;
                }
                Some('X') if !x_used => {
                    result.push_str(&format!("{:X}", offset));
                    x_used = true;
                }
                Some('x') if !x_used => {
                    result.push_str(&format!("{:x}", offset));
                    x_used = true;
                }
                Some('%') => result.push('%'),
                Some(other) => {
                    result.push('%');
                    result.push(other);
                }
                None => result.push('%'),
            }
        } else {
            result.push(c);
        }
    }
    result
}
