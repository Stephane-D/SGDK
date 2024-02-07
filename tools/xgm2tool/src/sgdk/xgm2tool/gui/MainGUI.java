package sgdk.xgm2tool.gui;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSplitPane;
import javax.swing.JTextField;
import javax.swing.JToggleButton;

import sgdk.xgm2tool.gui.CommandsTable.CommandsTableListener;

public class MainGUI extends JPanel implements ActionListener, CommandsTableListener
{
    // GUI
    private JButton btnOptimizeVGM;
    private JTextField timeField;
    private JButton btnGoTo;
    private JTextField timeLabel;
    private JButton btnConvertXGM;
    private JButton btnXgmToVgm;
    private JButton btnConvertXGC;
    private JLabel lblSourceFile;
    private JTextField inputFileField;
    private JLabel lblOutputFile;
    private JTextField outputFileField;
    private JButton selectInputButton;
    private JButton selectOutputButton;
    private JToggleButton syncButton;

    private CustomTabPane tabPaneTop;
    private CustomTabPane tabPaneBottom;

    public MainGUI()
    {
        super();

        initialize();

        selectInputButton.addActionListener(this);
        selectOutputButton.addActionListener(this);

        btnOptimizeVGM.addActionListener(this);
        btnConvertXGM.addActionListener(this);
        btnConvertXGC.addActionListener(this);
        btnXgmToVgm.addActionListener(this);
        btnGoTo.addActionListener(this);
    }

    private void initialize()
    {
        setLayout(new BorderLayout(0, 0));

        tabPaneTop = new CustomTabPane(this);
        tabPaneBottom = new CustomTabPane(this);

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, tabPaneTop, tabPaneBottom);
        add(splitPane, BorderLayout.CENTER);

