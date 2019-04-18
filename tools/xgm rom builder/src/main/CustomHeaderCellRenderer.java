package main;

import java.awt.Component;

import javax.swing.JComponent;
import javax.swing.JTable;

import sun.swing.table.DefaultTableCellHeaderRenderer;

public class CustomHeaderCellRenderer extends DefaultTableCellHeaderRenderer
{
    final static String[] columnsTooltip = new String[] {"Number", "XGM Filename", "XGM music Duration",
            "XGM Timing (use AUTO if unsure)", "Disable PCM auto ignore", "Disable PCM rate auto fix",
            "Disable delayed key OFF", "Compiled XGM size"};

    @Override
    public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus,
            int row, int column)
    {
        final Component result = super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

        if (result instanceof JComponent)
            ((JComponent) result).setToolTipText(columnsTooltip[column]);

        return result;
    }
}
