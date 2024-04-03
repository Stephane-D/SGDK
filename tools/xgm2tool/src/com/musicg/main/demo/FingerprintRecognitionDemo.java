/*
 * Copyright (C) 2012 Jacquet Wong
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.musicg.main.demo;

import com.musicg.fingerprint.FingerprintSimilarity;
import com.musicg.wave.Wave;

public class FingerprintRecognitionDemo {

	public static void main(String[] args) {

		String songA = "audio_work/songs/canon_d_major.wav";
		String songB = "audio_work/songs/fing_fing_ha.wav";
		String songC = "audio_work/songs/forrest_gump_theme.wav";
		String songD = "audio_work/songs/imagine.wav";
		String songE = "audio_work/songs/top_of_the_world.wav";

		// create a wave object
		Wave waveA = new Wave(songA);
		Wave waveB = new Wave(songB);
		Wave waveC = new Wave(songC);
		Wave waveD = new Wave(songD);
		Wave waveE = new Wave(songE);

		String recordedClip = "audio_work/songs/top_of_the_world_rec.wav";
		Wave waveRec = new Wave(recordedClip);

		FingerprintSimilarity similarity;
		
		// song A:
		similarity = waveA.getFingerprintSimilarity(waveRec);
		System.out.println("clip is found at "
				+ similarity.getsetMostSimilarTimePosition() + "s in "
				+ songA+" with similarity " + similarity.getSimilarity());
		
		// song B:
		similarity = waveB.getFingerprintSimilarity(waveRec);
		System.out.println("clip is found at "
				+ similarity.getsetMostSimilarTimePosition() + "s in "
				+ songB+" with similarity " + similarity.getSimilarity());
		
		// song C:
		similarity = waveC.getFingerprintSimilarity(waveRec);
		System.out.println("clip is found at "
				+ similarity.getsetMostSimilarTimePosition() + "s in "
				+ songC+" with similarity " + similarity.getSimilarity());
		
		// song D:
		similarity = waveD.getFingerprintSimilarity(waveRec);
		System.out.println("clip is found at "
				+ similarity.getsetMostSimilarTimePosition() + "s in "
				+ songD+" with similarity " + similarity.getSimilarity());
		
		// song E:
		similarity = waveE.getFingerprintSimilarity(waveRec);
		System.out.println("clip is found at "
				+ similarity.getsetMostSimilarTimePosition() + "s in "
				+ songE+" with similarity " + similarity.getSimilarity());
	}
}