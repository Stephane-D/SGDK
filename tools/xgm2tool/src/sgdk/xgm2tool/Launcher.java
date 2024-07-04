package sgdk.xgm2tool;

import java.awt.EventQueue;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JFrame;

import sgdk.tool.FileUtil;
import sgdk.tool.StringUtil;
import sgdk.xgm2tool.format.VGM;
import sgdk.xgm2tool.format.XGM;
import sgdk.xgm2tool.format.XGMMulti;
import sgdk.xgm2tool.gui.MainGUI;
import sgdk.xgm2tool.tool.Util;

public class Launcher extends JFrame
{
    final static String VERSION = "1.04";

    final static int SYSTEM_AUTO = -1;
    final static int SYSTEM_NTSC = 0;
    final static int SYSTEM_PAL = 1;

    public static int sys = SYSTEM_AUTO;
    public static boolean silent = false;
    public static boolean verbose = false;
    public static boolean sampleRateFix = true;
    public static boolean sampleIgnore = true;
    public static boolean sampleAdvancedCompare = false;
    public static boolean delayKeyOff = false;

    /**
     * Launch the application.
     */
    public static void main(String[] args)
    {
        // GUI usage
        if (args.length == 0)
        {
            EventQueue.invokeLater(new Runnable()
            {
                public void run()
                {
                    try
                    {
                        Launcher frame = new Launcher();
                        frame.setVisible(true);
                    }
                    catch (Exception e)
                    {
                        e.printStackTrace();
                    }
                }
            });
        }
        // command line usage
        else
        {
            // incorrect number of argument
            if (args.length < 2)
            {
                showUsage();
                System.exit(2);
            }
            else
            {
                try
                {
                    final int result = execute(args);
                    System.exit(result);
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                    System.exit(1);
                }
            }
        }
    }

