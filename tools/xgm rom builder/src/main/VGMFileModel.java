/**
 * 
 */
package main;

import java.util.ArrayList;
import java.util.List;

import javax.swing.table.AbstractTableModel;

import main.VGMFile.VGMTiming;

/**
 * @author stephane
 */
public class VGMFileModel extends AbstractTableModel
{
    final static String[] columns = new String[] {"Num", "Filename", "Duration", "Timing", "DI", "DR", "DD",
            "Size (KB)"};
    final List<VGMFile> vgmFiles;

    public VGMFileModel()
    {
        super();

        vgmFiles = new ArrayList<VGMFile>();
    }

    @Override
    public int getRowCount()
    {
        return vgmFiles.size();
    }

    @Override
    public int getColumnCount()
    {
        return columns.length;
    }

    @Override
    public String getColumnName(int columnIndex)
    {
        return columns[columnIndex];
    }

    @Override
    public Class<?> getColumnClass(int columnIndex)
    {
        switch (columnIndex)
        {
            default:
                return String.class;

            case 0:
                return Integer.class;

            case 3:
                return VGMTiming.class;

            case 4:
            case 5:
            case 6:
                return Boolean.class;

            case 7:
                return Double.class;
        }
    }

    @Override
    public boolean isCellEditable(int rowIndex, int columnIndex)
    {
        switch (columnIndex)
        {
            default:
                return false;

            case 3:
            case 4:
            case 5:
            case 6:
                return true;
        }
    }

    @Override
    public Object getValueAt(int rowIndex, int columnIndex)
    {
        final VGMFile vgmFile = vgmFiles.get(rowIndex);

        switch (columnIndex)
        {
            case 0:
                return Integer.valueOf(rowIndex);

            default:
            case 1:
                return Util.getFileName(vgmFile.file.getAbsolutePath(), true);

            case 2:
                return Util.convertFrameToTime(vgmFile.duration);
            case 3:
                return vgmFile.timing;
            case 4:
                return Boolean.valueOf(vgmFile.disablePCMIgnore);
            case 5:
                return Boolean.valueOf(vgmFile.disablePCMRateFix);
            case 6:
                return Boolean.valueOf(vgmFile.disableDelayedKeyOFF);
            case 7:
                final int len = (int) ((vgmFile.xgc.length / 1024d) * 10);
                return Double.valueOf(len / 10d);
        }
    }

    @Override
    public void setValueAt(Object value, int rowIndex, int columnIndex)
    {
        final VGMFile vgmFile = vgmFiles.get(rowIndex);

        switch (columnIndex)
        {
            case 3:
                vgmFile.timing = (VGMTiming) value;
                break;
            case 4:
                vgmFile.disablePCMIgnore = ((Boolean) value).booleanValue();
                break;
            case 5:
                vgmFile.disablePCMRateFix = ((Boolean) value).booleanValue();
                break;
            case 6:
                vgmFile.disableDelayedKeyOFF = ((Boolean) value).booleanValue();
                break;
        }
    }
}
