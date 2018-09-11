package org.sgdk.resourcemanager.ui.panels.console;

import java.awt.Color;
import java.util.Hashtable;

import javax.swing.JTextPane;
import javax.swing.SwingUtilities;
import javax.swing.text.MutableAttributeSet;
import javax.swing.text.SimpleAttributeSet;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyledDocument;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.appender.AbstractAppender;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginElement;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.layout.PatternLayout;

@Plugin(name = "JTextPaneAppender", category = "Core", elementType = "appender", printObject = true)
public class JTextPaneAppender extends AbstractAppender {

    private static JTextPane jTextPane = null;

    private int maxLines = 0;

    private Hashtable<String, MutableAttributeSet> myAttributeSet; 
    
    protected JTextPaneAppender(String name, Layout<?> layout, Filter filter, int maxLines, boolean ignoreExceptions) {
        super(name, filter, layout, ignoreExceptions);
        this.maxLines = maxLines;
        
        createAttributes();
    }
    
	private void createAttributes() {
		String prio[] = new String[6];
		prio[0] = Level.FATAL.toString();
		prio[1] = Level.ERROR.toString();
		prio[2] = Level.WARN.toString();
		prio[3] = Level.INFO.toString();
		prio[4] = Level.DEBUG.toString();
		prio[5] = Level.TRACE.toString();

		myAttributeSet = new Hashtable<String, MutableAttributeSet>();

		for (int i = 0; i < prio.length; i++) {
			MutableAttributeSet att = new SimpleAttributeSet();
			myAttributeSet.put(prio[i], att);
			StyleConstants.setFontSize(att, 10);
		}

		StyleConstants.setForeground(myAttributeSet.get(Level.FATAL.toString()), Color.red);
		StyleConstants.setForeground(myAttributeSet.get(Level.ERROR.toString()), Color.red);
		StyleConstants.setForeground(myAttributeSet.get(Level.WARN.toString()), Color.orange);
		StyleConstants.setForeground(myAttributeSet.get(Level.INFO.toString()), Color.black);
		StyleConstants.setForeground(myAttributeSet.get(Level.DEBUG.toString()), Color.black);
		StyleConstants.setForeground(myAttributeSet.get(Level.TRACE.toString()), Color.black);
	}

    @PluginFactory
    public static JTextPaneAppender createAppender(@PluginAttribute("name") String name,
                                              @PluginAttribute("maxLines") int maxLines,
                                              @PluginAttribute("ignoreExceptions") boolean ignoreExceptions,
                                              @PluginElement("Layout") Layout<?> layout,
                                              @PluginElement("Filters") Filter filter) {

        if (name == null) {
            LOGGER.error("No name provided for JTextPaneAppender");
            return null;
        }

        if (layout == null) {
            layout = PatternLayout.createDefaultLayout();
        }
        return new JTextPaneAppender(name, layout, filter, maxLines, ignoreExceptions);
    }

    // Add the target JTextPane to be populated and updated by the logging information.
    public static void addTextPane(final JTextPane textPane) {
        JTextPaneAppender.jTextPane = textPane;
    }

    @Override
	public void append(LogEvent event) {
		if (jTextPane == null) {
			LOGGER.warn("TextPane is not initialized");
			return;
		}
		final String message = new String(this.getLayout().toByteArray(event));
		SwingUtilities.invokeLater(new Runnable() {
             @Override
             public void run() {
				StyledDocument myDoc =jTextPane.getStyledDocument();
				try {
					myDoc.insertString(myDoc.getLength(), message, myAttributeSet 
					     .get(event.getLevel().toString()));
					if (myDoc.getDefaultRootElement().getElementCount() > maxLines) 
					     myDoc.remove(0, myDoc.getDefaultRootElement().getElement(0).getElement(0).getEndOffset());
				} catch (final Throwable t) {
					System.out.println("Unable to append log to text area: " + t.getMessage());
				}
				jTextPane.setCaretPosition(myDoc.getLength()); 
             }
        });
	}
}
           
                	
              
       