/*
 * Copyright (C) 2011 Jacquet Wong
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

package com.musicg.experiment.math.cluster;
import java.util.LinkedList;
import java.util.List;

import com.musicg.pitch.PitchHandler;

public class SegmentCluster{
	
	private double diffThreshold;
	
	public SegmentCluster(){
		this.diffThreshold=1;
	}
	
	public SegmentCluster(double diffThreshold){
		this.diffThreshold=diffThreshold;
	}
	
	public void setDiffThreshold(double diffThreshold){
		this.diffThreshold=diffThreshold;
	}
	
	public List<Segment> getSegments(double[] array){
		
		PitchHandler pitchHandler=new PitchHandler();
		List<Segment> segmentList=new LinkedList<Segment>();
		
		double segmentMean=0;
		int segmentSize=0;
		
		if (array.length>0){
			segmentMean=array[1];
			segmentSize=1;
		}
		
		for (int i=1; i<array.length; i++){
			double diff=Math.abs(pitchHandler.getToneChanged(array[i], segmentMean));
			if (diff<diffThreshold){
				// same cluster, add to the cluster and change the cluster data
				segmentMean=(segmentMean*segmentSize+array[i])/(++segmentSize);
			}
			else{
				// not the cluster, create a new one
				//System.out.println("New Cluster: "+clusterMean);
				Segment segment=new Segment();
				segment.setMean(segmentMean);
				segment.setStartPosition(i-segmentSize);
				segment.setSize(segmentSize);
				segmentList.add(segment);
				// end current cluster
				
				// set new cluster
				segmentMean=array[i];
				segmentSize=1;
			}
		}
		
		// add the last cluster
		Segment segment=new Segment();
		segment.setMean(segmentMean);
		segment.setStartPosition(array.length-segmentSize);
		segment.setSize(segmentSize);
		segmentList.add(segment);
		
		return segmentList;
	}
}