package sgdk.xgm2tool.gui;

import java.io.IOException;
import java.util.ArrayList;

import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.format.Command;
import sgdk.xgm2tool.format.VGM;
import sgdk.xgm2tool.format.XGM;
import sgdk.xgm2tool.tool.Util;

public class XGMTable extends CommandsTable
{
    public XGM xgm;
    private final boolean packed;

    public XGMTable(boolean packed)
    {
        super();

        this.packed = packed;

        titleLabel.setText("XGM");
    }

    public void convert(VGM input) throws IOException
    {
        if (input == null)
            return;

        // Convert VGM to XGM
        xgm = new XGM(input, packed);
        commands = new ArrayList(xgm.FMcommands);
        commands.addAll(xgm.PSGcommands);
        commands.sort(Command.timeComparator);
        pal = xgm.pal;

        // notify data changed
        tableModel.fireTableDataChanged();
        commandNumberField.setText(Integer.toString(commands.size()));
        lengthField.setText(Util.formatVGMTime(xgm.getTotalTime()));
        sizeField.setText(String.format("%.2f Kb", Double.valueOf(xgm.asByteArray().length / 1024d)));
    }

    @Override
    protected void doLoadAction() throws IOException
    {
        final String filename = Launcher.mainGui.getInputFilename();

        // Load VGM
        byte[] data = Util.readBinaryFile(filename);
        if (data == null)
            return;

        // load XGM
        xgm = new XGM(data);
        commands = new ArrayList(xgm.FMcommands);
        commands.addAll(xgm.PSGcommands);
        commands.sort(Command.timeComparator);
        pal = xgm.pal;

        // notify data changed
        tableModel.fireTableDataChanged();
        commandNumberField.setText(Integer.toString(commands.size()));
        lengthField.setText(Util.formatVGMTime(xgm.getTotalTime()));
        sizeField.setText(String.format("%.2f Kb", Double.valueOf(xgm.asByteArray().length / 1024d)));
    }

    @Override
    protected void doSaveAction() throws IOException
    {
        final String filename = Launcher.mainGui.getOutputFilename();

        // Save XGM
        if (xgm != null)
            Util.writeBinaryFile(xgm.asByteArray(), filename);
    }
}
