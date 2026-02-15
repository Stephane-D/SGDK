/// Parser for `/key=value /flag+ /flag-` option strings.
/// Used for -inopt and -outopt CLI parameters.

use std::collections::HashMap;

/// Value target for an option record.
pub enum RecordTarget<'a> {
    Bool(&'a mut bool),
    Char(&'a mut char),
    Str(&'a mut String),
}

/// Parse an options string of the form `/key=value /flag+ /flag-`.
///
/// Supported types:
/// - Bool: `/flag+` sets true, `/flag-` sets false
/// - Char: `/key=x` sets a single character
/// - String: `/key='value'` or `/key=value` sets a string
pub fn parse(opts: &str, records: &mut HashMap<&str, RecordTarget>) {
    if opts.is_empty() {
        return;
    }

    let mut chars = opts.chars().peekable();

    loop {
        // Skip whitespace
        while chars.peek().is_some_and(|c| c.is_whitespace()) {
            chars.next();
        }

        // Expect '/' at start of each option
        match chars.peek() {
            Some('/') => {
                chars.next();
            }
            None => break,
            Some(_) => {
                // Skip non-option characters
                chars.next();
                continue;
            }
        }

        // Read key name
        let mut key = String::new();
        while let Some(&c) = chars.peek() {
            if c == '=' || c == '+' || c == '-' || c.is_whitespace() {
                break;
            }
            key.push(c);
            chars.next();
        }

        if key.is_empty() {
            continue;
        }

        // Read value
        match chars.peek() {
            Some('=') => {
                chars.next();
                // Read value - may be quoted
                let value = if chars.peek() == Some(&'\'') {
                    chars.next(); // skip opening quote
                    let mut v = String::new();
                    while let Some(&c) = chars.peek() {
                        if c == '\'' {
                            chars.next();
                            break;
                        }
                        v.push(c);
                        chars.next();
                    }
                    v
                } else {
                    let mut v = String::new();
                    while let Some(&c) = chars.peek() {
                        if c.is_whitespace() || c == '/' {
                            break;
                        }
                        v.push(c);
                        chars.next();
                    }
                    v
                };

                if let Some(target) = records.get_mut(key.as_str()) {
                    match target {
                        RecordTarget::Char(c) => {
                            if let Some(ch) = value.chars().next() {
                                **c = ch;
                            }
                        }
                        RecordTarget::Str(s) => {
                            **s = value;
                        }
                        RecordTarget::Bool(b) => {
                            **b = !value.is_empty() && value != "0" && value != "false";
                        }
                    }
                }
            }
            Some('+') => {
                chars.next();
                if let Some(RecordTarget::Bool(b)) = records.get_mut(key.as_str()) {
                    **b = true;
                }
            }
            Some('-') => {
                chars.next();
                if let Some(RecordTarget::Bool(b)) = records.get_mut(key.as_str()) {
                    **b = false;
                }
            }
            _ => {
                // Flag without + or - is treated as true
                if let Some(RecordTarget::Bool(b)) = records.get_mut(key.as_str()) {
                    **b = true;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_bool_flags() {
        let mut flag_on = false;
        let mut flag_off = true;
        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("flagOn", RecordTarget::Bool(&mut flag_on));
            records.insert("flagOff", RecordTarget::Bool(&mut flag_off));
            parse("/flagOn+ /flagOff-", &mut records);
        }
        assert!(flag_on);
        assert!(!flag_off);
    }

    #[test]
    fn test_parse_char() {
        let mut sep = ':';
        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("sep", RecordTarget::Char(&mut sep));
            parse("/sep=|", &mut records);
        }
        assert_eq!(sep, '|');
    }

    #[test]
    fn test_parse_string() {
        let mut fmt = String::from("default");
        {
            let mut records: HashMap<&str, RecordTarget> = HashMap::new();
            records.insert("fmt", RecordTarget::Str(&mut fmt));
            parse("/fmt='%s %X'", &mut records);
        }
        assert_eq!(fmt, "%s %X");
    }
}
