package main;

import java.io.File;

public class VGMFile
{
    public static enum VGMTiming
    {
        AUTO, NTSC, PAL
    };

    File file;
    byte[] xgc;
    long duration;
    boolean disablePCMIgnore;
    boolean disablePCMRateFix;
    boolean disableDelayedKeyOFF;
    VGMTiming timing;

    public VGMFile()
    {
        super();

        file = null;
        xgc = null;
        duration = 0L;
        disablePCMIgnore = false;
        disablePCMRateFix = false;
        disableDelayedKeyOFF = false;
        timing = VGMTiming.AUTO;
    }

    public VGMFile(File file)
    {
        this();
        
        this.file = file;
    }
}
