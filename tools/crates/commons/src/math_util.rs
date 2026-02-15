//! Math utility functions.

/// Get the number of the highest set bit (0-based).
/// Returns 0 for value <= 0.
pub fn high_bit_num(value: i32) -> i32 {
    if value <= 0 {
        return 0;
    }
    let mut result = 0;
    let mut v = value;
    if v >= 0x10000 {
        result += 16;
        v >>= 16;
    }
    if v >= 0x100 {
        result += 8;
        v >>= 8;
    }
    if v >= 0x10 {
        result += 4;
        v >>= 4;
    }
    if v >= 4 {
        result += 2;
        v >>= 2;
    }
    if v >= 2 {
        result += 1;
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_high_bit_num() {
        assert_eq!(high_bit_num(1), 0);
        assert_eq!(high_bit_num(2), 1);
        assert_eq!(high_bit_num(3), 1);
        assert_eq!(high_bit_num(4), 2);
        assert_eq!(high_bit_num(255), 7);
        assert_eq!(high_bit_num(256), 8);
    }
}
