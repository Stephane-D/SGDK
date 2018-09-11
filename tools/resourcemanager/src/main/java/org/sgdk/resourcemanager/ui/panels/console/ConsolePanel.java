package org.sgdk.resourcemanager.ui.panels.console;

import java.awt.GridLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.text.BadLocationException;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyledDocument;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class ConsolePanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public ConsolePanel(ResourceManagerFrame parent) throws IOException {
		super(new GridLayout(1,1));		
		setBorder(BorderFactory.createTitledBorder("Console"));
		
		JTextPane console = new JTextPane();
		console.setEditable (false);
		console.setAutoscrolls(true);
				
		JTextPaneAppender.addTextPane(console);
		
		for(int i = 0; i<100; i++) {
			StyledDocument myDoc =console.getStyledDocument();
			try {
				myDoc.insertString(myDoc.getLength(), "\n\n", new SimpleAttributeSet());
			} catch (BadLocationException e) {
				e.printStackTrace();
			}
		}
		JScrollPane jScrollPane = new JScrollPane(console);
		add(jScrollPane);
	}

}
