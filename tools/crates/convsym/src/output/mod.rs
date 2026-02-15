pub mod asm;
pub mod deb1;
pub mod deb2;
pub mod log_output;

use anyhow::Result;

/// Trait for all output format generators.
pub trait OutputGenerator {
    fn generate(
        &self,
        symbols: &[(u32, String)],
        file_name: &str,
        append_offset: i64,
        pointer_offset: u32,
        opts: &str,
        align_on_append: bool,
    ) -> Result<()>;
}

/// Get an output generator by format name.
pub fn get_output_generator(name: &str) -> Result<Box<dyn OutputGenerator>> {
    match name {
        "asm" => Ok(Box::new(asm::AsmOutput)),
        "deb1" => Ok(Box::new(deb1::Deb1Output)),
        "deb2" => Ok(Box::new(deb2::Deb2Output)),
        "log" => Ok(Box::new(log_output::LogOutput)),
        _ => anyhow::bail!("Unknown output format specifier: {}", name),
    }
}
