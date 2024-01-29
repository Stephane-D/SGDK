package sgdk.xgm2tool.gui;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;

import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.SwingConstants;

public class CustomTabPane extends JTabbedPane
{
    private final MainGUI parent;

    private VGMTable vgmTable;
    private VGMOptTable vgmOptTable;
    private XGMTable xgmTable;
    private XGMTable xgcTable;
    private WaveformPanel mainWave;
    private WaveformPanel subWave;
    private JTable sampleTable;
    private JPanel sampleSouthPanel;
    private JLabel lblSelectSampleSource;
    private JRadioButton rdbtnVgm;
    private JRadioButton rdbtnVgmOpt;
    private JRadioButton rdbtnXgm;
    private JRadioButton rdbtnXgc;

    public CustomTabPane(MainGUI parent)
    {
        super(SwingConstants.TOP);

        this.parent = parent;

        initialize();

        vgmTable.addListener(parent);
        vgmOptTable.addListener(parent);
        xgmTable.addListener(parent);
        xgcTable.addListener(parent);
    }

    private void initialize()
    {
        vgmTable = new VGMTable();
        addTab("VGM", null, vgmTable, null);

        vgmOptTable = new VGMOptTable();
        addTab("Optimized VGM", null, vgmOptTable, null);

        xgmTable = new XGMTable(false);
        addTab("XGM", null, xgmTable, null);

        xgcTable = new XGMTable(true);
        addTab("XGC", null, xgcTable, null);

        JPanel samplePanel = new JPanel();
        addTab("Sample", null, samplePanel, null);
        samplePanel.setLayout(new BorderLayout(0, 0));

        JPanel sampleMainPanel = new JPanel();
        samplePanel.add(sampleMainPanel, BorderLayout.CENTER);
        sampleMainPanel.setLayout(new BorderLayout(0, 0));

        JSplitPane splitPane_1 = new JSplitPane();
        splitPane_1.setResizeWeight(0.5);
        sampleMainPanel.add(splitPane_1);

        JSplitPane splitPane = new JSplitPane();
        splitPane.setResizeWeight(0.5);
        splitPane.setOrientation(JSplitPane.VERTICAL_SPLIT);
        splitPane_1.setRightComponent(splitPane);

        mainWave = new WaveformPanel();
        splitPane.setLeftComponent(mainWave);

        subWave = new WaveformPanel();
        splitPane.setRightComponent(subWave);

        JScrollPane scrollPane = new JScrollPane();
        splitPane_1.setLeftComponent(scrollPane);

        sampleTable = new JTable();
        scrollPane.setViewportView(sampleTable);

        sampleSouthPanel = new JPanel();
        samplePanel.add(sampleSouthPanel, BorderLayout.SOUTH);
        GridBagLayout gbl_sampleSouthPanel = new GridBagLayout();
        gbl_sampleSouthPanel.columnWidths = new int[] {0, 0, 0, 0, 0, 0};
        gbl_sampleSouthPanel.rowHeights = new int[] {0, 0};
        gbl_sampleSouthPanel.columnWeights = new double[] {0.0, 0.0, 0.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_sampleSouthPanel.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        sampleSouthPanel.setLayout(gbl_sampleSouthPanel);

        lblSelectSampleSource = new JLabel("Select sample source :");
        GridBagConstraints gbc_lblSelectSampleSource = new GridBagConstraints();
        gbc_lblSelectSampleSource.insets = new Insets(0, 0, 0, 5);
        gbc_lblSelectSampleSource.gridx = 0;
        gbc_lblSelectSampleSource.gridy = 0;
        sampleSouthPanel.add(lblSelectSampleSource, gbc_lblSelectSampleSource);

        rdbtnVgm = new JRadioButton("VGM");
        GridBagConstraints gbc_rdbtnVgm = new GridBagConstraints();
        gbc_rdbtnVgm.insets = new Insets(0, 0, 0, 5);
        gbc_rdbtnVgm.gridx = 1;
        gbc_rdbtnVgm.gridy = 0;
        sampleSouthPanel.add(rdbtnVgm, gbc_rdbtnVgm);

        rdbtnVgmOpt = new JRadioButton("VGM Opt");
        GridBagConstraints gbc_rdbtnVgmOpt = new GridBagConstraints();
        gbc_rdbtnVgmOpt.insets = new Insets(0, 0, 0, 5);
        gbc_rdbtnVgmOpt.gridx = 2;
        gbc_rdbtnVgmOpt.gridy = 0;
        sampleSouthPanel.add(rdbtnVgmOpt, gbc_rdbtnVgmOpt);

        rdbtnXgm = new JRadioButton("XGM");
        GridBagConstraints gbc_rdbtnXgm = new GridBagConstraints();
        gbc_rdbtnXgm.insets = new Insets(0, 0, 0, 5);
        gbc_rdbtnXgm.gridx = 3;
        gbc_rdbtnXgm.gridy = 0;
        sampleSouthPanel.add(rdbtnXgm, gbc_rdbtnXgm);

        rdbtnXgc = new JRadioButton("XGC");
        GridBagConstraints gbc_rdbtnXgc = new GridBagConstraints();
        gbc_rdbtnXgc.gridx = 4;
        gbc_rdbtnXgc.gridy = 0;
        sampleSouthPanel.add(rdbtnXgc, gbc_rdbtnXgc);
    }

    public VGMTable getVgmTable()
    {
        return vgmTable;
    }

    public VGMOptTable getVgmOptTable()
    {
        return vgmOptTable;
    }

    public XGMTable getXgmTable()
    {
        return xgmTable;
    }

    public XGMTable getXgcTable()
    {
        return xgcTable;
    }

    public void syncTable(CommandsTable source, int vgmTime)
    {
        if (source != vgmTable)
            vgmTable.scrollTo(vgmTime);
        if (source != vgmOptTable)
            vgmOptTable.scrollTo(vgmTime);
        if (source != xgmTable)
            xgmTable.scrollTo(vgmTime);
        if (source != xgcTable)
            xgcTable.scrollTo(vgmTime);
    }
}
