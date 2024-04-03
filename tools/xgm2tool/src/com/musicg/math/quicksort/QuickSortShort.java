package com.musicg.math.quicksort;
public class QuickSortShort extends QuickSort{
	
	private int[] indexes;
	private short[] array;
	
	public QuickSortShort(short[] array){
		this.array=array;
		indexes=new int[array.length];
		for (int i=0; i<indexes.length; i++){
			indexes[i]=i;
		}
	}
	
	public int[] getSortIndexes(){
		sort();
		return indexes;
	}
	
	private void sort() {
	    quicksort(array, indexes, 0, indexes.length - 1);
	}
	
	// quicksort a[left] to a[right]
	private void quicksort(short[] a, int[] indexes, int left, int right) {
	    if (right <= left) return;
	    int i = partition(a, indexes, left, right);
	    quicksort(a, indexes, left, i-1);
	    quicksort(a, indexes, i+1, right);
	}
	
	// partition a[left] to a[right], assumes left < right
	private int partition(short[] a, int[] indexes, int left, int right) {
	    int i = left - 1;
	    int j = right;
	    while (true) {
	        while (a[indexes[++i]]<a[indexes[right]]);      // find item on left to swap, a[right] acts as sentinel
	        while (a[indexes[right]]<a[indexes[--j]]){      // find item on right to swap
	            if (j == left) break;           					// don't go out-of-bounds
	        }
	        if (i >= j) break;                  					// check if pointers cross
	        swap(a, indexes, i, j);              					 // swap two elements into place
	    }
	    swap(a, indexes, i, right);              					 // swap with partition element
	    return i;
	}
		
	// exchange a[i] and a[j]
	private void swap(short[] a, int[] indexes, int i, int j) {
	    int swap = indexes[i];
	    indexes[i] = indexes[j];
	    indexes[j] = swap;
	}
	
}