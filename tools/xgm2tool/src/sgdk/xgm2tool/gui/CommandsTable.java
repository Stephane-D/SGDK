package sgdk.xgm2tool.gui;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JViewport;
import javax.swing.ScrollPaneConstants;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.format.Command;
import sgdk.xgm2tool.format.VGMCommand;
import sgdk.xgm2tool.format.XGMFMCommand;
import sgdk.xgm2tool.format.XGMPSGCommand;
import sgdk.xgm2tool.tool.Util;

public abstract class CommandsTable extends JPanel implements ActionListener, ListSelectionListener, ChangeListener
{
    /**
     * CommandsTable listener interface
     */
    public interface CommandsTableListener
    {
        public void movedTo(CommandsTable source, int vgmTime);

        public void selected(CommandsTable source, int vgmTime);
    }

    protected JLabel titleLabel;
    protected JButton loadButton;
    protected JButton saveButton;
    protected JLabel commandNumberField;
    protected JLabel lengthField;
    protected JLabel sizeField;
    protected JTable table;

    // internal
    protected AbstractTableModel tableModel;
    protected List commands;
    protected boolean scrolling;

    // global setting
    protected final List<CommandsTableListener> listeners;
    public boolean pal;

    /**
     * Create the panel.
     */
    public CommandsTable()
    {
        super();

        initialize();

        listeners = new ArrayList<>();
        pal = false;
        loadButton.addActionListener(this);
        saveButton.addActionListener(this);

        tableModel = new AbstractTableModel()
        {
            @Override
            public Object getValueAt(int rowIndex, int columnIndex)
            {
                if (commands == null)
                    return "";

                final Command com = (Command) commands.get(rowIndex);
                final int ch = com.getChannel();

                switch (columnIndex)
                {
                    case 0: // time
                        return Util.formatVGMTime(com.time);
                    case 1: // offset
                        if (com instanceof XGMFMCommand)
                            return String.format("%06X (FM)", Integer.valueOf(com.getOriginOffset()));
                        else if (com instanceof XGMPSGCommand)
                            return String.format("%06X (PSG)", Integer.valueOf(com.getOriginOffset()));
                        else
                            return String.format("%06X", Integer.valueOf(com.getOriginOffset()));
                    case 2: // size
                        return StringUtil.toString(com.size);
                    case 3: // data
                        return com.toHexaString();
                    default:
                        if (ch == (columnIndex - 5))
                            return com;

                        return "";
                }
            }

            @Override
            public Class<?> getColumnClass(int columnIndex)
            {
                switch (columnIndex)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        return String.class;
                    default:
                        return Command.class;
                }
            }

            @Override
            public String getColumnName(int column)
            {
                switch (column)
                {
                    case 0:
                        return "Time";
                    case 1:
                        return "Offset";
                    case 2:
                        return "Size";
                    case 3:
                        return "Data";
                    case 4:
                        return "Com";
                    case 5:
                        return "FM0 / PSG0";
                    case 6:
                        return "FM1 / PSG1";
                    case 7:
                        return "FM2 / PSG2";
                    case 8:
                        return "FM3 / PSG3";
                    case 9:
                        return "FM4";
                    case 10:
                        return "FM5";
                    default:
                        return "";
                }
            }

            @Override
            public int getRowCount()
            {
                if (commands == null)
                    return 0;

                return commands.size();
            }

            @Override
            public int getColumnCount()
            {
                return 11;
            }
        };

