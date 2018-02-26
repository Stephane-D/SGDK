package org.sgdk.resourcemanager.entities.exceptions;

public class SGDKInvalidFormatException extends Exception {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public SGDKInvalidFormatException() {
		super("Invalid Format File");
	}
	
	public SGDKInvalidFormatException(String msg) {
		super(msg);
	}
	
	public SGDKInvalidFormatException(String msg, Throwable cause) {
		super(msg, cause);
	}
}
