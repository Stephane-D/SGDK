package main;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import javax.swing.DefaultCellEditor;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.border.EmptyBorder;
import javax.swing.table.TableColumnModel;

import main.VGMFile.VGMTiming;
import sun.misc.IOUtils;

public class MainFrame extends JFrame implements ActionListener
{
    // right after _stext after all symbols
    final static int numMusicLocation = 0x27F54; // (old = 0x2BF3E, 0x2BF00)
    final static int XGMListLocation = 0x1329E; // (old = 0x2BF40, 0x2BF02)
    // should be aligned on 0x100
    final static int startXGMLocation = 0x28000; // (old = 0x2F000)

    // GUI
    JPanel contentPane;
    JButton btnClearList;
    JButton btnRemoveSelected;
    JButton btnAddVGM;
    JButton btnBuildROM;
    JTable xgmFileTable;
    JProgressBar processingBar;
    JTextField romTitleField;
    JProgressBar romUsageBar;
    JLabel romSizeLabel;

    // internals
    final VGMFileModel model;
    final byte[] rom;
    String lastFolder;

    /**
     * Create the frame.
     */
    public MainFrame()
    {
        super();

        initialize();

        model = new VGMFileModel();
        lastFolder = "";
        
        xgmFileTable.setModel(model);
        xgmFileTable.getTableHeader().setDefaultRenderer(new CustomHeaderCellRenderer());
        xgmFileTable.setSelectionMode(ListSelectionModel.SINGLE_INTERVAL_SELECTION);

        // set column default size
        final TableColumnModel colModel = xgmFileTable.getColumnModel();
        // number
        colModel.getColumn(0).setPreferredWidth(40);
        // filename
        colModel.getColumn(1).setPreferredWidth(320);
        // duration
        colModel.getColumn(2).setPreferredWidth(80);
        // timing
        colModel.getColumn(3).setPreferredWidth(80);
        colModel.getColumn(3).setCellEditor(new DefaultCellEditor(new JComboBox<VGMTiming>(VGMTiming.values())));

        // flags
        colModel.getColumn(4).setPreferredWidth(40);
        colModel.getColumn(5).setPreferredWidth(40);
        colModel.getColumn(6).setPreferredWidth(40);
        // size
        colModel.getColumn(7).setPreferredWidth(80);

        // maximum rom size is 4096 KB
        romUsageBar.setMaximum(4096);

        btnAddVGM.addActionListener(this);
        btnRemoveSelected.addActionListener(this);
        btnClearList.addActionListener(this);
        btnBuildROM.addActionListener(this);

        // update rom usage
        updateRomUsage();
        // center
        setLocationRelativeTo(null);

        // load rom from resource
        rom = new byte[4 * 1024 * 1024];

        try
        {
            final InputStream ip = getClass().getResourceAsStream("/rom.bin");
            final byte[] tmp = IOUtils.readFully(ip, -1, true);
            System.arraycopy(tmp, 0, rom, 0, tmp.length);
        }
        catch (IOException e)
        {
            System.err.println("Cannot read ROM from resource: " + e.getMessage());
        }
    }