        JPanel topPanel = new JPanel();
        add(topPanel, BorderLayout.NORTH);
        GridBagLayout gbl_topPanel = new GridBagLayout();
        gbl_topPanel.columnWidths = new int[] {80, 0, 32, 0, 80, 0, 0, 0, 0};
        gbl_topPanel.rowHeights = new int[] {0, 0};
        gbl_topPanel.columnWeights = new double[] {0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_topPanel.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        topPanel.setLayout(gbl_topPanel);

        lblSourceFile = new JLabel("Input file");
        GridBagConstraints gbc_lblSourceFile = new GridBagConstraints();
        gbc_lblSourceFile.fill = GridBagConstraints.VERTICAL;
        gbc_lblSourceFile.insets = new Insets(0, 0, 0, 5);
        gbc_lblSourceFile.anchor = GridBagConstraints.EAST;
        gbc_lblSourceFile.gridx = 0;
        gbc_lblSourceFile.gridy = 0;
        topPanel.add(lblSourceFile, gbc_lblSourceFile);

        inputFileField = new JTextField();
        inputFileField.setText("D:\\Stephane\\Desktop\\tmp\\md music\\opt vgm");
        GridBagConstraints gbc_inputFileField = new GridBagConstraints();
        gbc_inputFileField.insets = new Insets(0, 0, 0, 5);
        gbc_inputFileField.fill = GridBagConstraints.BOTH;
        gbc_inputFileField.gridx = 1;
        gbc_inputFileField.gridy = 0;
        topPanel.add(inputFileField, gbc_inputFileField);
        inputFileField.setColumns(10);

        selectInputButton = new JButton("...");
        selectInputButton.setIconTextGap(0);
        selectInputButton.setMargin(new Insets(2, 2, 2, 2));
        GridBagConstraints gbc_selectInputButton = new GridBagConstraints();
        gbc_selectInputButton.fill = GridBagConstraints.VERTICAL;
        gbc_selectInputButton.insets = new Insets(0, 0, 0, 5);
        gbc_selectInputButton.gridx = 2;
        gbc_selectInputButton.gridy = 0;
        topPanel.add(selectInputButton, gbc_selectInputButton);

        lblOutputFile = new JLabel("Output file");
        GridBagConstraints gbc_lblOutputFile = new GridBagConstraints();
        gbc_lblOutputFile.fill = GridBagConstraints.VERTICAL;
        gbc_lblOutputFile.insets = new Insets(0, 0, 0, 5);
        gbc_lblOutputFile.anchor = GridBagConstraints.EAST;
        gbc_lblOutputFile.gridx = 4;
        gbc_lblOutputFile.gridy = 0;
        topPanel.add(lblOutputFile, gbc_lblOutputFile);

        outputFileField = new JTextField();
        GridBagConstraints gbc_outputFileField = new GridBagConstraints();
        gbc_outputFileField.insets = new Insets(0, 0, 0, 5);
        gbc_outputFileField.fill = GridBagConstraints.BOTH;
        gbc_outputFileField.gridx = 5;
        gbc_outputFileField.gridy = 0;
        topPanel.add(outputFileField, gbc_outputFileField);
        outputFileField.setColumns(10);

        selectOutputButton = new JButton("...");
        selectOutputButton.setMargin(new Insets(2, 2, 2, 2));
        selectOutputButton.setIconTextGap(0);
        GridBagConstraints gbc_selectOutputButton = new GridBagConstraints();
        gbc_selectOutputButton.insets = new Insets(0, 0, 0, 5);
        gbc_selectOutputButton.fill = GridBagConstraints.VERTICAL;
        gbc_selectOutputButton.gridx = 6;
        gbc_selectOutputButton.gridy = 0;
        topPanel.add(selectOutputButton, gbc_selectOutputButton);

        syncButton = new JToggleButton("Sync");
        GridBagConstraints gbc_syncButton = new GridBagConstraints();
        gbc_syncButton.gridx = 7;
        gbc_syncButton.gridy = 0;
        topPanel.add(syncButton, gbc_syncButton);

        JPanel bottomPanel = new JPanel();
        add(bottomPanel, BorderLayout.SOUTH);
        GridBagLayout gbl_bottomPanel = new GridBagLayout();
        gbl_bottomPanel.columnWidths = new int[] {0, 0, 0, 0, 20, 0, 50, 50, 0};
        gbl_bottomPanel.rowHeights = new int[] {23, 0};
        gbl_bottomPanel.columnWeights = new double[] {0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_bottomPanel.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        bottomPanel.setLayout(gbl_bottomPanel);

        btnOptimizeVGM = new JButton("Optimize VGM");
        GridBagConstraints gbc_btnOptimizeVGM = new GridBagConstraints();
        gbc_btnOptimizeVGM.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnOptimizeVGM.anchor = GridBagConstraints.NORTH;
        gbc_btnOptimizeVGM.insets = new Insets(0, 0, 0, 5);
        gbc_btnOptimizeVGM.gridx = 0;
        gbc_btnOptimizeVGM.gridy = 0;
        bottomPanel.add(btnOptimizeVGM, gbc_btnOptimizeVGM);

        btnConvertXGM = new JButton("VGM to XGM");
        GridBagConstraints gbc_btnConvertXGM = new GridBagConstraints();
        gbc_btnConvertXGM.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnConvertXGM.anchor = GridBagConstraints.NORTH;
        gbc_btnConvertXGM.insets = new Insets(0, 0, 0, 5);
        gbc_btnConvertXGM.gridx = 1;
        gbc_btnConvertXGM.gridy = 0;
        bottomPanel.add(btnConvertXGM, gbc_btnConvertXGM);

        btnConvertXGC = new JButton("XGM to XGC");
        GridBagConstraints gbc_btnConvertXGC = new GridBagConstraints();
        gbc_btnConvertXGC.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnConvertXGC.anchor = GridBagConstraints.NORTH;
        gbc_btnConvertXGC.insets = new Insets(0, 0, 0, 5);
        gbc_btnConvertXGC.gridx = 2;
        gbc_btnConvertXGC.gridy = 0;
        bottomPanel.add(btnConvertXGC, gbc_btnConvertXGC);

        btnXgmToVgm = new JButton("XGM to VGM");
        GridBagConstraints gbc_btnXgmToVgm = new GridBagConstraints();
        gbc_btnXgmToVgm.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnXgmToVgm.anchor = GridBagConstraints.NORTH;
        gbc_btnXgmToVgm.insets = new Insets(0, 0, 0, 5);
        gbc_btnXgmToVgm.gridx = 3;
        gbc_btnXgmToVgm.gridy = 0;
        bottomPanel.add(btnXgmToVgm, gbc_btnXgmToVgm);

        Component horizontalGlue = Box.createHorizontalGlue();
        horizontalGlue.setPreferredSize(new Dimension(100, 0));
        GridBagConstraints gbc_horizontalGlue = new GridBagConstraints();
        gbc_horizontalGlue.fill = GridBagConstraints.VERTICAL;
        gbc_horizontalGlue.insets = new Insets(0, 0, 0, 5);
        gbc_horizontalGlue.gridx = 4;
        gbc_horizontalGlue.gridy = 0;
        bottomPanel.add(horizontalGlue, gbc_horizontalGlue);

        btnGoTo = new JButton("Go to");
        GridBagConstraints gbc_btnGoTo = new GridBagConstraints();
        gbc_btnGoTo.fill = GridBagConstraints.HORIZONTAL;
        gbc_btnGoTo.anchor = GridBagConstraints.NORTH;
        gbc_btnGoTo.insets = new Insets(0, 0, 0, 5);
        gbc_btnGoTo.gridx = 5;
        gbc_btnGoTo.gridy = 0;
        bottomPanel.add(btnGoTo, gbc_btnGoTo);

        timeField = new JTextField();
        timeField.setText("0");
        GridBagConstraints gbc_timeField = new GridBagConstraints();
        gbc_timeField.fill = GridBagConstraints.HORIZONTAL;
        gbc_timeField.insets = new Insets(0, 0, 0, 5);
        gbc_timeField.gridx = 6;
        gbc_timeField.gridy = 0;
        bottomPanel.add(timeField, gbc_timeField);
        timeField.setColumns(5);

        timeLabel = new JTextField("0");
        timeLabel.setEditable(false);
        timeLabel.setColumns(5);
        GridBagConstraints gbc_timeLabel = new GridBagConstraints();
        gbc_timeLabel.fill = GridBagConstraints.HORIZONTAL;
        gbc_timeLabel.gridx = 7;
        gbc_timeLabel.gridy = 0;
        bottomPanel.add(timeLabel, gbc_timeLabel);
    }

    public String getInputFilename()
    {
        return inputFileField.getText();
    }

    public String getOutputFilename()
    {
        return outputFileField.getText();
    }

    public boolean isSync()
    {
        return syncButton.isSelected();
    }

    @Override
    public void actionPerformed(ActionEvent e)
    {
        final Object source = e.getSource();

        try
        {
            if (source == btnOptimizeVGM)
            {
                tabPaneTop.getVgmOptTable().optimize(tabPaneTop.getVgmTable().vgm);
                tabPaneBottom.getVgmOptTable().optimize(tabPaneBottom.getVgmTable().vgm);
            }
            else if (source == btnConvertXGM)
            {
                tabPaneTop.getXgmTable().convert(tabPaneTop.getVgmOptTable().vgm);
                tabPaneBottom.getXgmTable().convert(tabPaneBottom.getVgmOptTable().vgm);
            }
            else if (source == btnConvertXGC)
            {
                tabPaneTop.getXgcTable().convert(tabPaneTop.getVgmOptTable().vgm);
                tabPaneBottom.getXgcTable().convert(tabPaneBottom.getVgmOptTable().vgm);
            }
            else if (source == btnXgmToVgm)
            {
                // only convert bottom table to allow comparison
                // tabPaneTop.getVgmOptTable().convert(tabPaneTop.getXgmTable().xgm);
                tabPaneBottom.getVgmOptTable().convert(tabPaneBottom.getXgmTable().xgm);
            }
            else if (source == btnGoTo)
            {
                tabPaneTop.syncTable(null, Integer.parseInt(timeField.getText()));
                tabPaneBottom.syncTable(null, Integer.parseInt(timeField.getText()));
            }
            else if (source == selectInputButton)
            {
                JFileChooser f = new JFileChooser();
                f.setFileSelectionMode(JFileChooser.FILES_ONLY);
                f.setMultiSelectionEnabled(false);
                f.setSelectedFile(new File(inputFileField.getText()));

                if (f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION)
                    inputFileField.setText(f.getSelectedFile().getAbsolutePath());
            }
            else if (source == selectOutputButton)
            {
                JFileChooser f = new JFileChooser();
                f.setFileSelectionMode(JFileChooser.FILES_ONLY);
                f.setMultiSelectionEnabled(false);
                f.setSelectedFile(new File(outputFileField.getText()));

                if (f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION)
                    outputFileField.setText(f.getSelectedFile().getAbsolutePath());
            }
        }
        catch (IOException e1)
        {
            e1.printStackTrace();
        }
    }

    private boolean isLeftTable(CommandsTable table)
    {
        return (table == tabPaneTop.getVgmTable()) || (table == tabPaneTop.getVgmOptTable())
                || (table == tabPaneTop.getXgmTable()) || (table == tabPaneTop.getXgcTable());
    }

    @Override
    public void movedTo(CommandsTable source, int vgmTime)
    {
        if (isSync())
        {
            if (isLeftTable(source))
                tabPaneBottom.syncTable(source, vgmTime);
            else
                tabPaneTop.syncTable(source, vgmTime);
        }
    }

    @Override
    public void selected(CommandsTable source, int vgmTime)
    {
        if (isSync())
        {
            if (isLeftTable(source))
                tabPaneBottom.syncTable(source, vgmTime);
            else
                tabPaneTop.syncTable(source, vgmTime);
        }
    }
}
