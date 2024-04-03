package com.musicg.processor;

import com.musicg.math.rank.ArrayRankDouble;

public class RobustIntensityProcessor implements IntensityProcessor{

	private double[][] intensities;
	private int numPointsPerFrame;
	
	public RobustIntensityProcessor(double[][] intensities, int numPointsPerFrame){
		this.intensities=intensities;
		this.numPointsPerFrame=numPointsPerFrame;
	}
	
	public void execute(){
		
		int numX=intensities.length;
		int numY=intensities[0].length;
		double[][] processedIntensities=new double[numX][numY];
		
		for (int i=0; i<numX; i++){
			double[] tmpArray=new double[numY];
			System.arraycopy(intensities[i], 0, tmpArray, 0, numY);
			
			// pass value is the last some elements in sorted array	
			ArrayRankDouble arrayRankDouble=new ArrayRankDouble();
			double passValue=arrayRankDouble.getNthOrderedValue(tmpArray,numPointsPerFrame,false);
			
			// only passed elements will be assigned a value
			for (int j=0; j<numY; j++){
				if (intensities[i][j]>=passValue){
					processedIntensities[i][j]=intensities[i][j];
				}
			}
		}
		intensities=processedIntensities;
	}
	
	public double[][] getIntensities(){
		return intensities;
	}
}