    private static int execute(String[] args) throws IOException
    {
        // out/sor2*.vgm out/sor2.xgc
        
        final List<String> files = new ArrayList<>();

        sys = SYSTEM_AUTO;
        silent = false;
        verbose = false;
        sampleIgnore = true;
        sampleRateFix = true;
        delayKeyOff = true;

        // options
        for (int i = 0; i < args.length; i++)
        {
            final String arg = args[i];

            if (StringUtil.equals(arg, "-s"))
            {
                silent = true;
                verbose = false;
            }
            else if (StringUtil.equals(arg, "-v"))
            {
                verbose = true;
                silent = false;
            }
            else if (StringUtil.equals(arg, "-di"))
                sampleIgnore = false;
            else if (StringUtil.equals(arg, "-dr"))
                sampleRateFix = false;
            else if (StringUtil.equals(arg, "-dd"))
                delayKeyOff = false;
            else if (StringUtil.equals(arg, "-ac"))
                sampleAdvancedCompare = true;
            else if (StringUtil.equals(arg, "-n"))
                sys = SYSTEM_NTSC;
            else if (StringUtil.equals(arg, "-p"))
                sys = SYSTEM_PAL;
            else if (arg.startsWith("-"))
                System.out.println("Warning: option '" + arg + "' not recognized (ignored)");
            else
                files.add(arg);
        }

        // silent mode has priority
        if (silent)
            verbose = false;
        
        final int numFile = files.size();
        final String outFile = files.get(files.size() - 1);
        final String outExt = FileUtil.getFileExtension(outFile, false).toUpperCase();

        if (numFile > 2)
        {
            // multi tracks --> VGM to XGM/XGC
            for (int i = 0; i < numFile - 1; i++)
            {
                final String file = files.get(i);

                if (!FileUtil.exists(file))
                {
                    System.out.println("Error: the source file '" + file + "' could not be find");
                    System.exit(2);
                }

                final String ext = FileUtil.getFileExtension(file, false).toUpperCase();

                // check that we have VGM files there
                if (!StringUtil.isEmpty(ext) & !StringUtil.equals(ext, "VGM"))
                {
                    System.out.println("Error: expected VGM file as input for multi tracks conversion (found " + ext + ")");
                    System.exit(2);
                }
            }

            // XGM output --> get byte array
            if (!StringUtil.equals(outExt, "XGM") && !StringUtil.equals(outExt, "XGC"))
            {
                System.out.println("Error: expected XGM or XGC file as output for multi tracks conversion (found " + outExt + ")");
                System.exit(2);
            }

            final List<XGM> xgms = new ArrayList<>();

            for (int i = 0; i < numFile - 1; i++)
                xgms.add(new XGM(loadVGM(files.get(i)), StringUtil.equals(outExt, "XGC")));

            final byte[] outData = new XGMMulti(xgms, StringUtil.equals(outExt, "XGC")).asByteArray();

            // write to file
            Util.writeBinaryFile(outData, outFile);

            // success
            System.exit(0);
        }

        final String inFile = files.get(0);
        if (!FileUtil.exists(inFile))
        {
            System.out.println("Error: the source file '" + inFile + "' could not be find");
            System.exit(2);
        }

        final String inExt = FileUtil.getFileExtension(inFile, false).toUpperCase();

        // input file is VGM or empty (assumed as VGM)
        if (StringUtil.equals(inExt, "VGM") || inExt.isEmpty())
        {
            final VGM vgm = loadVGM(inFile);

            // VGM output --> directly write to file
            if (StringUtil.equals(outExt, "VGM"))
                Util.writeBinaryFile(vgm.asByteArray(), outFile);
            else
            {
                // convert to XGM or XGC depending output extension
                final XGM xgm = new XGM(vgm, !StringUtil.equals(outExt, "XGM"));
                Util.writeBinaryFile(xgm.asByteArray(), outFile);
            }
        }
        // input file is XGM
        else if (StringUtil.equals(inExt, "XGM"))
        {
            // load XGM
            final XGM xgm = loadXGM(inFile);

            // VGM conversion
            if (StringUtil.equals(outExt, "VGM"))
            {
                // convert to VGM
                final VGM vgm = new VGM(xgm);
                // write to file
                Util.writeBinaryFile(vgm.asByteArray(), outFile);
            }
            // assume XGC conversion
            else
            {
                // convert to XGC (just set packed to true)
                xgm.packed = true;
                // write to file
                Util.writeBinaryFile(xgm.asByteArray(), outFile);
            }
        }
        // input file is XGC
        else if (StringUtil.equals(inExt, "XGC"))
        {
            // load XGC
            final XGM xgm = loadXGM(inFile);

            // VGM conversion
            if (StringUtil.equals(outExt, "VGM"))
            {
                // convert to VGM
                final VGM vgm = new VGM(xgm);
                // write to file
                Util.writeBinaryFile(vgm.asByteArray(), outFile);
            }
            // XGM conversion
            else if (StringUtil.equals(outExt, "XGM"))
            {
                // convert to XGM (just set packed to false)
                xgm.packed = false;
                // write to file
                Util.writeBinaryFile(xgm.asByteArray(), outFile);
            }
            else
            {
                System.out.println("Error: the output file '" + outFile + "' is incorrect (should be a VGM or XGM file)");
                System.exit(4);
            }
        }
        else
        {
            System.out.println("Error: the input file '" + inFile + "' is incorrect (should be a VGM, XGM or XGC file)");
            System.exit(4);
        }

        return 0;
    }

    private static VGM loadVGM(String file) throws IOException
    {
        final byte[] inData = Util.readBinaryFile(file);

        // load VGM
        if (sys == SYSTEM_NTSC)
            inData[0x24] = 60;
        else if (sys == SYSTEM_PAL)
            inData[0x24] = 50;

        // create with conversion
        return new VGM(inData, true);
    }

    private static XGM loadXGM(String file) throws IOException
    {
        final byte[] inData = Util.readBinaryFile(file);
        // create with conversion
        return new XGM(inData);
    }

