pub mod asm68k_sym;
pub mod asm68k_listing;
pub mod as_listing;
pub mod as_listing_experimental;
pub mod log_input;
pub mod txt;

use crate::symbol_table::SymbolTable;
use anyhow::Result;

/// Trait for all input format parsers.
pub trait InputParser {
    fn parse(&self, symbol_table: &mut SymbolTable, file_name: &str, opts: &str) -> Result<()>;
}

/// Get an input parser by format name.
pub fn get_input_parser(name: &str) -> Result<Box<dyn InputParser>> {
    match name {
        "asm68k_sym" => Ok(Box::new(asm68k_sym::Asm68kSym)),
        "asm68k_lst" => Ok(Box::new(asm68k_listing::Asm68kListing)),
        "as_lst" => Ok(Box::new(as_listing::AsListing)),
        "as_lst_exp" => Ok(Box::new(as_listing_experimental::AsListingExperimental)),
        "log" => Ok(Box::new(log_input::LogInput)),
        "txt" => Ok(Box::new(txt::TxtInput)),
        _ => anyhow::bail!("Unknown input format specifier: {}", name),
    }
}
