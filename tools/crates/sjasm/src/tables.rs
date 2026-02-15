use crate::assembler::{StructMemb, LABMAX};
use std::collections::HashMap;
use std::io::Write;

// ============================================================
// Label table
// ============================================================

#[derive(Debug, Clone)]
struct LabEntry {
    name: String,
    value: i64,
    used: i64,
}

pub struct LabTab {
    entries: Vec<LabEntry>,
    index: HashMap<String, usize>,
}

impl LabTab {
    pub fn new() -> Self {
        LabTab {
            entries: Vec::new(),
            index: HashMap::new(),
        }
    }

    pub fn insert(&mut self, name: &str, value: i64) -> bool {
        if self.index.contains_key(name) {
            return false;
        }
        let idx = self.entries.len();
        self.entries.push(LabEntry {
            name: name.to_string(),
            value,
            used: -1,
        });
        self.index.insert(name.to_string(), idx);
        true
    }

    pub fn lookup(&mut self, name: &str, pass: i32) -> Option<i64> {
        if let Some(&idx) = self.index.get(name) {
            if pass == 2 {
                self.entries[idx].used += 1;
            }
            Some(self.entries[idx].value)
        } else {
            None
        }
    }

    pub fn dump(&self, list_fp: &mut dyn Write) {
        let _ = writeln!(list_fp, "\nvalue      label");
        let _ = writeln!(list_fp, "-------- - -----------------------------------------------------------");
        for entry in &self.entries {
            let used_marker = if entry.used != 0 { ' ' } else { 'X' };
            let _ = writeln!(list_fp, "{:08X} {} {}", entry.value as u32, used_marker, entry.name);
        }
    }

    pub fn dumpsym(&self, filename: &str) {
        let mut f = match std::fs::File::create(filename) {
            Ok(f) => f,
            Err(e) => {
                eprintln!("Error opening {}: {}", filename, e);
                return;
            }
        };
        for entry in &self.entries {
            if !entry.name.starts_with(|c: char| c.is_ascii_digit()) {
                let _ = writeln!(f, "{}: equ {:08X}h", entry.name, entry.value as u32);
            }
        }
    }
}

// ============================================================
// Local label table
// ============================================================

#[derive(Debug, Clone)]
struct LokLabEntry {
    regel: i64, // global line where defined
    nummer: i64,
    value: i64,
}

pub struct LokLabTab {
    entries: Vec<LokLabEntry>,
}

impl LokLabTab {
    pub fn new() -> Self {
        LokLabTab { entries: Vec::new() }
    }

    pub fn insert(&mut self, nummer: i64, value: i64, gcurlin: i64) {
        self.entries.push(LokLabEntry {
            regel: gcurlin,
            nummer,
            value,
        });
    }

    /// Search forward for local label number
    pub fn zoekf(&self, nnum: i64, gcurlin: i64) -> Option<i64> {
        for entry in &self.entries {
            if entry.regel > gcurlin && entry.nummer == nnum {
                return Some(entry.value);
            }
        }
        None
    }

    /// Search backward for local label number
    pub fn zoekb(&self, nnum: i64, gcurlin: i64) -> Option<i64> {
        for entry in self.entries.iter().rev() {
            if entry.regel <= gcurlin && entry.nummer == nnum {
                return Some(entry.value);
            }
        }
        None
    }
}

// ============================================================
// Define table
// ============================================================

pub struct DefineTab {
    defs: HashMap<String, String>,
}

impl DefineTab {
    pub fn new() -> Self {
        DefineTab {
            defs: HashMap::new(),
        }
    }

    pub fn add(&mut self, name: &str, replacement: &str) -> bool {
        if self.defs.contains_key(name) {
            return false; // duplicate
        }
        // trim leading whitespace and trailing newline from replacement
        let repl = replacement.trim_start().trim_end_matches(|c| c == '\n' || c == '\r');
        self.defs.insert(name.to_string(), repl.to_string());
        true
    }

    pub fn get(&self, name: &str) -> Option<&str> {
        self.defs.get(name).map(|s| s.as_str())
    }

    pub fn exists(&self, name: &str) -> bool {
        self.defs.contains_key(name)
    }
}

// ============================================================
// Macro-scoped define table
// ============================================================

