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

package com.musicg.fingerprint;

import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import com.musicg.math.quicksort.QuickSortIndexPreserved;
import com.musicg.properties.FingerprintProperties;

/**
 * Make pairs for the audio fingerprints, which a pair is used to group the same features together
 * 
 * @author jacquet
 *
 */
public class PairManager{

	FingerprintProperties fingerprintProperties=FingerprintProperties.getInstance();
	private int numFilterBanks=fingerprintProperties.getNumFilterBanks();
	private int bandwidthPerBank=fingerprintProperties.getNumFrequencyUnits()/numFilterBanks;
	private int anchorPointsIntervalLength=fingerprintProperties.getAnchorPointsIntervalLength();
	private int numAnchorPointsPerInterval=fingerprintProperties.getNumAnchorPointsPerInterval();
	private int maxTargetZoneDistance=fingerprintProperties.getMaxTargetZoneDistance();
	private int numFrequencyUnits=fingerprintProperties.getNumFrequencyUnits();
	
	private int maxPairs;
	private boolean isReferencePairing;
	private HashMap<Integer,Boolean> stopPairTable=new HashMap<Integer,Boolean>();
	
	/**
	 * Constructor
	 */
	public PairManager(){
		maxPairs=fingerprintProperties.getRefMaxActivePairs();
		isReferencePairing=true;
	}
	
	/**
	 * Constructor, number of pairs of robust points depends on the parameter isReferencePairing
	 * no. of pairs of reference and sample can be different due to environmental influence of source  
	 * @param isReferencePairing
	 */
	public PairManager(boolean isReferencePairing){
		if (isReferencePairing){
			maxPairs=fingerprintProperties.getRefMaxActivePairs();
		}
		else{
			maxPairs=fingerprintProperties.getSampleMaxActivePairs();
		}
		this.isReferencePairing=isReferencePairing;
	}
	
	/**
	 * Get a pair-positionList table
	 * It's a hash map which the key is the hashed pair, and the value is list of positions
	 * That means the table stores the positions which have the same hashed pair
	 * 
	 * @param fingerprint	fingerprint bytes
	 * @return pair-positionList HashMap
	 */
	public HashMap<Integer,List<Integer>> getPair_PositionList_Table(byte[] fingerprint){
		
		List<int[]> pairPositionList=getPairPositionList(fingerprint);
		
		// table to store pair:pos,pos,pos,...;pair2:pos,pos,pos,....
		HashMap<Integer,List<Integer>> pair_positionList_table=new HashMap<Integer,List<Integer>>();
		
		// get all pair_positions from list, use a table to collect the data group by pair hashcode
		Iterator<int[]> pairPositionListIterator=pairPositionList.iterator();
		while (pairPositionListIterator.hasNext()){
			int[] pair_position=pairPositionListIterator.next();
			//System.out.println(pair_position[0]+","+pair_position[1]);
			
			// group by pair-hashcode, i.e.: <pair,List<position>>
			if (pair_positionList_table.containsKey(pair_position[0])){
				pair_positionList_table.get(pair_position[0]).add(pair_position[1]);
			}
			else{
				List<Integer> positionList=new LinkedList<Integer>();
				positionList.add(pair_position[1]);
				pair_positionList_table.put(pair_position[0], positionList);
			}
			// end group by pair-hashcode, i.e.: <pair,List<position>>
		}
		// end get all pair_positions from list, use a table to collect the data group by pair hashcode
		
		return pair_positionList_table;
	}
	
