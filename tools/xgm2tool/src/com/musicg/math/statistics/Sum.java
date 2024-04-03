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
 * Evaluate the sum of an array
 * 
 * @author Jacquet Wong
 *
 */
public class Sum extends MathStatistics{

	public Sum(){		
	}
	
	public Sum(double[] values){		
		setValues(values);
	}
	
	public double evaluate(){
		double sum=0;
		int size=values.length;
		for (int i=0 ;i<size; i++){
			sum+=values[i];
		}
		return sum;
	}
	
	public int size(){
		return values.length;
	}
}