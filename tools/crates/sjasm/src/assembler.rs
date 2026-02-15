use crate::tables::{LabTab, LokLabTab, DefineTab, MacDefineTab, MacroTab, StructTab};
use std::collections::HashMap;
use std::io::Write;

/// Max line length
pub const LINEMAX: usize = 300;
/// Max label length
pub const LABMAX: usize = 70;

/// Error severity kinds
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ErrorKind {
    All = 0,
    Pass1 = 1,
    Pass2 = 2,
    Fatal = 3,
    CatchAll = 4,
    Suppres = 5,
}

/// Ending types returned by ReadFile/SkipFile
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Ending {
    End,
    EndIf,
    Else,
    EndTextArea,
}

/// Output file open mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OutputMode {
    Truncate,
    Rewind,
    Append,
}

/// Struct member kinds
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StructMemb {
    Unknown,
    Block,
    Byte,
    Word,
    D24,
    DWord,
    Align,
    ParenOpen,
    ParenClose,
}

/// Central assembler state, replaces all the C++ globals
pub struct Assembler {
    // Source tracking
    pub filename: String,
    pub line: String,
    pub lp: usize, // current position in line (index)
    pub bp: usize,  // beginning position saved for error reporting
    pub pass: i32,
    pub adres: i64,        // current address
    pub mapadr: i64,       // map address for .field
    pub gcurlin: i64,      // global current line
    pub lcurlin: i64,      // local current line (per file)
    pub curlin: i64,       // total lines
    pub maxlin: i64,
    pub include: i32,      // include nesting depth
    pub running: bool,
    pub nerror: i32,

    // Listing state
    pub listfile: bool,
    pub donotlist: bool,
    pub listdata: bool,
    pub listmacro: bool,
    pub reglenwidth: i32,

    // Filenames
    pub sourcefilename: String,
    pub destfilename: String,
    pub listfilename: String,
    pub expfilename: String,
    pub symfilename: String,

    // Options
    pub quiet: bool,
    pub symfile: bool,
    pub labellisting: bool,
    pub use_vs_error_format: bool,
    pub compass_compat: bool,
    pub err_to_stderr: bool,
    pub include_dirs: Vec<String>,

    // Emitted bytes buffer for listing
    pub eb: Vec<i32>,

    // Output buffer
    pub dest_buf: Vec<u8>,
    pub dest_len: i64,
    pub output: Option<std::fs::File>,
    pub list_fp: Option<std::fs::File>,
    pub exp_fp: Option<std::fs::File>,

    // Size constraint
    pub size: i64,

    // Labels, defines, macros, structs
    pub labtab: LabTab,
    pub loklabtab: LokLabTab,
    pub definetab: DefineTab,
    pub macdeftab: MacDefineTab,
    pub macrotab: MacroTab,
    pub structtab: StructTab,

    // Module / macro label state
    pub modlabp: Option<String>,
    pub modlstp: Vec<String>,  // module stack
    pub vorlabp: String,       // previous label for local labels
    pub macrolabp: Option<String>,
    pub macronummer: i32,

    // Map stack
    pub maplstp: Vec<i64>,

    // IF state
    pub comlin: i32,  // block comment nesting

    // List for macro/rept expansion
    pub lijst: bool,
    pub lijstp: Vec<String>,
    pub lijst_idx: usize,

    // Skip errors  
    pub skiperrors: bool,
    pub preverror: i64,

    // Label not found flag (used during expression evaluation)
    pub labelnotfound: bool,
    pub synerr: bool,

    // Listing address tracking
    pub eadres: i64,
    pub epadres: i64,

    // Temp buffer for listing
    pub pline: String,
    pub eline: String,
    pub temp: String,

    // Error output
    pub error_output: Vec<String>,

    // Directive function table
    pub dir_tab: HashMap<String, fn(&mut Assembler)>,
    // Z80 instruction function table
    pub z80_tab: HashMap<String, fn(&mut Assembler)>,

    // Compass-style macro definition flag
    pub inside_compass_macro_def: bool,

    // Previous label for struct parsing
    pub prevlab: Option<String>,

    // Replacement counter (for recursive define replacement)
    pub replace_define_counter: i32,
    pub comnxtlin: i32,

    // File input stack for stream-based reading
    pub input_lines: Vec<String>,
    pub input_idx: usize,
}