pub struct MacDefineTab {
    defs: Vec<(String, String)>,
    snapshot_len: usize,
}

impl MacDefineTab {
    pub fn new() -> Self {
        MacDefineTab {
            defs: Vec::new(),
            snapshot_len: 0,
        }
    }

    pub fn init(&mut self) {
        self.defs.clear();
        self.snapshot_len = 0;
    }

    pub fn add(&mut self, name: &str, replacement: &str) {
        let repl = replacement.trim_start().trim_end_matches(|c| c == '\n' || c == '\r');
        self.defs.push((name.to_string(), repl.to_string()));
    }

    pub fn get(&self, name: &str) -> Option<&str> {
        // Search from end (most recent definition wins)
        for (n, v) in self.defs.iter().rev() {
            if n == name {
                return Some(v.as_str());
            }
        }
        None
    }

    pub fn exists(&self, name: &str) -> bool {
        self.defs.iter().any(|(n, _)| n == name)
    }

    pub fn save_snapshot(&mut self) -> usize {
        let len = self.defs.len();
        self.snapshot_len = len;
        len
    }

    pub fn restore_snapshot(&mut self, len: usize) {
        self.defs.truncate(len);
    }
}

// ============================================================
// Macro table
// ============================================================

#[derive(Debug, Clone)]
pub struct MacroEntry {
    pub name: String,
    pub args: Vec<String>,
    pub body: Vec<String>,
}

pub struct MacroTab {
    macros: HashMap<String, MacroEntry>,
}

impl MacroTab {
    pub fn new() -> Self {
        MacroTab {
            macros: HashMap::new(),
        }
    }

    pub fn add(&mut self, name: &str, args: Vec<String>, body: Vec<String>) -> bool {
        if self.macros.contains_key(name) {
            return false; // duplicate
        }
        self.macros.insert(name.to_string(), MacroEntry {
            name: name.to_string(),
            args,
            body,
        });
        true
    }

    pub fn get(&self, name: &str) -> Option<&MacroEntry> {
        self.macros.get(name)
    }

    pub fn exists(&self, name: &str) -> bool {
        self.macros.contains_key(name)
    }
}

// ============================================================
// Struct table
// ============================================================

#[derive(Debug, Clone)]
pub struct StructMembN {
    pub name: String,
    pub offset: i64,
}

#[derive(Debug, Clone)]
pub struct StructMembI {
    pub offset: i64,
    pub len: i64,
    pub def: i64,
    pub kind: StructMemb,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub id: String,
    pub binding: i32,
    pub global: bool,
    pub noffset: i64,
    pub member_names: Vec<StructMembN>,
    pub member_items: Vec<StructMembI>,
}

impl StructDef {
    pub fn new(name: &str, id: &str, binding: i32, noffset: i64, global: bool) -> Self {
        StructDef {
            name: name.to_string(),
            id: id.to_string(),
            binding,
            global,
            noffset,
            member_names: Vec::new(),
            member_items: Vec::new(),
        }
    }

    pub fn add_label(&mut self, name: &str) {
        self.member_names.push(StructMembN {
            name: name.to_string(),
            offset: self.noffset,
        });
    }

    pub fn add_memb(&mut self, item: StructMembI) {
        self.noffset += item.len;
        self.member_items.push(item);
    }

    pub fn copy_label(&mut self, name: &str, offset: i64) {
        self.member_names.push(StructMembN {
            name: name.to_string(),
            offset: self.noffset + offset,
        });
    }

    pub fn copy_labels_from(&mut self, other: &StructDef, prevlab: &str) {
        if other.member_names.is_empty() || prevlab.is_empty() {
            return;
        }
        let prefix = format!("{}.", prevlab);
        for mn in &other.member_names {
            let full_name = format!("{}{}", prefix, mn.name);
            self.copy_label(&full_name, mn.offset);
        }
    }

    pub fn copy_memb(&mut self, item: &StructMembI, def: i64) {
        let new_item = StructMembI {
            offset: self.noffset,
            len: item.len,
            def,
            kind: item.kind,
        };
        self.noffset += new_item.len;
        self.member_items.push(new_item);
    }
}

pub struct StructTab {
    structs: HashMap<String, Vec<StructDef>>,
}

