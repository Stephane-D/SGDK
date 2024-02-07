package sgdk.xgm2tool.gui;

import javax.swing.JPanel;
import java.awt.BorderLayout;
import javax.swing.JButton;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import javax.swing.JLabel;
import javax.swing.JTextPane;
import java.awt.Font;

public class WaveformPanel extends JPanel
{
    private JButton decreaseScaleXButton;
    private JButton increaseScaleXButton;
    private JButton decreaseScaleYButton;
    private JButton increaseScaleYButton;
    private JButton resetButton;
    public WaveformPanel()
    {
        super();
        setLayout(new BorderLayout(0, 0));
        
        JPanel leftPanel = new JPanel();
        add(leftPanel, BorderLayout.WEST);
        GridBagLayout gbl_leftPanel = new GridBagLayout();
        gbl_leftPanel.columnWidths = new int[]{36, 36, 0};
        gbl_leftPanel.rowHeights = new int[]{23, 0, 0, 0, 0, 0};
        gbl_leftPanel.columnWeights = new double[]{0.0, 0.0, Double.MIN_VALUE};
        gbl_leftPanel.rowWeights = new double[]{0.0, 0.0, 0.0, 0.0, 0.0, Double.MIN_VALUE};
        leftPanel.setLayout(gbl_leftPanel);
        
        JLabel lblHorizontal = new JLabel("Horizontal");
        GridBagConstraints gbc_lblHorizontal = new GridBagConstraints();
        gbc_lblHorizontal.gridwidth = 2;
        gbc_lblHorizontal.insets = new Insets(0, 0, 5, 0);
        gbc_lblHorizontal.gridx = 0;
        gbc_lblHorizontal.gridy = 0;
        leftPanel.add(lblHorizontal, gbc_lblHorizontal);
        
        decreaseScaleXButton = new JButton("-");
        decreaseScaleXButton.setMargin(new Insets(0, 6, 0, 2));
        GridBagConstraints gbc_decreaseScaleXButton = new GridBagConstraints();
        gbc_decreaseScaleXButton.fill = GridBagConstraints.BOTH;
        gbc_decreaseScaleXButton.insets = new Insets(0, 0, 5, 5);
        gbc_decreaseScaleXButton.gridx = 0;
        gbc_decreaseScaleXButton.gridy = 1;
        leftPanel.add(decreaseScaleXButton, gbc_decreaseScaleXButton);
        
        increaseScaleXButton = new JButton("+");
        increaseScaleXButton.setMargin(new Insets(0, 6, 0, 6));
        GridBagConstraints gbc_increaseScaleXButton = new GridBagConstraints();
        gbc_increaseScaleXButton.fill = GridBagConstraints.BOTH;
        gbc_increaseScaleXButton.insets = new Insets(0, 0, 5, 0);
        gbc_increaseScaleXButton.gridx = 1;
        gbc_increaseScaleXButton.gridy = 1;
        leftPanel.add(increaseScaleXButton, gbc_increaseScaleXButton);
        
        JLabel lblNewLabel = new JLabel("Vertical");
        GridBagConstraints gbc_lblNewLabel = new GridBagConstraints();
        gbc_lblNewLabel.gridwidth = 2;
        gbc_lblNewLabel.insets = new Insets(0, 0, 5, 0);
        gbc_lblNewLabel.gridx = 0;
        gbc_lblNewLabel.gridy = 2;
        leftPanel.add(lblNewLabel, gbc_lblNewLabel);
        
        decreaseScaleYButton = new JButton("-");
        decreaseScaleYButton.setMargin(new Insets(0, 6, 0, 6));
        GridBagConstraints gbc_decreaseScaleYButton = new GridBagConstraints();
        gbc_decreaseScaleYButton.fill = GridBagConstraints.BOTH;
        gbc_decreaseScaleYButton.insets = new Insets(0, 0, 5, 5);
        gbc_decreaseScaleYButton.gridx = 0;
        gbc_decreaseScaleYButton.gridy = 3;
        leftPanel.add(decreaseScaleYButton, gbc_decreaseScaleYButton);
        
        increaseScaleYButton = new JButton("+");
        increaseScaleYButton.setMargin(new Insets(0, 6, 0, 6));
        GridBagConstraints gbc_increaseScaleYButton = new GridBagConstraints();
        gbc_increaseScaleYButton.fill = GridBagConstraints.BOTH;
        gbc_increaseScaleYButton.insets = new Insets(0, 0, 5, 0);
        gbc_increaseScaleYButton.gridx = 1;
        gbc_increaseScaleYButton.gridy = 3;
        leftPanel.add(increaseScaleYButton, gbc_increaseScaleYButton);
        
        resetButton = new JButton("Reset");
        resetButton.setMargin(new Insets(0, 2, 0, 2));
        GridBagConstraints gbc_resetButton = new GridBagConstraints();
        gbc_resetButton.gridwidth = 2;
        gbc_resetButton.fill = GridBagConstraints.BOTH;
        gbc_resetButton.gridx = 0;
        gbc_resetButton.gridy = 4;
        leftPanel.add(resetButton, gbc_resetButton);
        
        WaveformComponent waveform = new WaveformComponent();
        add(waveform, BorderLayout.CENTER);

        initialize();
    }

    private void initialize()
    {

    }
}
