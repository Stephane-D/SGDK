//! Sound utilities for WAV file loading and conversion.

use anyhow::Result;

/// Read WAV file and convert to raw PCM data (8-bit signed mono).
pub fn get_raw_data_from_wav(
    path: &str,
    bits: u32,
    out_rate: u32,
    signed: bool,
    mono: bool,
    _reversed: bool,
) -> Result<Vec<u8>> {
    let mut reader = hound::WavReader::open(path)?;
    let spec = reader.spec();

    // Read all samples as i32
    let samples: Vec<i32> = match spec.sample_format {
        hound::SampleFormat::Int => {
            reader.samples::<i32>().map(|s| s.unwrap_or(0)).collect()
        }
        hound::SampleFormat::Float => {
            reader.samples::<f32>()
                .map(|s| (s.unwrap_or(0.0) * 32767.0) as i32)
                .collect()
        }
    };

    let channels = spec.channels as usize;
    let in_rate = spec.sample_rate;
    let in_bits = spec.bits_per_sample;

    // Convert to mono if needed
    let mono_samples: Vec<i32> = if mono && channels > 1 {
        samples.chunks(channels)
            .map(|chunk| {
                let sum: i64 = chunk.iter().map(|&s| s as i64).sum();
                (sum / channels as i64) as i32
            })
            .collect()
    } else {
        samples.chunks(channels)
            .map(|chunk| chunk[0])
            .collect()
    };

    // Normalize to 16-bit range
    let scale_shift = if in_bits > 16 { in_bits - 16 } else { 0 };
    let scale_up = if in_bits < 16 { 16 - in_bits } else { 0 };
    let normalized: Vec<i16> = mono_samples.iter().map(|&s| {
        let v = if scale_shift > 0 {
            s >> scale_shift
        } else if scale_up > 0 {
            s << scale_up
        } else {
            s
        };
        v.clamp(-32768, 32767) as i16
    }).collect();

    // Resample if needed
    let resampled = if in_rate != out_rate && out_rate > 0 {
        resample(&normalized, in_rate, out_rate)
    } else {
        normalized
    };

    // Convert to 8-bit
    let result: Vec<u8> = if bits == 8 {
        if signed {
            resampled.iter().map(|&s| (s >> 8) as i8 as u8).collect()
        } else {
            resampled.iter().map(|&s| ((s >> 8) as i8 as i32 + 128) as u8).collect()
        }
    } else {
        // 16-bit output
        let mut out = Vec::with_capacity(resampled.len() * 2);
        for &s in &resampled {
            let v = if signed { s as u16 } else { (s as i32 + 32768) as u16 };
            out.push((v >> 8) as u8);
            out.push(v as u8);
        }
        out
    };

    Ok(result)
}

/// Simple linear resampling.
fn resample(data: &[i16], in_rate: u32, out_rate: u32) -> Vec<i16> {
    if data.is_empty() || in_rate == out_rate {
        return data.to_vec();
    }

    let ratio = in_rate as f64 / out_rate as f64;
    let out_len = (data.len() as f64 / ratio) as usize;
    let mut result = Vec::with_capacity(out_len);

    for i in 0..out_len {
        let pos = i as f64 * ratio;
        let idx = pos as usize;
        let frac = pos - idx as f64;

        if idx + 1 < data.len() {
            let sample = data[idx] as f64 * (1.0 - frac) + data[idx + 1] as f64 * frac;
            result.push(sample.round() as i16);
        } else if idx < data.len() {
            result.push(data[idx]);
        }
    }

    result
}