    private void initialize()
    {
        setTitle("XGM rom builder 1.31");

        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setBounds(100, 100, 800, 381);
        setMinimumSize(new Dimension(640, 240));

        contentPane = new JPanel();
        contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
        contentPane.setLayout(new BorderLayout(0, 0));
        setContentPane(contentPane);

        JPanel panel = new JPanel();
        contentPane.add(panel, BorderLayout.EAST);
        GridBagLayout gbl_panel = new GridBagLayout();
        gbl_panel.columnWidths = new int[] {93, 0};
        gbl_panel.rowHeights = new int[] {23, 0, 0, 0, 0, 0, 0};
        gbl_panel.columnWeights = new double[] {0.0, Double.MIN_VALUE};
        gbl_panel.rowWeights = new double[] {0.0, 0.0, 0.0, 0.0, 1.0, 0.0, Double.MIN_VALUE};
        panel.setLayout(gbl_panel);

        btnClearList = new JButton("Clear list");
        GridBagConstraints gbc_btnClearList = new GridBagConstraints();
        gbc_btnClearList.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnClearList.insets = new Insets(0, 0, 5, 0);
        gbc_btnClearList.gridx = 0;
        gbc_btnClearList.gridy = 0;
        panel.add(btnClearList, gbc_btnClearList);

        btnRemoveSelected = new JButton("Remove selected");
        GridBagConstraints gbc_btnRemoveSelected = new GridBagConstraints();
        gbc_btnRemoveSelected.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnRemoveSelected.insets = new Insets(0, 0, 5, 0);
        gbc_btnRemoveSelected.gridx = 0;
        gbc_btnRemoveSelected.gridy = 1;
        panel.add(btnRemoveSelected, gbc_btnRemoveSelected);

        btnAddVGM = new JButton("Add VGM");
        GridBagConstraints gbc_btnAddVGM = new GridBagConstraints();
        gbc_btnAddVGM.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnAddVGM.insets = new Insets(0, 0, 5, 0);
        gbc_btnAddVGM.gridx = 0;
        gbc_btnAddVGM.gridy = 2;
        panel.add(btnAddVGM, gbc_btnAddVGM);

        processingBar = new JProgressBar();
        GridBagConstraints gbc_processingBar = new GridBagConstraints();
        gbc_processingBar.insets = new Insets(0, 0, 5, 0);
        gbc_processingBar.gridx = 0;
        gbc_processingBar.gridy = 3;
        panel.add(processingBar, gbc_processingBar);

        btnBuildROM = new JButton("Build Rom");
        GridBagConstraints gbc_btnBuildROM = new GridBagConstraints();
        gbc_btnBuildROM.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnBuildROM.anchor = GridBagConstraints.NORTH;
        gbc_btnBuildROM.gridx = 0;
        gbc_btnBuildROM.gridy = 5;
        panel.add(btnBuildROM, gbc_btnBuildROM);

        JPanel panel_1 = new JPanel();
        contentPane.add(panel_1, BorderLayout.SOUTH);
        GridBagLayout gbl_panel_1 = new GridBagLayout();
        gbl_panel_1.columnWidths = new int[] {8, 0, 0, 0, 80, 0};
        gbl_panel_1.rowHeights = new int[] {14, 0};
        gbl_panel_1.columnWeights = new double[] {0.0, 0.0, 1.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_panel_1.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        panel_1.setLayout(gbl_panel_1);

        JLabel lblRomUsage = new JLabel("Rom usage");
        GridBagConstraints gbc_lblRomUsage = new GridBagConstraints();
        gbc_lblRomUsage.insets = new Insets(0, 0, 0, 5);
        gbc_lblRomUsage.anchor = GridBagConstraints.NORTHWEST;
        gbc_lblRomUsage.gridx = 1;
        gbc_lblRomUsage.gridy = 0;
        panel_1.add(lblRomUsage, gbc_lblRomUsage);

        romUsageBar = new JProgressBar();
        GridBagConstraints gbc_romUsageBar = new GridBagConstraints();
        gbc_romUsageBar.fill = GridBagConstraints.HORIZONTAL;
        gbc_romUsageBar.insets = new Insets(0, 0, 0, 5);
        gbc_romUsageBar.gridx = 2;
        gbc_romUsageBar.gridy = 0;
        panel_1.add(romUsageBar, gbc_romUsageBar);

        romSizeLabel = new JLabel("0.00");
        GridBagConstraints gbc_romSizeLabel = new GridBagConstraints();
        gbc_romSizeLabel.insets = new Insets(0, 0, 0, 5);
        gbc_romSizeLabel.gridx = 3;
        gbc_romSizeLabel.gridy = 0;
        panel_1.add(romSizeLabel, gbc_romSizeLabel);

        JLabel lblMb = new JLabel("MB / 4 MB");
        GridBagConstraints gbc_lblMb = new GridBagConstraints();
        gbc_lblMb.anchor = GridBagConstraints.WEST;
        gbc_lblMb.gridx = 4;
        gbc_lblMb.gridy = 0;
        panel_1.add(lblMb, gbc_lblMb);

        JScrollPane scrollPane = new JScrollPane();
        contentPane.add(scrollPane, BorderLayout.CENTER);

        xgmFileTable = new JTable();
        scrollPane.setViewportView(xgmFileTable);

        JPanel panel_2 = new JPanel();
        contentPane.add(panel_2, BorderLayout.NORTH);
        GridBagLayout gbl_panel_2 = new GridBagLayout();
        gbl_panel_2.columnWidths = new int[] {8, 0, 0, 0, 0};
        gbl_panel_2.rowHeights = new int[] {14, 0};
        gbl_panel_2.columnWeights = new double[] {0.0, 0.0, 0.0, 1.0, Double.MIN_VALUE};
        gbl_panel_2.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        panel_2.setLayout(gbl_panel_2);

        JLabel lblDragAndDrop = new JLabel("ROM Title");
        GridBagConstraints gbc_lblDragAndDrop = new GridBagConstraints();
        gbc_lblDragAndDrop.fill = GridBagConstraints.VERTICAL;
        gbc_lblDragAndDrop.insets = new Insets(0, 0, 0, 5);
        gbc_lblDragAndDrop.anchor = GridBagConstraints.EAST;
        gbc_lblDragAndDrop.gridx = 1;
        gbc_lblDragAndDrop.gridy = 0;
        panel_2.add(lblDragAndDrop, gbc_lblDragAndDrop);

        romTitleField = new JTextField();
        romTitleField.setText("XGM Compilation");
        GridBagConstraints gbc_romTitleField = new GridBagConstraints();
        gbc_romTitleField.insets = new Insets(0, 0, 0, 5);
        gbc_romTitleField.fill = GridBagConstraints.BOTH;
        gbc_romTitleField.gridx = 2;
        gbc_romTitleField.gridy = 0;
        panel_2.add(romTitleField, gbc_romTitleField);
        romTitleField.setColumns(20);
    }

    void updateRomUsage()
    {
        int size = startXGMLocation;

        // VGM length should be aligned on 0x100
        for (VGMFile vgm : model.vgmFiles)
            size += vgm.xgc.length;

        romUsageBar.setValue(size / 1024);
        if (romUsageBar.getValue() >= 4096)
            romUsageBar.setForeground(Color.red);
        else
            romUsageBar.setForeground(Color.green);
        romSizeLabel.setText(String.format("%.2g", Double.valueOf(size / (1024d * 1024d))));
    }

    @Override
    public void actionPerformed(ActionEvent e)
    {
        final Object source = e.getSource();

        if (source == btnAddVGM)
        {
            final JFileChooser f = new JFileChooser();

            f.setSelectedFile(new File(lastFolder+"/music.vgm"));
            f.setFileSelectionMode(JFileChooser.FILES_ONLY);
            f.setMultiSelectionEnabled(true);

            if (f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION)
            {
                lastFolder = f.getSelectedFile().getParent();
                new Thread(new VGMAdder(f.getSelectedFiles()), "VGM adder").start();
            }
        }
        else if (source == btnRemoveSelected)
        {
            final int[] selected = xgmFileTable.getSelectedRows();

            // remove selected
            for (int i = selected.length - 1; i >= 0; i--)
                model.vgmFiles.remove(selected[i]);

            model.fireTableDataChanged();
            updateRomUsage();
        }
        else if (source == btnClearList)
        {
            // clear VGM list
            model.vgmFiles.clear();
            model.fireTableDataChanged();
            updateRomUsage();
        }
        else if (source == btnBuildROM)
        {
            new Thread(new ROMBuilder(), "ROM Builder").start();
        }
    }

    boolean addVGM(File file)
    {
        // limit raised...
        if (model.vgmFiles.size() >= 100)
            return false;

        if (file.exists())
        {
            try
            {
                VGMFile vgmFile = compileVGM(file);

                if (vgmFile != null)
                {
                    model.vgmFiles.add(vgmFile);
                    model.fireTableDataChanged();
                }
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }

        return true;
    }

    VGMFile compileVGM(VGMFile vgm) throws InterruptedException, IOException
    {
        final String inputFile = vgm.file.getAbsolutePath();
        final String outputFile = "out.xgc";
        String cmd = "xgmtool ";
        cmd += "\"" + inputFile + "\"";
        cmd += " " + outputFile + " -s";
        if (vgm.disablePCMIgnore)
            cmd += " -di";
        if (vgm.disablePCMRateFix)
            cmd += " -dr";
        if (vgm.disableDelayedKeyOFF)
            cmd += " -dd";
        switch (vgm.timing)
        {
            case NTSC:
                cmd += " -n";
                break;
            case PAL:
                cmd += " -p";
                break;
            default:
                break;
        }

        final Process process = Util.exec(cmd, "./");

        if (process.waitFor() == 0)
        {
            vgm.xgc = Util.readBinaryFile(outputFile);
            Util.deleteFile(outputFile);
            vgm.duration = Util.getXGCDuration(vgm.xgc);
            return vgm;
        }

        System.err.println("An error occured during conversion of " + inputFile);

        return null;
    }

    private VGMFile compileVGM(File file) throws InterruptedException, IOException
    {
        try
        {
            final String filename = file.getAbsolutePath();
            final String outName = Util.getFileName(filename, false) + ".vgm";
            // try zip first
            if (Util.unzipSingle(filename, outName))
                return compileVGM(new VGMFile(new File(outName)));
            // try gzip
            Util.gunzipFile(filename, outName);
            return compileVGM(new VGMFile(new File(outName)));
        }
        catch (Exception e)
        {
            // probably not a zipped file
        }

        return compileVGM(new VGMFile(file));
    }

    class VGMAdder implements Runnable
    {
        final File[] files;

        public VGMAdder(File[] files)
        {
            super();

            this.files = files;
        }

        @Override
        public void run()
        {
            btnAddVGM.setEnabled(false);
            try
            {
                processingBar.setMaximum(files.length);
                processingBar.setValue(0);
                processingBar.setStringPainted(true);
                processingBar.setString("Processing...");

                for (int i = 0; i < files.length; i++)
                {
                    if (!addVGM(files[i]))
                    {
                        if (model.vgmFiles.size() >= 100)
                            JOptionPane.showMessageDialog(MainFrame.this, "Maximum number of track is limited to 100");
                        else
                            JOptionPane.showMessageDialog(MainFrame.this,
                                    "An error occured while compiling the VGM file");
                    }
                    processingBar.setValue(i + 1);
                    updateRomUsage();
                }

                processingBar.setValue(0);
                processingBar.setStringPainted(false);
            }
            finally
            {
                btnAddVGM.setEnabled(true);
            }
        }
    }

    class ROMBuilder implements Runnable
    {
        public ROMBuilder()
        {
            super();
        }

        @Override
        public void run()
        {
            btnBuildROM.setEnabled(false);
            try
            {
                final List<VGMFile> vgms = model.vgmFiles;
                final int len = vgms.size();

                processingBar.setMaximum(len);
                processingBar.setValue(0);
                processingBar.setStringPainted(true);
                processingBar.setString("Building rom...");

                String romTitle = romTitleField.getText();
                while (romTitle.length() < 0x30)
                    romTitle += " ";

                // set ROM title
                for (int i = 0; i < 0x30; i++)
                {
                    final byte ch = (byte) romTitle.charAt(i);
                    rom[i + 0x120] = ch;
                    rom[i + 0x150] = ch;
                }

                // set music number
                Util.setInt16Swapped(rom, numMusicLocation, len);
                // Util.setInt16Swapped(rom, numMusicLocationAlt, len);

                try
                {
                    int vgmAddr = startXGMLocation;
                    for (int i = 0; i < vgms.size(); i++)
                    {
                        // set VGM address in list
                        Util.setIntSwapped(rom, XGMListLocation + (i * 4), vgmAddr);

                        final VGMFile vgm = compileVGM(vgms.get(i));
                        System.arraycopy(vgm.xgc, 0, rom, vgmAddr, vgm.xgc.length);
                        vgmAddr = (vgmAddr + vgm.xgc.length + 0xFF) & 0xFFFF00;

                        processingBar.setValue(i + 1);
                    }

                    // align output size on 128KB
                    final byte[] out = new byte[(vgmAddr + 0x1FFFF) & 0x7E0000];
                    System.arraycopy(rom, 0, out, 0, vgmAddr);

                    Util.writeBinaryFile(out, "xgmrom.bin");
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }

                processingBar.setValue(0);
                processingBar.setStringPainted(false);
            }
            finally
            {
                btnBuildROM.setEnabled(true);
            }
        }
    }
}