    private static void showUsage()
    {
        System.out.println("XGM2Tool " + VERSION + " - Stephane Dallongeville - copyright 2024");
        System.out.println("");
        System.out.println("Usage: java -jar xgmtool.jar inputFile(s) outputFile <options>");
        System.out.println("");
        System.out.println("The action XGM2Tool performs is dependant from the input and output file extension.");
        System.out.println("It can do the following operations:");
        System.out.println(" - Optimize and reduce size of Sega Mega Drive VGM file");
        System.out.println("   Note that it won't work correctly on VGM file which require sub frame accurate timing.");
        System.out.println(" - Convert a Sega Mega Drive VGM file to XGM2/XGC2 file");
        System.out.println(" - Convert a list of Sega Mega Drive VGM files to a multi tracks XGM2/XGC2 file");
        System.out.println(" - Convert a XGM2 file to Sega Mega Drive VGM file");
        System.out.println(" - Compile a XGM2 file into a packed XGM2 (XGC2) ready to played by the Z80 XGM2 driver");
        System.out.println(" - Convert a XGC2 file (packed XGM2) to Sega Mega Drive VGM file");
        System.out.println(" - Convert a XGC2 file (packed XGM2) to XGM2 file");
        System.out.println("");
        System.out.println("Simple Sega Mega Drive VGM optimization:");
        System.out.println("  xgmtool input.vgm output.vgm");
        System.out.println("");
        System.out.println("Convert VGM to XGM2:");
        System.out.println("  xgmtool input.vgm output.xgm");
        System.out.println("Convert VGM to XGC2 (packed XGM2):");
        System.out.println("  xgmtool input.vgm output.xgc");
        System.out.println("");
        System.out.println("Convert a list of Sega Mega Drive VGM to XGM2:");
        System.out.println("  xgmtool input1.vgm input2.vgm input3.vgm ... output.xgm");
        System.out.println("  xgmtool *.vgm output.xgm");
        System.out.println("Convert a list of Sega Mega Drive VGM to XGC2 (packed XGM2):");
        System.out.println("  xgmtool input1.vgm input2.vgm input3.vgm ... output.xgc");
        System.out.println("  xgmtool *.vgm output.xgc");
        System.out.println("");
        System.out.println("Convert XGM2 to Sega Mega Drive VGM:");
        System.out.println("  xgmtool input.xgm output.vgm");
        System.out.println("Convert XGM2 to XGC2 (packed XGM2):");
        System.out.println("  xgmtool input.xgm output.xgc");
        System.out.println("");
        System.out.println("Convert XGC2 (packed XGM2) to Sega Mega Drive VGM:");
        System.out.println("  xgmtool input.xgc output.vgm");
        System.out.println("Convert XGC2 (packed XGM2) to XGM2:");
        System.out.println("  xgmtool input.xgc output.xgm");
        System.out.println("");
        System.out.println("Supported options:");
        System.out.println("-s\tenable silent mode (no message except error and warning).");
        System.out.println("-v\tenable verbose mode (give more info about conversion).");
        System.out.println("-n\tforce NTSC timing (only meaningful for VGM to XGM conversion).");
        System.out.println("-p\tforce PAL timing (only meaningful for VGM to XGM conversion).");
        System.out.println("-di\tdisable PCM sample auto ignore (it can help when PCM are not properly extracted).");
        System.out.println("-dr\tdisable PCM sample rate auto fix (it can help when PCM are not properly extracted).");
        System.out.println("-dd\tdisable delayed KEY OFF event when we have KEY ON/OFF in a single frame (it can fix incorrect instrument sound).");
        System.out.println("-ac\tenable fingerprint compare on PCM merging operation (use it to improve duplicate sample detection, can be too aggressive..).");
    }

    // easier access
    public static MainGUI mainGui = null;

    /**
     * Create the frame.
     */
    public Launcher()
    {
        super();

        setTitle("XGM Tool v" + VERSION);

        initialize();
    }

    private void initialize()
    {
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1280, 720);
        setLocationRelativeTo(null);

        mainGui = new MainGUI();
        setContentPane(mainGui);

        setVisible(true);
    }
}
