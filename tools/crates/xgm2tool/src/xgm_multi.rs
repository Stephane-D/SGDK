use crate::util;
use crate::xgm::XGM;
use crate::xgm_sample::XGMSample;
use anyhow::Result;

/// Multi-track XGM2 file (up to 128 tracks, 248 shared samples)
pub struct XGMMulti {
    pub xgms: Vec<XGM>,
    pub shared_samples: Vec<XGMSample>,
    pub pal: bool,
    pub has_gd3: bool,
    pub packed: bool,
    merged_sample: usize,
}

impl XGMMulti {
    pub fn new(mut xgms: Vec<XGM>, pack: bool) -> Self {
        if !crate::is_silent() {
            println!("Converting {} XGM to multi tracks XGM...", xgms.len());
        }

        if xgms.len() > 128 {
            eprintln!(
                "Warning: multi tracks XGM is limited to 128 tracks max ({} tracks will be ignored)!",
                xgms.len() - 128
            );
            xgms.truncate(128);
        }

        let mut has_gd3 = false;
        let mut pal_val: Option<bool> = None;

        for xgm in &xgms {
            match pal_val {
                None => pal_val = Some(xgm.pal),
                Some(p) if p != xgm.pal => {
                    eprintln!("Warning: multi tracks XGM cannot mix PAL and NTSC tracks");
                    pal_val = Some(false);
                }
                _ => {}
            }
            if xgm.gd3.is_some() || xgm.xd3.is_some() {
                has_gd3 = true;
            }
        }

        let pal = pal_val.unwrap_or(false);

        let mut multi = XGMMulti {
            xgms,
            shared_samples: Vec::new(),
            pal,
            has_gd3,
            packed: pack,
            merged_sample: 0,
        };

        // Build shared samples
        let num_xgms = multi.xgms.len();
        for xi in 0..num_xgms {
            let sample_ids: Vec<i32> = multi.xgms[xi].samples.iter().map(|s| s.id).collect();
            for sid in sample_ids {
                // Find the sample in xgms[xi] and clone it
                if let Some(sample) = multi.xgms[xi]
                    .samples
                    .iter()
                    .find(|s| s.id == sid)
                    .cloned()
                {
                    multi.merge_sample(sample, xi);
                }
            }
            multi.xgms[xi].rebuild_fm_commands();
        }

        if !crate::is_silent() {
            if let Ok(fm_size) = multi.get_total_fm_music_data_size() {
                println!("FM data size: {}", fm_size);
            }
            if let Ok(psg_size) = multi.get_total_psg_music_data_size() {
                println!("PSG data size: {}", psg_size);
            }
            println!("PCM data size: {}", multi.get_pcm_data_size());
            println!(
                "Number of sample = {} ({} merged samples)",
                multi.shared_samples.len(),
                multi.merged_sample
            );
        }

        multi
    }

    fn find_matching_sample(&self, sample: &XGMSample) -> Option<usize> {
        let mut best_idx: Option<usize> = None;
        let mut best_score: f64 = 0.0;

        for (i, s) in self.shared_samples.iter().enumerate() {
            let score = s.get_similarity_score(sample);
            if score > best_score {
                best_idx = Some(i);
                best_score = score;
            }
        }

        if best_score >= 1.0 {
            best_idx
        } else {
            None
        }
    }

    fn merge_sample(&mut self, sample: XGMSample, xgm_index: usize) {
        if let Some(match_idx) = self.find_matching_sample(&sample) {
            let match_id = self.shared_samples[match_idx].id;
            let match_len = self.shared_samples[match_idx].get_length();
            let sample_len = sample.get_length();
            let same_duration =
                (sample_len as f64 / 60.0).round() as i64 == (match_len as f64 / 60.0).round() as i64;
            let duration = if same_duration {
                -1i64
            } else {
                (sample_len as i64 * 44100) / crate::xgm_sample::XGM_FULL_RATE as i64
            };

            self.xgms[xgm_index].update_sample_commands(sample.id, match_id, duration);
            self.merged_sample += 1;

            if crate::is_verbose() {
                println!("Found duplicated sample #{} (merged)", sample.id);
            }
        } else {
            if self.shared_samples.len() >= 248 {
                eprintln!("Warning: multi tracks XGM is limited to 248 samples max, some samples will be lost !");
                return;
            }

            let new_id = (self.shared_samples.len() + 1) as i32;
            self.xgms[xgm_index].update_sample_commands(sample.id, new_id, -1);

            let mut s = sample;
            s.id = new_id;
            self.shared_samples.push(s);
        }
    }

    fn get_total_psg_music_data_size(&self) -> Result<usize> {
        let mut total = 0;
        for xgm in &self.xgms {
            let size = if self.packed {
                xgm.get_psg_music_data_array().len() // approximate for packed
            } else {
                xgm.get_psg_music_data_size()
            };
            total += util::align_size(size, 256);
        }
        Ok(total)
    }

    fn get_total_fm_music_data_size(&self) -> Result<usize> {
        let mut total = 0;
        for xgm in &self.xgms {
            let size = if self.packed {
                xgm.get_fm_music_data_array().len() // approximate for packed
            } else {
                xgm.get_fm_music_data_size()
            };
            total += util::align_size(size, 256);
        }
        Ok(total)
    }

