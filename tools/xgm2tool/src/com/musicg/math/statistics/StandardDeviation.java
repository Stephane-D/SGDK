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

package com.musicg.math.statistics;

/**
 * Evaluate the standard deviation of an array
 * 
 * @author Jacquet Wong
 *
 */
public class StandardDeviation extends MathStatistics{
	
	private Mean mean=new Mean();
	
	public StandardDeviation(){
		
	}
	
	public StandardDeviation(double[] values){
		setValues(values);
	}
	
	public double evaluate(){
		
		mean.setValues(values);
		double meanValue=mean.evaluate();
		
		int size=values.length;
		double diffSquare=0;
		double sd=Double.NaN;
		
		for (int i=0; i<size; i++){
			diffSquare+=Math.pow(values[i]-meanValue,2);
		}
		
		if (size>0){
			sd=Math.sqrt(diffSquare/size);
		}
		
		return sd;
	}
}