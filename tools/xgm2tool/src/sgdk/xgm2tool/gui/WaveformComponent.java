package sgdk.xgm2tool.gui;

import java.awt.Color;
import java.awt.Graphics;

import javax.swing.JPanel;

public class WaveformComponent extends JPanel
{
    final static double BORDER_X = 3;
    final static double BORDER_Y = 10;

    // 8 bit 44100 Hz sample
    protected byte[] wave;

    // offset on X range (in number of sample)
    protected int offsetX;
    // offset on Y range (intensity value)
    protected int offsetY;
    protected double scaleX;
    protected double scaleY;

    /**
     * Create the panel.
     */
    public WaveformComponent()
    {
        super();

        // default rate
        wave = new byte[] {0};
        offsetX = 0;
        offsetY = 0;
        scaleX = 1d;
        scaleY = 1d;
    }

    public int getOffsetX()
    {
        return offsetX;
    }

    public void setOffsetX(int value)
    {
        offsetX = value;
    }

    public int getOffsetY()
    {
        return offsetY;
    }

    public void setOffsetY(int value)
    {
        offsetY = value;
    }

    public double getScaleX()
    {
        return scaleX;
    }

    public void setScaleX(double value)
    {
        scaleX = value;
    }

    public double getScaleY()
    {
        return scaleY;
    }

    public void setScaleY(double value)
    {
        scaleY = value;
    }

    /**
     * Set waveform data (8bit signed)
     */
    public void setWaveformData(byte[] data)
    {
        if ((data == null) || (data.length == 0))
            wave = new byte[] {0};
        else
            wave = data;

        repaint();
    }

    @Override
    protected void paintComponent(Graphics g)
    {
        super.paintComponent(g);

        int w = getWidth();
        int h = getHeight();
        int borderX = (int) (w * BORDER_X);
        int borderY = (int) (h * BORDER_Y);

        w -= borderX * 2;
        h -= borderY * 2;

        // draw axes
        g.setColor(Color.black);
        g.drawLine(borderX, borderY, borderX, borderY + h);
        g.drawLine(borderX, borderY, borderX + w, borderY);

        // draw waveform

    }
}