    fn get_pcm_data_size(&self) -> usize {
        self.shared_samples.iter().map(|s| s.data.len()).sum()
    }

    fn get_pcm_data_array(&self) -> Vec<u8> {
        let mut result = Vec::new();
        for sample in &self.shared_samples {
            let mut signed = sample.data.clone();
            for b in &mut signed {
                *b = b.wrapping_add(0x80);
            }
            result.extend(signed);
        }
        result
    }

    pub fn as_byte_array(&mut self) -> Result<Vec<u8>> {
        let mut result = Vec::new();

        // XGM2 id
        if !self.packed {
            result.extend_from_slice(b"XGM2");
        }
        // version
        result.push(0x10);

        // flags
        let mut flags = 0u8;
        if self.pal {
            flags |= 1;
        }
        flags |= 2; // multi tracks
        if self.has_gd3 {
            flags |= 4;
        }
        if self.packed {
            flags |= 8;
        }
        result.push(flags);

        // SLEN
        let pcm_size = util::align_size(self.get_pcm_data_size(), 256);
        let slen = (pcm_size >> 8) as u16;
        result.push((slen & 0xFF) as u8);
        result.push(((slen >> 8) & 0xFF) as u8);

        // FMLEN
        let fm_total = self.get_total_fm_music_data_size()?;
        let fmlen = (fm_total >> 8) as u16;
        result.push((fmlen & 0xFF) as u8);
        result.push(((fmlen >> 8) & 0xFF) as u8);

        // PSGLEN
        let psg_total = self.get_total_psg_music_data_size()?;
        let psglen = (psg_total >> 8) as u16;
        result.push((psglen & 0xFF) as u8);
        result.push(((psglen >> 8) & 0xFF) as u8);

        // SID table (504 bytes = 252 entries)
        let mut offset = 0usize;
        for sample in &self.shared_samples {
            let len = sample.data.len();
            result.push((offset >> 8) as u8);
            result.push((offset >> 16) as u8);
            offset += len;
        }
        result.push((offset >> 8) as u8);
        result.push((offset >> 16) as u8);
        let entries_written = self.shared_samples.len() + 1;
        for _ in entries_written..(504 / 2) {
            result.push(0xFF);
            result.push(0xFF);
        }

        // FMID table (256 bytes = 128 entries)
        offset = 0;
        for xgm in &self.xgms {
            result.push((offset & 0xFF) as u8);
            result.push(((offset >> 8) & 0xFF) as u8);

            let size = if self.packed {
                xgm.get_fm_music_data_array().len()
            } else {
                xgm.get_fm_music_data_size()
            };
            offset += util::align_size(size, 256) >> 8;
        }
        for _ in self.xgms.len()..128 {
            result.push(0xFF);
            result.push(0xFF);
        }

        // PSGID table (256 bytes = 128 entries)
        offset = 0;
        for xgm in &self.xgms {
            result.push((offset & 0xFF) as u8);
            result.push(((offset >> 8) & 0xFF) as u8);

            let size = if self.packed {
                xgm.get_psg_music_data_array().len()
            } else {
                xgm.get_psg_music_data_size()
            };
            offset += util::align_size(size, 256) >> 8;
        }
        for _ in self.xgms.len()..128 {
            result.push(0xFF);
            result.push(0xFF);
        }

        // PCM data
        result.extend(self.get_pcm_data_array());

        // FM + PSG music data
        if self.packed {
            for xgm in &mut self.xgms {
                let data = xgm.get_packed_fm_music_data_array()?;
                result.extend(util::align_data(&data, 256));
            }
            for xgm in &mut self.xgms {
                let data = xgm.get_packed_psg_music_data_array()?;
                result.extend(util::align_data(&data, 256));
            }
        } else {
            for xgm in &self.xgms {
                let data = xgm.get_fm_music_data_array();
                result.extend(util::align_data(&data, 256));
            }
            for xgm in &self.xgms {
                let data = xgm.get_psg_music_data_array();
                result.extend(util::align_data(&data, 256));
            }
        }

        // GD3/XD3 tags
        if self.has_gd3 {
            // GID table (256 bytes = 128 entries)
            offset = 0;
            for xgm in &self.xgms {
                if self.packed {
                    if let Some(ref xd3) = xgm.xd3 {
                        result.push((offset & 0xFF) as u8);
                        result.push(((offset >> 8) & 0xFF) as u8);
                        offset += xd3.get_total_data_size();
                    } else {
                        result.push(0xFF);
                        result.push(0xFF);
                    }
                } else if let Some(ref gd3) = xgm.gd3 {
                    result.push((offset & 0xFF) as u8);
                    result.push(((offset >> 8) & 0xFF) as u8);
                    offset += gd3.get_total_data_size();
                } else {
                    result.push(0xFF);
                    result.push(0xFF);
                }
            }
            for _ in self.xgms.len()..128 {
                result.push(0xFF);
                result.push(0xFF);
            }

            // GD3/XD3 data
            for xgm in &self.xgms {
                if self.packed {
                    if let Some(ref xd3) = xgm.xd3 {
                        result.extend(xd3.as_byte_array());
                    }
                } else if let Some(ref gd3) = xgm.gd3 {
                    result.extend(gd3.as_byte_array());
                }
            }
        }

        Ok(result)
    }
}
