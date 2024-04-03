package com.musicg.serialization;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

public class ObjectSerializer{
	
	public ObjectSerializer(){
	}
	
	public void dump(Object object, String dumpFile){
		// serialize the object
		try {
			FileOutputStream fout = new FileOutputStream(dumpFile);
			ObjectOutputStream oos = new ObjectOutputStream(fout);
			oos.writeObject(object);
			oos.close();
			fout.close();
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public Object load(String dumpFile){
		
		Object object=null;
		
		// load the memory
		try {
			FileInputStream fin=new FileInputStream(dumpFile);
		    ObjectInputStream ois=new ObjectInputStream(fin);
		    object=ois.readObject();
		    ois.close();
		    fin.close();
		}
		catch (Exception e) {
			e.printStackTrace();
		}

		return object;
	}
}