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
 * Evaluate the mean of an array
 * @author Jacquet Wong
 *
 */
public class Mean extends MathStatistics{
	
	private Sum sum=new Sum();
	
	public Mean(){
	}
	
	public Mean(double[] values){
		setValues(values);
	}
	
	public double evaluate(){	
		sum.setValues(values);
		double mean=sum.evaluate()/sum.size();
		return mean;
	}
}