/// Symbol table: multimap of (offset -> label), with offset conversion options.

use std::collections::BTreeMap;
use std::collections::HashMap;

/// Options for converting raw offsets to final offsets.
#[derive(Debug, Clone)]
pub struct OffsetConversionOptions {
    pub base_offset: u32,
    pub offset_left_boundary: u32,
    pub offset_right_boundary: u32,
    pub offset_mask: u32,
}

impl Default for OffsetConversionOptions {
    fn default() -> Self {
        Self {
            base_offset: 0,
            offset_left_boundary: 0,
            offset_right_boundary: 0x3FFFFF,
            offset_mask: 0xFFFFFF,
        }
    }
}

/// Table for resolving symbol names to offsets (for -org @Symbol / -ref @Symbol).
pub type SymbolToOffsetResolveTable = HashMap<String, u32>;

/// Symbol table storing (offset -> label) mappings.
/// Uses BTreeMap for deterministic ordering (sorted by offset).
pub struct SymbolTable {
    pub options: OffsetConversionOptions,
    pub resolve_table: SymbolToOffsetResolveTable,
    pub symbols: BTreeMap<u32, Vec<String>>,
    pub debug: bool,
}

impl SymbolTable {
    pub fn new(options: OffsetConversionOptions, resolve_table: SymbolToOffsetResolveTable) -> Self {
        Self {
            options,
            resolve_table,
            symbols: BTreeMap::new(),
            debug: false,
        }
    }

    /// Add a symbol with offset conversion and range filtering.
    /// Returns true if the symbol was inserted.
    pub fn add(&mut self, offset: u32, label: &str) -> bool {
        let corrected_offset =
            offset.wrapping_sub(self.options.base_offset) & self.options.offset_mask;

        // Resolve symbol names referenced in options (e.g. -ref @MySymbol)
        if !self.resolve_table.is_empty() {
            if let Some(entry) = self.resolve_table.get_mut(label) {
                if *entry == u32::MAX - 1 {
                    // sentinel value for "unresolved"
                    *entry = corrected_offset;
                    if self.debug {
                        eprintln!("Resolved requested symbol offset: {:X}", corrected_offset);
                    }
                }
            }
        }

        // Range check
        if corrected_offset < self.options.offset_left_boundary
            || corrected_offset > self.options.offset_right_boundary
        {
            return false;
        }

        if self.debug {
            eprintln!("Adding symbol: {}", label);
        }

        self.symbols
            .entry(corrected_offset)
            .or_default()
            .push(label.to_string());

        true
    }

    /// Returns a flattened multimap-like iterator: (offset, label) pairs, sorted by offset.
    pub fn iter(&self) -> impl Iterator<Item = (u32, &str)> {
        self.symbols
            .iter()
            .flat_map(|(offset, labels)| labels.iter().map(move |l| (*offset, l.as_str())))
    }

    /// Returns the symbols as a BTreeMap<u32, Vec<String>> for output processing.
    /// This is the "multimap" view used by output generators.
    pub fn to_multimap(&self) -> Vec<(u32, String)> {
        self.symbols
            .iter()
            .flat_map(|(offset, labels)| labels.iter().map(move |l| (*offset, l.clone())))
            .collect()
    }

    /// Check if empty.
    pub fn is_empty(&self) -> bool {
        self.symbols.is_empty()
    }

    /// Apply uppercase transformation to all symbol names.
    pub fn to_upper(&mut self) {
        for labels in self.symbols.values_mut() {
            for label in labels.iter_mut() {
                *label = label.to_uppercase();
            }
        }
    }

    /// Apply lowercase transformation to all symbol names.
    pub fn to_lower(&mut self) {
        for labels in self.symbols.values_mut() {
            for label in labels.iter_mut() {
                *label = label.to_lowercase();
            }
        }
    }

    /// Add prefix to all symbol names.
    pub fn add_prefix(&mut self, prefix: &str) {
        for labels in self.symbols.values_mut() {
            for label in labels.iter_mut() {
                label.insert_str(0, prefix);
            }
        }
    }

    /// Filter symbols using a regex pattern.
    /// If `exclude` is true, matching symbols are removed; otherwise, non-matching are removed.
    pub fn filter_regex(&mut self, pattern: &regex::Regex, exclude: bool) {
        for labels in self.symbols.values_mut() {
            labels.retain(|label| {
                let matched = pattern.is_match(label);
                if exclude {
                    !matched
                } else {
                    matched
                }
            });
        }
        // Remove empty entries
        self.symbols.retain(|_, labels| !labels.is_empty());
    }
}
