package sgdk.rescomp.processor;

import java.io.IOException;

import javax.sound.sampled.UnsupportedAudioFileException;

import sgdk.rescomp.Compiler;
import sgdk.rescomp.Processor;
import sgdk.rescomp.Resource;
import sgdk.rescomp.resource.Bin;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Compression;
import sgdk.rescomp.type.Basics.SoundDriver;
import sgdk.tool.FileUtil;
import sgdk.tool.SoundUtil;
import sgdk.tool.StringUtil;

public class WavProcessor implements Processor
{
    @Override
    public String getId()
    {
        return "WAV";
    }

    @Override
    public Resource execute(String[] fields) throws IOException
    {
        if (fields.length < 3)
        {
            System.out.println("Wrong WAV definition");
            System.out.println("WAV name \"file\" [driver [out_rate]]");
            System.out.println("  name      variable name");
            System.out.println("  file      path of the .wav file (will be converted to 8 bits signed PCM)");
            System.out.println("  driver    specify the Z80 driver we will use to play the WAV file:");
            System.out.println("              0 / PCM (default)");
            System.out.println("                Single channel 8 bits sample driver.");
            System.out.println("                It can play sample from 8 Khz up to 32 Khz rate.");
            System.out.println("              1 / 2ADPCM");
            System.out.println("                2 channels 4 bits ADPCM sample driver.");
            System.out.println("                It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.");
            System.out.println("              2 / 3 / 4PCM");
            System.out.println("                4 channels 8 bits sample driver with volume support.");
            System.out.println("                It can mix up to 4 samples at a fixed 16 Khz rate");
            System.out.println("                with volume support (16 levels du to memory limitation).");
            System.out.println("              4 / 5 / XGM");
            System.out.println("                XGM music with 4 channels 8 bits samples driver.");
            System.out.println("                It supports 4 PCM SFX at a fixed 14 Khz rate while playing XGM music.");
            System.out.println("  out_rate  output PCM rate (only used for Z80_DRIVER_PCM driver)");
            System.out.println("              By default the default WAV output rate is used.");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);

        SoundDriver driver = SoundDriver.PCM;
        int outRate = 0;

        // get sound driver
        if (fields.length >= 4)
            driver = Util.getSoundDriver(fields[3]);

        // determine default output rate
        switch (driver)
        {
            case DPCM2:
                outRate = 22050;
                break;

            case PCM4:
                outRate = 16000;
                break;

            case XGM:
                outRate = 14000;
                break;

            default:
                break;
        }

        // output rate
        if (fields.length >= 5)
            outRate = StringUtil.parseInt(fields[4], outRate);

        byte[] pcmData;

        try
        {
            pcmData = SoundUtil.getRawDataFromWAV(fileIn, 8, outRate, true, true, false);
        }
        catch (UnsupportedAudioFileException e)
        {
            System.err.println("Error while converting WAV '" + fileIn + "' to PCM format");
            throw new IOException(e);
        }

        // compress to DPCM
        if (driver == SoundDriver.DPCM2)
            pcmData = Util.dpcmPack(pcmData);

        // add resource file (used for deps generation)
        Compiler.addResourceFile(fileIn);

        // build BIN resource
        return new Bin(id, pcmData, (driver == SoundDriver.DPCM2) ? 128 : 256,
                (driver == SoundDriver.DPCM2) ? 128 : 256, (driver == SoundDriver.DPCM2) ? 136 : 0, Compression.NONE, false);
    }
}