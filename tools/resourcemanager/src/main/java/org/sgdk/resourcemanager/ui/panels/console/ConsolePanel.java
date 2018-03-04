package org.sgdk.resourcemanager.ui.panels.console;

import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.io.IOException;

import javax.swing.BorderFactory;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import org.sgdk.resourcemanager.ui.ResourceManagerFrame;

public class ConsolePanel extends JPanel {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	public ConsolePanel(ResourceManagerFrame parent) throws IOException {
		super(new GridBagLayout());		
		setBorder(BorderFactory.createTitledBorder("Console"));
		
		JTextArea console = new JTextArea(3,15);
		console.setLineWrap(true);
		console.setWrapStyleWord(true);
		console.setEditable (false);
		console.setFont(new Font("Courier", Font.PLAIN, 12));
		console.setAutoscrolls(true);
		
		JTextAreaAppender.addTextArea(console);
		
		GridBagConstraints c = new GridBagConstraints();
		
		JScrollPane scrollPaneConsole = new JScrollPane(console);
		
		c.fill = GridBagConstraints.BOTH;
		c.weightx = 1.0;
		c.weighty = 1.0;	
		c.gridx = 0;
		c.gridy = 0;
		c.gridwidth = GridBagConstraints.REMAINDER;
		c.gridheight = GridBagConstraints.REMAINDER;
		add(scrollPaneConsole, c);
	}

}