impl Assembler {
    pub fn new() -> Self {
        Assembler {
            filename: String::new(),
            line: String::new(),
            lp: 0,
            bp: 0,
            pass: 0,
            adres: 0,
            mapadr: 0,
            gcurlin: 0,
            lcurlin: 0,
            curlin: 0,
            maxlin: 0,
            include: 0,
            running: true,
            nerror: 0,
            listfile: false,
            donotlist: false,
            listdata: false,
            listmacro: false,
            reglenwidth: 5,
            sourcefilename: String::new(),
            destfilename: String::new(),
            listfilename: String::new(),
            expfilename: String::new(),
            symfilename: String::new(),
            quiet: false,
            symfile: false,
            labellisting: false,
            use_vs_error_format: false,
            compass_compat: false,
            err_to_stderr: false,
            include_dirs: Vec::new(),
            eb: Vec::new(),
            dest_buf: Vec::new(),
            dest_len: 0,
            output: None,
            list_fp: None,
            exp_fp: None,
            size: -1,
            labtab: LabTab::new(),
            loklabtab: LokLabTab::new(),
            definetab: DefineTab::new(),
            macdeftab: MacDefineTab::new(),
            macrotab: MacroTab::new(),
            structtab: StructTab::new(),
            modlabp: None,
            modlstp: Vec::new(),
            vorlabp: String::new(),
            macrolabp: None,
            macronummer: 0,
            maplstp: Vec::new(),
            comlin: 0,
            lijst: false,
            lijstp: Vec::new(),
            lijst_idx: 0,
            skiperrors: false,
            preverror: -1,
            labelnotfound: false,
            synerr: true,
            eadres: -1,
            epadres: 0,
            pline: String::new(),
            eline: String::new(),
            temp: String::new(),
            error_output: Vec::new(),
            dir_tab: HashMap::new(),
            z80_tab: HashMap::new(),
            inside_compass_macro_def: false,
            prevlab: None,
            replace_define_counter: 0,
            comnxtlin: 0,
            input_lines: Vec::new(),
            input_idx: 0,
        }
    }

    /// Initialize for a new pass
    pub fn init_pass(&mut self, p: i32) {
        self.pass = p;
        self.adres = 0;
        self.mapadr = 0;
        self.eadres = -1;
        self.epadres = 0;
        self.gcurlin = 0;
        self.lcurlin = 0;
        self.curlin = 0;
        self.include = 0;
        self.running = true;
        self.macronummer = 0;
        self.lijst = false;
        self.lijstp.clear();
        self.lijst_idx = 0;
        self.comlin = 0;
        self.skiperrors = false;
        self.preverror = -1;
        self.modlabp = None;
        self.modlstp.clear();
        self.vorlabp = String::new();
        self.macrolabp = None;
        self.donotlist = false;
        self.listmacro = false;
        self.size = -1;
        self.dest_len = 0;
    }

    /// Report an error
    pub fn error(&mut self, msg: &str, detail: Option<&str>, kind: ErrorKind) {
        if self.skiperrors && self.preverror == self.lcurlin && kind != ErrorKind::Fatal {
            return;
        }
        if kind == ErrorKind::CatchAll && self.preverror == self.lcurlin {
            return;
        }
        if kind == ErrorKind::Pass1 && self.pass != 1 {
            return;
        }
        if (kind == ErrorKind::CatchAll || kind == ErrorKind::Suppres || kind == ErrorKind::Pass2) && self.pass != 2 {
            return;
        }
        self.skiperrors = kind == ErrorKind::Suppres;
        self.preverror = self.lcurlin;
        self.nerror += 1;

        let error_sort = match kind {
            ErrorKind::All => "ALL",
            ErrorKind::Pass1 => "PASS1",
            ErrorKind::Pass2 => "PASS2",
            ErrorKind::Fatal => "FATAL",
            ErrorKind::CatchAll => "CATCHALL",
            ErrorKind::Suppres => "SUPPRES",
        };

        let mut ep = if self.use_vs_error_format {
            format!("{}({}) : error {} : {}", self.filename, self.lcurlin, error_sort, msg)
        } else {
            format!("{} line {}: {}", self.filename, self.lcurlin, msg)
        };

        if let Some(bd) = detail {
            ep.push_str(": ");
            ep.push_str(bd);
        }
        if !ep.ends_with('\n') {
            ep.push('\n');
        }

        // Write to list file  
        if self.listfile {
            if let Some(ref mut fp) = self.list_fp {
                let _ = fp.write_all(ep.as_bytes());
            }
        }

        // Write to stderr or store
        eprint!("{}", ep);
        self.error_output.push(ep);

        if kind == ErrorKind::Fatal {
            std::process::exit(1);
        }
    }

    /// Shorthand for error with no detail, default severity
    pub fn error_simple(&mut self, msg: &str) {
        self.error(msg, None, ErrorKind::Pass2);
    }

    /// Get current char at lp position
    pub fn cur_char(&self) -> Option<char> {
        self.line.as_bytes().get(self.lp).map(|&b| b as char)
    }

    /// Peek at char at offset from lp
    pub fn peek_char(&self, offset: usize) -> Option<char> {
        self.line.as_bytes().get(self.lp + offset).map(|&b| b as char)
    }

    /// Advance lp by n
    pub fn advance(&mut self, n: usize) {
        self.lp = (self.lp + n).min(self.line.len());
    }

    /// Get remaining line from lp
    pub fn remaining(&self) -> &str {
        if self.lp >= self.line.len() {
            ""
        } else {
            &self.line[self.lp..]
        }
    }
}