        table.setModel(tableModel);
        // set renderer for Command data
        table.setDefaultRenderer(Command.class, new DefaultTableCellRenderer()
        {
            @Override
            public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected,
                    boolean hasFocus, int row, int column)
            {
                super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

                if (commands == null)
                    return this;

                setBackground(Color.white);

                final Command com = (Command) commands.get(row);

                if (com instanceof VGMCommand)
                {
                    final VGMCommand vgmCom = (VGMCommand) com;

                    if (vgmCom.isYM2612KeyWrite())
                        setBackground(new Color(0xA0A0FF));
                    else if (vgmCom.isYM2612Write())
                        setBackground(new Color(0xA0FFA0));
                    else if (vgmCom.isPSGWrite())
                    {                        
                        if (vgmCom.isPSGEnvWrite())
                            setBackground(new Color(0xFFA080));
                        else if (vgmCom.isPSGToneWrite())
                            setBackground(new Color(0xFF80A0));
                        else
                            setBackground(new Color(0xFFA0A0));   
                    }
                    else if (vgmCom.isSeek())
                        setBackground(new Color(0xFFFF00));
                    else if (vgmCom.isPCM())
                        setBackground(new Color(0xFFFF00));
                    else if (vgmCom.isStream())
                        setBackground(new Color(0xFFFF00));
                    else if (vgmCom.isWait())
                        setBackground(new Color(0xC0C0C0));
                    else
                        setBackground(new Color(0xFFFFFF));
                }
                else if (com instanceof XGMFMCommand)
                {
                    final XGMFMCommand fmCom = (XGMFMCommand) com;

                    if (fmCom.isWait(true))
                        setBackground(new Color(0xC0C0C0));
                    else if (fmCom.isYMTLDeltaWait())
                        setBackground(new Color(0xB0C0C0));
                    else if (fmCom.isYMSetTL() || fmCom.isYMTLDelta())
                        setBackground(new Color(0xA0C0C0));
                    else if (fmCom.isYMKeyWrite())
                        setBackground(new Color(0xA0A0FF));
                    else if (fmCom.isYMFreqWriteWait() || fmCom.isYMFreqDeltaWriteWait())
                        setBackground(new Color(0xB0C0D8));
                    else if (fmCom.isYMFreqWrite() || fmCom.isYMFreqDeltaWrite())
                        setBackground(new Color(0xA0C0FF));
                    else if (fmCom.isYMLoadInst())
                        setBackground(new Color(0xA0FFA0));
                    else if (fmCom.isYMWrite())
                        setBackground(new Color(0xA0FFA0));
                    else if (fmCom.isPCM())
                        setBackground(new Color(0xFFFF00));
                    else
                        setBackground(new Color(0xFFFFFF));
                }
                else if (com instanceof XGMPSGCommand)
                {
                    final XGMPSGCommand psgCom = (XGMPSGCommand) com;

                    if (psgCom.isDummy())
                        setBackground(new Color(0xFFFFFF));
                    else if (psgCom.isWaitShort() || psgCom.isWaitLong())
                        setBackground(new Color(0xD0B0B0));
                    else if (psgCom.isEnvDeltaWait())
                        setBackground(new Color(0xE0A890));
                    else if (psgCom.isEnv() || psgCom.isEnvDeltaNoWait())
                        setBackground(new Color(0xFFA080));
                    else if (psgCom.isFreqWait() || psgCom.isFreqDeltaWait())
                        setBackground(new Color(0xE090A8));
                    else if (psgCom.isFreq() || psgCom.isFreqLow() || psgCom.isFreqDeltaNoWait())
                        setBackground(new Color(0xFF80A0));
                    else
                        setBackground(new Color(0xFF8080));
                }

                setToolTipText(value.toString());

                return this;
            }
        });
        // set renderer for String data
        table.setDefaultRenderer(String.class, new DefaultTableCellRenderer()
        {
            @Override
            public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected,
                    boolean hasFocus, int row, int column)
            {
                super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

                if (commands == null)
                    return this;

                final Command com = (Command) commands.get(row);

                // even frame ?
                if ((com.getFrame(pal) & 1) == 0)
                    setBackground(Color.lightGray);
                // odd frame ?
                else
                    setBackground(Color.white);

                setToolTipText(value.toString());

                return this;
            }
        });

        table.getSelectionModel().addListSelectionListener(this);
        table.setCellSelectionEnabled(false);
        table.setRowSelectionAllowed(true);

        scrolling = false;
    }

    private void initialize()
    {
        setLayout(new BorderLayout(0, 0));

        JPanel topPanel = new JPanel();
        add(topPanel, BorderLayout.NORTH);
        GridBagLayout gbl_topPanel = new GridBagLayout();
        gbl_topPanel.columnWidths = new int[] {50, 0, 0, 0};
        gbl_topPanel.rowHeights = new int[] {0, 0};
        gbl_topPanel.columnWeights = new double[] {1.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_topPanel.rowWeights = new double[] {1.0, Double.MIN_VALUE};
        topPanel.setLayout(gbl_topPanel);

        titleLabel = new JLabel("Title");
        titleLabel.setFont(new Font("Tahoma", Font.BOLD, 11));
        GridBagConstraints gbc_titleLabel = new GridBagConstraints();
        gbc_titleLabel.insets = new Insets(0, 0, 0, 5);
        gbc_titleLabel.gridx = 0;
        gbc_titleLabel.gridy = 0;
        topPanel.add(titleLabel, gbc_titleLabel);

        loadButton = new JButton("Load");
        GridBagConstraints gbc_actionButton = new GridBagConstraints();
        gbc_actionButton.fill = GridBagConstraints.BOTH;
        gbc_actionButton.insets = new Insets(0, 0, 0, 5);
        gbc_actionButton.gridx = 1;
        gbc_actionButton.gridy = 0;
        topPanel.add(loadButton, gbc_actionButton);

        saveButton = new JButton("Save");
        GridBagConstraints gbc_saveButton = new GridBagConstraints();
        gbc_saveButton.fill = GridBagConstraints.BOTH;
        gbc_saveButton.gridx = 2;
        gbc_saveButton.gridy = 0;
        topPanel.add(saveButton, gbc_saveButton);

        JPanel mainPanel = new JPanel();
        add(mainPanel);
        mainPanel.setLayout(new BorderLayout(0, 0));

        JScrollPane scrollPane = new JScrollPane();
        scrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
        mainPanel.add(scrollPane, BorderLayout.CENTER);

        table = new JTable();
        scrollPane.setViewportView(table);
        scrollPane.getViewport().addChangeListener(this);

        JPanel bottomPanel = new JPanel();
        add(bottomPanel, BorderLayout.SOUTH);
        GridBagLayout gbl_bottomPanel = new GridBagLayout();
        gbl_bottomPanel.columnWidths = new int[] {60, 16, 80, 16, 50, 0};
        gbl_bottomPanel.rowHeights = new int[] {0, 0};
        gbl_bottomPanel.columnWeights = new double[] {0.0, 0.0, 0.0, 0.0, 0.0, Double.MIN_VALUE};
        gbl_bottomPanel.rowWeights = new double[] {0.0, Double.MIN_VALUE};
        bottomPanel.setLayout(gbl_bottomPanel);

        commandNumberField = new JLabel("0000");
        GridBagConstraints gbc_commandNumberField = new GridBagConstraints();
        gbc_commandNumberField.anchor = GridBagConstraints.EAST;
        gbc_commandNumberField.insets = new Insets(0, 0, 0, 5);
        gbc_commandNumberField.gridx = 0;
        gbc_commandNumberField.gridy = 0;
        bottomPanel.add(commandNumberField, gbc_commandNumberField);

        JLabel label = new JLabel("-");
        GridBagConstraints gbc_label = new GridBagConstraints();
        gbc_label.insets = new Insets(0, 0, 0, 5);
        gbc_label.gridx = 1;
        gbc_label.gridy = 0;
        bottomPanel.add(label, gbc_label);

        lengthField = new JLabel("00:00:00");
        GridBagConstraints gbc_lengthField = new GridBagConstraints();
        gbc_lengthField.insets = new Insets(0, 0, 0, 5);
        gbc_lengthField.anchor = GridBagConstraints.EAST;
        gbc_lengthField.gridx = 2;
        gbc_lengthField.gridy = 0;
        bottomPanel.add(lengthField, gbc_lengthField);

        JLabel lblNewLabel = new JLabel("-");
        GridBagConstraints gbc_lblNewLabel = new GridBagConstraints();
        gbc_lblNewLabel.insets = new Insets(0, 0, 0, 5);
        gbc_lblNewLabel.gridx = 3;
        gbc_lblNewLabel.gridy = 0;
        bottomPanel.add(lblNewLabel, gbc_lblNewLabel);

        sizeField = new JLabel("0 Kb");
        GridBagConstraints gbc_sizeField = new GridBagConstraints();
        gbc_sizeField.anchor = GridBagConstraints.EAST;
        gbc_sizeField.gridx = 4;
        gbc_sizeField.gridy = 0;
        bottomPanel.add(sizeField, gbc_sizeField);
    }

    public void addListener(CommandsTableListener listener)
    {
        listeners.add(listener);
    }

    public void removeListener(CommandsTableListener listener)
    {
        listeners.remove(listener);
    }

    protected void fireMovedTo(int vgmTime)
    {
        for (CommandsTableListener listener : listeners)
            listener.movedTo(this, vgmTime);
    };

    protected void fireSelected(int vgmTime)
    {
        for (CommandsTableListener listener : listeners)
            listener.selected(this, vgmTime);
    }

    protected void movedTo(int vgmTime)
    {
        fireMovedTo(vgmTime);
    }

    protected void selected(int vgmTime)
    {
        fireSelected(vgmTime);
    }

    public void scrollTo(int vgmTime)
    {
        if (scrolling)
            return;
        if (commands == null)
            return;

        scrolling = true;
        try
        {
            final int ind = Command.getCommandIndexAtTime(commands, vgmTime);
            if (ind != -1)
                table.scrollRectToVisible(new Rectangle(table.getCellRect(ind, 0, true)));
        }
        finally
        {
            scrolling = false;
        }
    }

    @Override
    public void stateChanged(ChangeEvent e)
    {
        if (scrolling)
            return;
        if (commands == null)
            return;

        final Object source = e.getSource();

        // scroll changed ?
        scrolling = true;
        try
        {
            if (source instanceof JViewport)
            {
                // find the first visible command
                final Rectangle viewRect = ((JViewport) source).getViewRect();
                final int row = table.rowAtPoint(new Point(0, viewRect.y));
                // we moved to that point
                if ((row >= 0) && (row < commands.size()))
                    movedTo(((Command) commands.get(row)).time);
            }
        }
        finally
        {
            scrolling = false;
        }
    }

    @Override
    public void valueChanged(ListSelectionEvent e)
    {
        if (scrolling)
            return;
        if (commands == null)
            return;

        // selection changed ?
        scrolling = true;
        try
        {
            final Object source = e.getSource();

            if (source == table.getSelectionModel())
            {
                final int row = table.getSelectedRow();

                // we moved to that point
                if ((row >= 0) && (row < commands.size()))
                    selected(((Command) commands.get(row)).time);
            }
        }
        finally
        {
            scrolling = false;
        }
    }

    @Override
    public void actionPerformed(ActionEvent e)
    {
        try
        {
            if (e.getSource() == loadButton)
                doLoadAction();
            else if (e.getSource() == saveButton)
                doSaveAction();
        }
        catch (IOException e1)
        {
            e1.printStackTrace();
        }
    }

    protected abstract void doLoadAction() throws IOException;

    protected abstract void doSaveAction() throws IOException;
}