	// this return list contains: int[0]=pair_hashcode, int[1]=position
	private List<int[]> getPairPositionList(byte[] fingerprint){
		
		int numFrames=FingerprintManager.getNumFrames(fingerprint);

		// table for paired frames
		byte[] pairedFrameTable=new byte[numFrames/anchorPointsIntervalLength+1];	// each second has numAnchorPointsPerSecond pairs only
		// end table for paired frames
		
		List<int[]> pairList=new LinkedList<int[]>();
		List<int[]> sortedCoordinateList=getSortedCoordinateList(fingerprint);

		Iterator<int[]> anchorPointListIterator=sortedCoordinateList.iterator();
		while (anchorPointListIterator.hasNext()){
			int[] anchorPoint=anchorPointListIterator.next();
			int anchorX=anchorPoint[0];
			int anchorY=anchorPoint[1];
			int numPairs=0;
			
			Iterator<int[]> targetPointListIterator=sortedCoordinateList.iterator();
			while (targetPointListIterator.hasNext()){
				
				if (numPairs>=maxPairs){
					break;
				}
				
				if (isReferencePairing && pairedFrameTable[anchorX/anchorPointsIntervalLength]>=numAnchorPointsPerInterval){
					break;
				}

				int[] targetPoint=targetPointListIterator.next();
				int targetX=targetPoint[0];
				int targetY=targetPoint[1];
				
				if (anchorX==targetX && anchorY==targetY){
					continue;
				}
				
				// pair up the points
				int x1,y1,x2,y2;	// x2 always >= x1
				if (targetX>=anchorX){
					x2=targetX;
					y2=targetY;
					x1=anchorX;
					y1=anchorY;					
				}
				else{
					x2=anchorX;
					y2=anchorY;
					x1=targetX;
					y1=targetY;	
				}
				
				// check target zone
				if ((x2-x1)>maxTargetZoneDistance){
					continue;
				}
				// end check target zone
				
				// check filter bank zone				
				if (!(y1/bandwidthPerBank == y2/bandwidthPerBank)){
					continue;	// same filter bank should have equal value
				}
				// end check filter bank zone
				
				int pairHashcode=(x2-x1)*numFrequencyUnits*numFrequencyUnits+y2*numFrequencyUnits+y1;	
				
				// stop list applied on sample pairing only
				if (!isReferencePairing && stopPairTable.containsKey(pairHashcode)){
					numPairs++;	// no reservation
					continue;	// escape this point only							
				}
				// end stop list applied on sample pairing only
				
				// pass all rules
				pairList.add(new int[]{pairHashcode,anchorX});
				pairedFrameTable[anchorX/anchorPointsIntervalLength]++;
				//System.out.println(anchorX+","+anchorY+"&"+targetX+","+targetY+":"+pairHashcode+" ("+pairedFrameTable[anchorX/anchorPointsIntervalLength]+")");
				numPairs++;
				// end pair up the points
			}
		}
		
		return pairList;
	}
		
	private List<int[]> getSortedCoordinateList(byte[] fingerprint){
		// each point data is 8 bytes 
		// first 2 bytes is x
		// next 2 bytes is y
		// next 4 bytes is intensity
		
		// get all intensities
		int numCoordinates=fingerprint.length/8;
		int[] intensities=new int[numCoordinates];
		for (int i=0; i<numCoordinates; i++){
			int pointer=i*8+4;
			int intensity=(int)(fingerprint[pointer]&0xff)<<24 | (int)(fingerprint[pointer+1]&0xff)<<16 | (int)(fingerprint[pointer+2]&0xff)<<8 | (int)(fingerprint[pointer+3]&0xff);
			intensities[i]=intensity;
		}
		
		QuickSortIndexPreserved quicksort=new QuickSortIndexPreserved(intensities);
		int[] sortIndexes=quicksort.getSortIndexes();
		
		List<int[]> sortedCoordinateList=new LinkedList<int[]>();
		for (int i=sortIndexes.length-1; i>=0; i--){
			int pointer=sortIndexes[i]*8;
			int x=(int)(fingerprint[pointer]&0xff)<<8 | (int)(fingerprint[pointer+1]&0xff);
			int y=(int)(fingerprint[pointer+2]&0xff)<<8 | (int)(fingerprint[pointer+3]&0xff);
			sortedCoordinateList.add(new int[]{x,y});
		}
		return sortedCoordinateList;
	}

	/**
	 * Convert hashed pair to bytes
	 * 
	 * @param pairHashcode hashed pair
	 * @return byte array
	 */
	public static byte[] pairHashcodeToBytes(int pairHashcode){	
		return new byte[]{(byte)(pairHashcode>>8),(byte)pairHashcode};
	}
	
	/**
	 * Convert bytes to hased pair
	 * 
	 * @param pairBytes
	 * @return hashed pair
	 */
	public static int pairBytesToHashcode(byte[] pairBytes){	
		return (int)(pairBytes[0]&0xFF)<<8|(int)(pairBytes[1]&0xFF);
	}
}