impl StructTab {
    pub fn new() -> Self {
        StructTab {
            structs: HashMap::new(),
        }
    }

    pub fn add(&mut self, name: &str, noffset: i64, _binding: i32, global: bool, modlabp: &Option<String>) -> usize {
        let mut id = String::new();
        if !global {
            if let Some(ref mlp) = modlabp {
                id.push_str(mlp);
                id.push('.');
            }
        }
        id.push_str(name);

        let entry = StructDef::new(name, &id, _binding, 0, global);
        let list = self.structs.entry(id.clone()).or_insert_with(Vec::new);

        // Add initial block member if noffset > 0
        let idx = list.len();
        let mut sd = entry;
        if noffset > 0 {
            sd.add_memb(StructMembI {
                offset: 0,
                len: noffset,
                def: 0,
                kind: StructMemb::Block,
            });
        }
        list.push(sd);

        // Return a handle (we'll store the key + index)
        idx
    }

    pub fn lookup(&self, name: &str, global: bool, modlabp: &Option<String>) -> Option<&StructDef> {
        let mut sn = String::new();
        if !global {
            if let Some(ref mlp) = modlabp {
                sn.push_str(mlp);
                sn.push('.');
            }
        }
        sn.push_str(name);

        if let Some(list) = self.structs.get(&sn) {
            return list.last();
        }
        // Try without module prefix
        if !global {
            if let Some(list) = self.structs.get(name) {
                return list.last();
            }
        }
        None
    }

    pub fn lookup_mut(&mut self, name: &str, global: bool, modlabp: &Option<String>) -> Option<&mut StructDef> {
        let mut sn = String::new();
        if !global {
            if let Some(ref mlp) = modlabp {
                sn.push_str(mlp);
                sn.push('.');
            }
        }
        sn.push_str(name);

        if self.structs.contains_key(&sn) {
            return self.structs.get_mut(&sn).and_then(|list| list.last_mut());
        }
        if !global {
            if self.structs.contains_key(name) {
                return self.structs.get_mut(name).and_then(|list| list.last_mut());
            }
        }
        None
    }

    pub fn exists(&self, name: &str) -> bool {
        self.structs.contains_key(name)
    }
}

// ============================================================
// Label name construction
// ============================================================

/// Check if char is valid in a label/identifier
pub fn is_valid_id_char(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '_' || c == '.' || c == '?' || c == '!' || c == '#' || c == '@'
}

/// Build a fully qualified label name from the raw name
pub fn make_label_name(
    naam: &str,
    modlabp: &Option<String>,
    vorlabp: &str,
    macrolabp: &Option<String>,
) -> Result<(String, String), String> {
    let mut np = naam;
    let mut is_global = false;
    let mut is_local = false;
    let mut use_mlp = macrolabp.is_some();

    if use_mlp && np.starts_with('@') {
        np = &np[1..];
        use_mlp = false;
    }

    if np.starts_with('@') {
        is_global = true;
        np = &np[1..];
    } else if np.starts_with('.') {
        is_local = true;
        np = &np[1..];
    }

    let raw_name = np;

    // Validate
    let first_char = raw_name.chars().next().ok_or_else(|| "Invalid labelname".to_string())?;
    if !first_char.is_ascii_alphabetic() && first_char != '_' {
        return Err(format!("Invalid labelname: {}", raw_name));
    }
    for c in raw_name.chars() {
        if !is_valid_id_char(c) {
            return Err(format!("Invalid labelname: {}", raw_name));
        }
    }

    if raw_name.len() > LABMAX {
        return Err(format!("Label too long: {}", raw_name));
    }

    let mut label = String::new();
    let new_vorlabp;

    if use_mlp && is_local {
        if let Some(ref mlp) = macrolabp {
            label.push_str(mlp);
            label.push('>');
        }
        new_vorlabp = vorlabp.to_string();
    } else {
        if !is_global {
            if let Some(ref mlp) = modlabp {
                label.push_str(mlp);
                label.push('.');
            }
        }
        if is_local {
            label.push_str(vorlabp);
            label.push('.');
            new_vorlabp = vorlabp.to_string();
        } else {
            new_vorlabp = raw_name.to_string();
        }
    }
    label.push_str(raw_name);

    Ok((label, new_vorlabp))
}
