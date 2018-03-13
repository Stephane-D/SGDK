package org.sgdk.resourcemanager.ui.panels.console;

import java.util.ArrayList;

import javax.swing.JTextArea;
import javax.swing.SwingUtilities;

import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.appender.AbstractAppender;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginElement;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.layout.PatternLayout;

@Plugin(name = "JTextAreaAppender", category = "Core", elementType = "appender", printObject = true)
public class JTextAreaAppender extends AbstractAppender {

    private static volatile ArrayList<JTextArea> jTextAreaList = new ArrayList<JTextArea>();

    private int maxLines = 0;

    protected JTextAreaAppender(String name, Layout<?> layout, Filter filter, int maxLines, boolean ignoreExceptions) {
        super(name, filter, layout, ignoreExceptions);
        this.maxLines = maxLines;
    }

    @PluginFactory
    public static JTextAreaAppender createAppender(@PluginAttribute("name") String name,
                                              @PluginAttribute("maxLines") int maxLines,
                                              @PluginAttribute("ignoreExceptions") boolean ignoreExceptions,
                                              @PluginElement("Layout") Layout<?> layout,
                                              @PluginElement("Filters") Filter filter) {

        if (name == null) {
            LOGGER.error("No name provided for JTextAreaAppender");
            return null;
        }

        if (layout == null) {
            layout = PatternLayout.createDefaultLayout();
        }
        return new JTextAreaAppender(name, layout, filter, maxLines, ignoreExceptions);
    }

    // Add the target JTextArea to be populated and updated by the logging information.
    public static void addTextArea(final JTextArea textArea) {
        JTextAreaAppender.jTextAreaList.add(textArea);
    }

    @Override
    public void append(LogEvent event) {
        // TODO Auto-generated method stub
        final String message = new String(this.getLayout().toByteArray(event));

        // Append formatted message to text area using the Thread.
        try {
            SwingUtilities.invokeLater(new Runnable() {
                @Override
                public void run() {
                    for (JTextArea jTA : jTextAreaList){
                        try {
                            if (jTA != null) {
                                if (jTA.getText().length() == 0) {
                                    jTA.setText(message);
                                } else {
                                    jTA.append("\n" + message);
                                    if (maxLines > 0 & jTA.getLineCount() > maxLines + 1) {
                                        int endIdx = jTA.getDocument().getText(0, jTA.getDocument().getLength()).indexOf("\n", 0);
                                        jTA.getDocument().remove(0, endIdx+1);
                                    }
                                }
                                String content = jTA.getText();
                                jTA.setText(content.substring(0,content.length()-1));
                            }
                        } catch (final Throwable t) {
                            System.out.println("Unable to append log to text area: "
                                    + t.getMessage());
                        }
                    }
                }
            });
        } catch (final IllegalStateException e) {
            // ignore case when the platform hasn't yet been iniitialized
        }
    }
}