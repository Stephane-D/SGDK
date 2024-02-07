package sgdk.xgm2tool.gui;

import java.io.IOException;

import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.format.VGM;
import sgdk.xgm2tool.format.XGM;
import sgdk.xgm2tool.tool.Util;

public class VGMOptTable extends CommandsTable
{
    public VGM vgm;

    public VGMOptTable()
    {
        super();

        titleLabel.setText("Optimised VGM");
    }

    public void optimize(VGM input) throws IOException
    {
        if (input == null)
            return;

        // Optimize VGM
        vgm = new VGM(input, true);

        commands = vgm.commands;
        pal = (vgm.rate == 50) ? true : false;

        // notify data changed
        tableModel.fireTableDataChanged();
        commandNumberField.setText(Integer.toString(commands.size()));
        lengthField.setText(Util.formatVGMTime(vgm.getTotalTime()));
        sizeField.setText(String.format("%.2f Kb", Double.valueOf(vgm.asByteArray().length / 1024d)));
    }

    public void convert(XGM input) throws IOException
    {
        if (input == null)
            return;

        // Convert XGM to VGM
        vgm = new VGM(input);
        commands = vgm.commands;
        pal = (vgm.rate == 50) ? true : false;

        // notify data changed
        tableModel.fireTableDataChanged();
        commandNumberField.setText(Integer.toString(commands.size()));
        lengthField.setText(Util.formatVGMTime(vgm.getTotalTime()));
        sizeField.setText(String.format("%.2f Kb", Double.valueOf(vgm.asByteArray().length / 1024d)));
    }

    @Override
    protected void doLoadAction() throws IOException
    {
        final String filename = Launcher.mainGui.getInputFilename();

        // Load VGM
        byte[] data = Util.readBinaryFile(filename);
        if (data == null)
            return;

        vgm = new VGM(data, true);
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
