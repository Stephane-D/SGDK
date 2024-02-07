package sgdk.xgm2tool.gui;

import java.io.IOException;

import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.format.VGM;
import sgdk.xgm2tool.tool.Util;

public class VGMTable extends CommandsTable
{
    public VGM vgm;

    public VGMTable()
    {
        super();

        titleLabel.setText("VGM");
    }

    @Override
    protected void doLoadAction() throws IOException
    {
        final String filename = Launcher.mainGui.getInputFilename();

        // Load VGM
        byte[] data = Util.readBinaryFile(filename);
        if (data == null)
            return;

        vgm = new VGM(data, false);
        commands = vgm.commands;
        pal = (vgm.rate == 50) ? true : false;

        // notify data changed
        tableModel.fireTableDataChanged();
        commandNumberField.setText(Integer.toString(commands.size()));
        lengthField.setText(Util.formatVGMTime(vgm.getTotalTime()));
        sizeField.setText(String.format("%.2f Kb", Double.valueOf(vgm.asByteArray().length / 1024d)));
    }

    @Override
    protected void doSaveAction() throws IOException
    {
        final String filename = Launcher.mainGui.getOutputFilename();

        // Save VGM
        if (vgm != null)
            Util.writeBinaryFile(vgm.asByteArray(), filename);
    }
}
