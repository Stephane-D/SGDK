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
        if (fields.length < 4)
        {
            System.out.println("Wrong WAV definition");
            System.out.println("WAV name \"file\" driver [out_rate [far]]");
            System.out.println("  name      variable name");
            System.out.println("  file      path of the .wav file (will be automatically converted int the correct format)");
            System.out.println("  driver    specify the Z80 driver we will use to play the WAV file:");
            System.out.println("              PCM / DEFAULT (0 value is not anymore accepted)");
            System.out.println("                Single PCM channel sound driver.");
            System.out.println("                It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.");
            System.out.println("                Method to use: SND_PCM_startPlay(..)");
            System.out.println("              2DPCM (2ADPCM and 1 are not anymore accepted)");
            System.out.println("                2 PCM channels DPCM sound driver.");
            System.out.println("                It can mix up to 2 (4 bit) DCPM samples at a fixed 22050 Hz rate.");
            System.out.println("                Method to use: SND_DPCM2_startPlay(..)");
            System.out.println("              4PCM (2 and 3 are not anymore accepted)");
            System.out.println("                4 PCM channels sample sound driver with volume support.");
            System.out.println("                It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate with 16 levels of volume.");
            System.out.println("                Method to use: SND_PCM4_startPlay(..)");
            System.out.println("              XGM (4 and 5 are not anymore accepted)");
            System.out.println("                XGM music sound driver.");
            System.out.println("                It can mix up to 4 samples (8 bit signed) at a fixed 14 Khz rate while playing XGM music.");
            System.out.println("                Methods to use: XGM_setPCM(..) and XGM_startPlayPCM(..)");
            System.out.println("              XGM2");
            System.out.println("                XGM music sound driver.");
            System.out.println("                It can mix up to 3 samples (8 bit signed) at either 13.3 Khz or 6.65 Khz while playing XGM music.");
            System.out.println("                Methods to use: XGM2_playPCM(..)");
            System.out.println("  out_rate  output PCM rate, this parameter is meaningful only for PCM and XGM2 driver.");
            System.out.println("              PCM driver accepts following values: 8000, 11025, 13400, 16000, 22050, 32000 (default is 16000 if omitted)");
            System.out.println("              XGM2 driver accepts only 6650 or 13300 (default is 13300 if omitted).");
            System.out.println("  far       'far' binary data flag to put it at the end of the ROM (useful for bank switch, default = FALSE)");

            return null;
        }

        // get resource id
        final String id = fields[1];
        // get input file
        final String fileIn = FileUtil.adjustPath(Compiler.resDir, fields[2]);
        // get sound driver
        SoundDriver driver = Util.getSoundDriver(fields[3]);

        int outRate;

        // determine default output rate
        switch (driver)
        {
            case PCM:
                outRate = 16000;
                break;

            case DPCM2:
                outRate = 22050;
                break;

            case PCM4:
                outRate = 16000;
                break;

            case XGM:
                outRate = 14000;
                break;

            case XGM2:
                outRate = 13300;
                break;

            default:
                // 0 = use the media audio rate
                outRate = 0;
                break;
        }

        // override output rate
        if (fields.length >= 5)
            outRate = StringUtil.parseInt(fields[4], outRate);

        // get far value
        boolean far = false;
        if (fields.length >= 6)
            far = StringUtil.parseBoolean(fields[5], far);

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
        return new Bin(id, pcmData, (driver == SoundDriver.DPCM2) ? 128 : 256, (driver == SoundDriver.DPCM2) ? 128 : 256,
                (driver == SoundDriver.DPCM2) ? 136 : 0, Compression.NONE, far);
    }
}