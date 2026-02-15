//! String utility functions.

/// Convert a string to a valid C identifier (replace non-alphanumeric with '_').
pub fn to_c_identifier(s: &str) -> String {
    let mut result = String::with_capacity(s.len());
    for (i, c) in s.chars().enumerate() {
        if c.is_ascii_alphanumeric() || c == '_' {
            if i == 0 && c.is_ascii_digit() {
                result.push('_');
            }
            result.push(c);
        } else {
            result.push('_');
        }
    }
    result
}

/// Convert an integer to a hexadecimal string with a given minimum width.
pub fn int_to_hex(value: u32, min_width: usize) -> String {
    format!("{:0>width$X}", value, width = min_width)
}

/// Parse an integer from various formats: decimal, hex (0x prefix), binary (0b prefix).
pub fn parse_int(s: &str) -> Option<i64> {
    let s = s.trim();
    if let Some(hex) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
        i64::from_str_radix(hex, 16).ok()
    } else if let Some(bin) = s.strip_prefix("0b").or_else(|| s.strip_prefix("0B")) {
        i64::from_str_radix(bin, 2).ok()
    } else {
        s.parse::<i64>().ok()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_to_c_identifier() {
        assert_eq!(to_c_identifier("hello-world"), "hello_world");
        assert_eq!(to_c_identifier("my file.txt"), "my_file_txt");
        assert_eq!(to_c_identifier("123abc"), "_123abc");
        assert_eq!(to_c_identifier("valid_name"), "valid_name");
    }

    #[test]
    fn test_int_to_hex() {
        assert_eq!(int_to_hex(0xFF, 4), "00FF");
        assert_eq!(int_to_hex(0xDEAD, 2), "DEAD");
        assert_eq!(int_to_hex(0, 2), "00");
    }

    #[test]
    fn test_parse_int() {
        assert_eq!(parse_int("42"), Some(42));
        assert_eq!(parse_int("0xFF"), Some(255));
        assert_eq!(parse_int("0b1010"), Some(10));
        assert_eq!(parse_int("-1"), Some(-1));
        assert_eq!(parse_int("abc"), None);
    }
}
