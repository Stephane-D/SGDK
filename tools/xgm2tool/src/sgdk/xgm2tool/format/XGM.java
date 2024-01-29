package sgdk.xgm2tool.format;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.format.SampleBank.InternalSample;
import sgdk.xgm2tool.struct.YM2612State;
import sgdk.xgm2tool.tool.Util;
import sgdk.xgm2tool.tool.XGCPacker;

public class XGM
{
    // single XGM supports a maximum of 124-1 = 123 samples

    public final List<XGMSample> samples;
    public final List<XGMFMCommand> FMcommands;
    public final List<XGMPSGCommand> PSGcommands;
    public GD3 gd3;
    public XD3 xd3;
    public boolean pal;
    public boolean packed;

    protected XGM()
    {
        super();

        samples = new ArrayList<>();
        FMcommands = new ArrayList<>();
        PSGcommands = new ArrayList<>();
        gd3 = null;
        xd3 = null;
        pal = false;
        packed = false;
    }

    public XGM(byte[] data)
    {
        this();

        int flags;
        int offset, len;
        int pcmLen, fmLen, psgLen;

        if (!Launcher.silent)
            System.out.println("Parsing XGM file...");

        if (!"XGM2".equalsIgnoreCase(Util.getASCIIString(data, 0, 4)))
            throw new IllegalArgumentException("Error: XGM2 file not recognized !");

        // 0004: version (0x10 currently)
        Util.getInt8(data, 4);
        // 0005: format description (see xgm2.txt)
        flags = Util.getInt8(data, 5);

        // bit #0: NTSC / PAL information: 0=NTSC 1=PAL
        pal = (flags & 1) != 0;
        // bit #1: multi tracks file: 0=No 1=Yes (always 0 here)
        if ((flags & 2) != 0)
            throw new UnsupportedOperationException("Cannot convert from multi tracks XGM file !");

        // bit #2: GD3 tags: 0=No 1=Yes
        if ((flags & 4) != 0)
            gd3 = new GD3();
        // bit #3: packed FM / PSG / GD3 data blocks: 0=No 1=Yes
        packed = (flags & 8) != 0;

        // 0006-0007: SLEN = Sample data bloc size / 256 (ex: $0200 means 512*256 = 131072 bytes)
        pcmLen = Util.getInt16(data, 0x0006) * 256;
        // 0008-0009: FMLEN = FM music data block size / 256 (ex: $0040 means 64*256 = 16384 bytes)
        fmLen = Util.getInt16(data, 0x0008) * 256;
        // 000A-000B: PSGLEN = PSG music data block size / 256 (ex: $0020 means 32*256 = 8192 bytes)
        psgLen = Util.getInt16(data, 0x000A) * 256;

        // 000C-0103: sample id table (size = 124 entries = 248 bytes)
        for (int s = 0; s < (124 - 1); s++)
        {
            // get sample address
            final int addr = Util.getInt16(data, ((s + 0) * 2) + 0x000C);
            // get next sample address (end address)
            final int naddr = Util.getInt16(data, ((s + 1) * 2) + 0x000C);

            // does we have a sample ?
            if ((addr != 0xFFFF) && (naddr != 0xFFFF))
                // add sample (id 0 is reserved for stop operation)
                samples.add(new XGMSample(s + 1, data, 0x0104 + (addr << 8), (naddr - addr) << 8));
        }

        // calculate music data offset (0x0104 + sample data block size)
        offset = 0x0104 + pcmLen;

        if (packed)
        {
            // build FM command list
            parseFMMusic(XGCPacker.unpack(Arrays.copyOfRange(data, offset, offset + fmLen), null));
            // build PSG command list
            parsePSGMusic(XGCPacker.unpack(Arrays.copyOfRange(data, offset + fmLen, offset + fmLen + psgLen), null));

            // FIXME: loop point is not properly restored for packed XGM (loop address has been adjusted for compressed
            // data)
            // for now we just lost the loop point
            setFMLoopAddress(0);
            setPSGLoopAddress(0);
        }
        else
        {
            // build FM command list
            parseFMMusic(Arrays.copyOfRange(data, offset, offset + fmLen));
            // build PSG command list
            parsePSGMusic(Arrays.copyOfRange(data, offset + fmLen, offset + fmLen + psgLen));
        }

        updateTimes();
        updateOffsets();

        // GD3 tags ?
        if (gd3 != null)
        {
            // packed enabled ?
            if (packed)
            {
                xd3 = new XD3(data, offset + fmLen + psgLen);
                gd3 = new GD3(xd3);
            }
            else
            {
                gd3 = new GD3(data, offset + fmLen + psgLen);
                xd3 = new XD3(gd3, getTotalTimeInFrame(), getLoopDurationInFrame());
            }
        }

        if (!Launcher.silent)
        {
            System.out.println("Number of PCM sample: " + samples.size());
            System.out.println("Number of FM command : " + FMcommands.size());
            System.out.println("Number of PSG command : " + PSGcommands.size());
            System.out.println("PCM data size: " + getPCMDataSize());

            if (packed)
            {
                try
                {
                    System.out.println("FM music data size: " + getFMMusicDataSize() + " - packed size: " + getPackedFMMusicDataArray().length);
                    System.out.println("PSG music data size: " + getPSGMusicDataSize() + " - packed size: " + getPackedPSGMusicDataArray().length);
                }
                catch (IOException e)
                {
                    System.out.println("FM music data size: " + getFMMusicDataSize());
                    System.out.println("PSG music data size: " + getPSGMusicDataSize());
                }
            }
            else
            {
                System.out.println("FM music data size: " + getFMMusicDataSize());
                System.out.println("PSG music data size: " + getPSGMusicDataSize());
            }

            System.out.println("XGM duration: " + getTotalTimeInFrame() + " frames (" + getTotalTimeInSecond() + " seconds) - loop: " + getLoopDurationInFrame()
                    + " frames (" + getLoopDurationInSecond() + " seconds)");
        }
    }

    public XGM(VGM vgm, boolean pack)
    {
        this();

        if (!Launcher.silent)
            System.out.println("Converting VGM to XGM...");

        if (vgm.rate == 50)
            pal = true;
        else
            pal = false;

        gd3 = vgm.gd3;
        packed = pack;

        try
        {
            // first we extract samples from VGM
            extractSamples(vgm);
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
        // then we extract music data
        extractMusic(vgm);
        // XGM optimization
        optimizeCommands();
        // samples optimization
        optimizeSamples();

        // update times and offsets
        updateTimes();
        updateOffsets();

        // then update loop points offset
        updateLoopOffsets();

        // build XD3 after duration has been computed
        if (gd3 != null)
            xd3 = new XD3(gd3, getTotalTimeInFrame(), getLoopDurationInFrame());

        if (!Launcher.silent)
        {
            System.out.println("Number of PCM sample: " + samples.size());
            System.out.println("Number of FM command : " + FMcommands.size());
            System.out.println("Number of PSG command : " + PSGcommands.size());
            System.out.println("PCM data size: " + getPCMDataSize());

            if (packed)
            {
                try
                {
                    System.out.println("FM music data size: " + getFMMusicDataSize() + " - packed size: " + getPackedFMMusicDataArray().length);
                    System.out.println("PSG music data size: " + getPSGMusicDataSize() + " - packed size: " + getPackedPSGMusicDataArray().length);
                }
                catch (IOException e)
                {
                    System.out.println("FM music data size: " + getFMMusicDataSize());
                    System.out.println("PSG music data size: " + getPSGMusicDataSize());
                }
            }
            else
            {
                System.out.println("FM music data size: " + getFMMusicDataSize());
                System.out.println("PSG music data size: " + getPSGMusicDataSize());
            }

            System.out.println("XGM duration: " + getTotalTimeInFrame() + " frames (" + getTotalTimeInSecond() + " seconds) - loop: " + getLoopDurationInFrame()
                    + " frames (" + getLoopDurationInSecond() + " seconds)");
        }
    }

    private void parseFMMusic(byte[] data)
    {
        // parse all XGM commands
        int off = 0;
        while (off < data.length)
        {
            final XGMFMCommand command = new XGMFMCommand(data, off);

            FMcommands.add(command);
            off += command.size;

            // stop here (need to check for it as data block is aligned on 256 bytes)
            if (command.isLoop())
                break;
        }
    }

    private void parsePSGMusic(byte[] data)
    {
        // parse all XGM commands
        int off = 0;
        while (off < data.length)
        {
            final XGMPSGCommand command = new XGMPSGCommand(data, off);

            PSGcommands.add(command);
            off += command.size;

            // stop here (need to check for it as data block is aligned on 256 bytes)
            if (command.isLoop())
                break;
        }
    }

    private void extractSamples(VGM vgm) throws IOException
    {
        // extract samples
        for (SampleBank bank : vgm.sampleBanks)
        {
            for (InternalSample sample : bank.samples)
                // start from id 1 (0 is reserved for silent sample)
                samples.add(XGMSample.createFromVGMSample(samples.size() + 1, sample));
        }
    }

    private void extractMusic(VGM vgm)
    {
        // need to classify by channel
        final Map<Integer, VGMCommand> ymKeyCommands = new HashMap<>();
        final Map<Integer, List<VGMCommand>> ymChannelSetCommands = new HashMap<>();
        final Map<Integer, Integer> ymFreqCommands = new HashMap<>();
        final List<VGMCommand> ymMiscCommands = new ArrayList<>();
        final List<VGMCommand> psgCommands = new ArrayList<>();
        final List<VGMCommand> sampleCommands = new ArrayList<>();
        final List<VGMCommand> frameCommands = new ArrayList<>();

        final List<XGMFMCommand> newFMCommands = new ArrayList<>();
        final List<XGMPSGCommand> newPSGCommands = new ArrayList<>();

        int index = 0;
        int highFreqLatch = 0;

        while (index < vgm.commands.size())
        {
            // prepare new commands for this frame
            newFMCommands.clear();
            newPSGCommands.clear();

            int frameToWait = 0;

            // get frame commands
            frameCommands.clear();
            while (index < vgm.commands.size())
            {
                final VGMCommand command = vgm.commands.get(index++);

                if (command.isLoopStart())
                {
                    newFMCommands.add(new XGMFMCommand.LoopStartCommand());
                    newPSGCommands.add(new XGMPSGCommand.LoopStartCommand());
                    continue;
                }
                // ignore data block
                if (command.isDataBlock())
                    continue;
                // wait command ?
                if (command.isWait())
                {
                    // get wait
                    frameToWait += command.getWaitValue();

                    // check if next commands are wait too
                    while (index < vgm.commands.size())
                    {
                        final VGMCommand nextCom = vgm.commands.get(index);

                        // not a wait --> stop here
                        if (!nextCom.isWait())
                            break;

                        frameToWait += nextCom.getWaitValue();
                        index++;
                    }

                    // stop here
                    break;
                }
                // stop here
                if (command.isEnd())
                    break;

                // add command
                frameCommands.add(command);
            }

            // group commands
            ymKeyCommands.clear();
            ymChannelSetCommands.clear();
            ymFreqCommands.clear();
            ymMiscCommands.clear();
            psgCommands.clear();
            sampleCommands.clear();

            for (VGMCommand command : frameCommands)
            {
                int ch;
                Integer chKey;

                if (command.isStream())
                    sampleCommands.add(command);
                else if (command.isPSGWrite())
                    psgCommands.add(command);
                // YM command
                else if (command.isYM2612Write())
                {
                    ch = command.getYM2612Channel();
                    chKey = Integer.valueOf(ch);

                    // we have a key event pending for this channel ? --> transfer all previous commands now
                    if (ymKeyCommands.get(chKey) != null)
                        newFMCommands.addAll(compileYMCommands(ymChannelSetCommands, ymFreqCommands, ymMiscCommands, ymKeyCommands));

                    // frequency set command
                    if (command.isYM2612FreqWrite())
                    {
                        final int reg = command.getYM2612Register();

                        // FIXME: it seems that register 0xA4 and 0xAC has 2 separates latch
                        // case 0xa0:
                        // chip->fnum[channel] = (chip->data & 0xff) | ((chip->reg_a4 & 0x07) << 8);
                        // chip->block[channel] = (chip->reg_a4 >> 3) & 0x07;
                        // chip->kcode[channel] = (chip->block[channel] << 2) | fn_note[chip->fnum[channel] >> 7];
                        // break;
                        // case 0xa4:
                        // chip->reg_a4 = chip->data & 0xff;
                        // break;
                        // case 0xa8:
                        // chip->fnum_3ch[channel] = (chip->data & 0xff) | ((chip->reg_ac & 0x07) << 8);
                        // chip->block_3ch[channel] = (chip->reg_ac >> 3) & 0x07;
                        // chip->kcode_3ch[channel] = (chip->block_3ch[channel] << 2) | fn_note[chip->fnum_3ch[channel]
                        // >> 7];
                        // break;
                        // case 0xac:
                        // chip->reg_ac = chip->data & 0xff;
                        // break;

                        // high byte !! having a single latch should be ok as we sort commands by channel / operator !
                        if ((reg & 4) == 4)
                            highFreqLatch = (command.getYM2612Value() & 0x3F) << 8;
                        // low part, we can write it with last high part latch
                        else
                        {
                            int c;

                            // special mode freq set ? --> keep slot info (ch8-10)
                            if ((reg & 8) != 0)
                                c = 8 + (reg & 3);
                            else
                                c = ch;

                            ymFreqCommands.put(Integer.valueOf(c), Integer.valueOf(command.getYM2612Value() | highFreqLatch));
                        }
                    }
                    else if (command.isYM2612KeyWrite())
                        ymKeyCommands.put(chKey, command);
                    // general channel set command
                    else if (command.isYM2612ChannelSet())
                    {
                        List<VGMCommand> coms = ymChannelSetCommands.get(chKey);

                        if (coms == null)
                        {
                            // build the list from this command
                            coms = new ArrayList<>();
                            coms.add(command);
                            ymChannelSetCommands.put(chKey, coms);
                        }
                        // simply add the command in the list
                        else
                            coms.add(command);
                    }
                    // YM misc command
                    else
                        ymMiscCommands.add(command);
                }
                else if (Launcher.verbose)
                    System.out.println("Command " + command + " ignored");
            }

            // YM commands first
            newFMCommands.addAll(compileYMCommands(ymChannelSetCommands, ymFreqCommands, ymMiscCommands, ymKeyCommands));
            // then PCM commands
            if (!sampleCommands.isEmpty())
                newFMCommands.addAll(XGMFMCommand.createPCMCommands(this, sampleCommands));
            // and finally PSG commands (need to have them separate as they can't be processed during DMA)
            if (!psgCommands.isEmpty())
                newPSGCommands.addAll(XGMPSGCommand.createPSGCommands(psgCommands));

            // last frame ?
            if (index >= vgm.commands.size())
            {
                // add end command (loop command internally, offset will be computed later)
                newFMCommands.add(new XGMFMCommand.EndCommand());
                newPSGCommands.add(new XGMPSGCommand.EndCommand());
            }
            else
            {
                while (frameToWait > 0)
                {
                    // end frame
                    newFMCommands.add(new XGMFMCommand.FrameCommand());
                    // add end frame
                    newPSGCommands.add(new XGMPSGCommand.FrameCommand());

                    if (pal)
                        frameToWait -= 882;
                    else
                        frameToWait -= 735;
                }
            }

            // finally add the new commands
            FMcommands.addAll(newFMCommands);
            PSGcommands.addAll(newPSGCommands);
        }
    }

    private void setFMLoopAddress(int addr)
    {
        // Loop is the last command
        final XGMFMCommand loopCom = FMcommands.get(FMcommands.size() - 1);
        // set loop address
        loopCom.setLoopAddr(addr);
    }

    private void setPSGLoopAddress(int addr)
    {
        // Loop is the last command
        final XGMPSGCommand loopCom = PSGcommands.get(PSGcommands.size() - 1);
        // set loop address
        loopCom.setLoopAddr(addr);
    }

    private void updateLoopOffsets()
    {
        final XGMFMCommand fmLoopStartCom = getFMLoopStartCommand();
        final XGMPSGCommand psgLoopStartCom = getPSGLoopStartCommand();

        if (fmLoopStartCom != null)
            setFMLoopAddress(fmLoopStartCom.getOriginOffset());
        if (psgLoopStartCom != null)
            setPSGLoopAddress(psgLoopStartCom.getOriginOffset());
    }

    private Map<Integer, List<XGMFMCommand>> getFMCommandsPerFrame()
    {
        final Map<Integer, List<XGMFMCommand>> result = new HashMap<>();

        int c = 0;
        int frame = 0;
        while (c < FMcommands.size())
        {
            final List<XGMFMCommand> frameCommands = new ArrayList<>();

            // not a wait command ?
            while (!FMcommands.get(c).isWait(false))
            {
                // add command to frame list
                frameCommands.add(FMcommands.get(c));

                // done ? --> stop here
                if (++c >= FMcommands.size())
                    break;
            }

            final Integer key = Integer.valueOf(frame);

            if (c < FMcommands.size())
            {
                // get wait command
                final XGMFMCommand com = FMcommands.get(c);
                // add it to frame list
                frameCommands.add(com);

                // next frame
                frame += FMcommands.get(c).getWaitFrame();
                c++;
            }

            // add commands for this frame
            result.put(key, frameCommands);
        }

        return result;
    }

    private Map<Integer, List<XGMPSGCommand>> getPSGCommandsPerFrame()
    {
        final Map<Integer, List<XGMPSGCommand>> result = new HashMap<>();

        int c = 0;
        int frame = 0;
        while (c < PSGcommands.size())
        {
            final List<XGMPSGCommand> frameCommands = new ArrayList<>();

            // not a wait command ?
            while (!PSGcommands.get(c).isWait(false))
            {
                // add command to frame list
                frameCommands.add(PSGcommands.get(c));

                // done ? --> stop here
                if (++c >= PSGcommands.size())
                    break;
            }

            final Integer key = Integer.valueOf(frame);

            if (c < PSGcommands.size())
            {
                // get wait command
                final XGMPSGCommand com = PSGcommands.get(c);
                // add it to frame list
                frameCommands.add(com);

                // next frame
                frame += PSGcommands.get(c).getWaitFrame();
                c++;
            }

            // add commands for this frame
            result.put(key, frameCommands);
        }

        return result;
    }

    private void packWaitFM()
    {
        int c = 0;
        while (c < FMcommands.size())
        {
            // get next wait command
            while (!FMcommands.get(c).isWait(true))
            {
                // done ? --> stop here
                if (++c >= FMcommands.size())
                    return;
            }

            final int startInd = c;
            int wait = 0;
            // get next no wait command
            while (FMcommands.get(c).isWait(true))
            {
                // sum wait
                wait += FMcommands.get(c).getWaitFrame();
                // done ? --> stop here
                if (++c >= FMcommands.size())
                    break;
            }

            // remove all wait commands
            while (c > startInd)
                FMcommands.remove(--c);

            // add new wait commands
            final List<XGMFMCommand> waitComs = XGMFMCommand.createWaitCommands(wait);
            FMcommands.addAll(startInd, waitComs);

            // next
            c += waitComs.size();
        }
    }

    private void packWaitPSG()
    {
        int c = 0;
        while (c < PSGcommands.size())
        {
            // get next wait command
            while (!PSGcommands.get(c).isWait(true))
            {
                // done ? --> stop here
                if (++c >= PSGcommands.size())
                    return;
            }

            final int startInd = c;
            int wait = 0;
            // get next no wait command
            while (PSGcommands.get(c).isWait(true))
            {
                // sum wait
                if (!PSGcommands.get(c).isDummy())
                    wait += PSGcommands.get(c).getWaitFrame();

                // done ? --> stop here
                if (++c >= PSGcommands.size())
                    break;
            }

            // remove all wait commands
            while (c > startInd)
                PSGcommands.remove(--c);

            // add new wait commands
            final List<XGMPSGCommand> waitComs = XGMPSGCommand.createWaitCommands(wait);
            PSGcommands.addAll(startInd, waitComs);

            // next
            c += waitComs.size();
        }
    }

    private static List<XGMFMCommand> optimizeSetPanning(int port, int ch, List<XGMFMCommand> fmWriteCHCommands, YM2612State ymState)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (XGMFMCommand com : fmWriteCHCommands)
            result.addAll(XGMFMCommand.convertToSetPanningCommands(port, ch, com, ymState));

        return result;
    }

    private static List<XGMFMCommand> optimizeSetTL(int port, int ch, List<XGMFMCommand> fmWriteCHCommands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (XGMFMCommand com : fmWriteCHCommands)
            result.addAll(XGMFMCommand.convertToSetTLCommands(port, ch, com));

        return result;
    }

    private static List<XGMFMCommand> optimizeLoadInst(int port, int ch, List<XGMFMCommand> fmWriteCHCommands, YM2612State ymState)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        // load inst command is 31 bytes
        // we optimize as soon FM writes are >= 16 bytes (longer to parse and load inst command compress better)
        if (Command.getDataSize(fmWriteCHCommands) >= 16)
        {
            final XGMFMCommand com = XGMFMCommand.convertToLoadInstCommand(port, ch, fmWriteCHCommands, ymState);

            if (com != null)
                result.add(com);
        }

        return result;
    }

    private void removeDuplicateSetFreq(List<XGMFMCommand> fmChannelCommands)
    {
        boolean freqSet = false;

        // only keep the last one so we start by the end
        for (int c = fmChannelCommands.size() - 1; c >= 0; c--)
        {
            final XGMFMCommand com = fmChannelCommands.get(c);

            // don't optimize special mode freq set
            if (com.isYMFreqSpecialWrite() || com.isYMFreqDeltaSpecialWrite())
                continue;

            if (com.isYMFreqWrite() || com.isYMFreqDeltaWrite())
            {
                // already set ? --> set dummy
                if (freqSet)
                    com.setDummy();
                else
                    freqSet = true;
            }
        }
    }

    private void cleanKeyCommands(List<XGMFMCommand> fmChannelCommands)
    {
        boolean hasKeyOn = false;
        boolean hasKeyOff = false;
        // boolean hasLoadInst = false;

        // start from end of frame
        for (int c = fmChannelCommands.size() - 1; c >= 0; c--)
        {
            final XGMFMCommand com = fmChannelCommands.get(c);

            if (com.isYMKeyONWrite())
            {
                // can't have several key-on in a single frame
                if (hasKeyOn)
                    com.setDummy();
                else
                    hasKeyOn = true;
            }
            else if (com.isYMKeyOFFWrite())
            {
                // can't have more than 2 key-off in a single frame
                if (hasKeyOff)
                    com.setDummy();
                else
                {
                    // allow an extra key-off after a key-on
                    if (hasKeyOn)
                        hasKeyOff = true;

                    // // ignore any key-off before a load inst command
                    // if (hasLoadInst)
                    // com.setDummy();
                }
            }
            // else if (com.isYMLoadInst())
            // hasLoadInst = true;
        }

        Boolean keyState = null;
        // then in normal frame order
        for (XGMFMCommand com : fmChannelCommands)
        {
            // if (com.isYMLoadInst())
            // keyState = Boolean.FALSE;
            // else
            if (com.isYMKeyONWrite())
            {
                // already on
                if ((keyState != null) && keyState.booleanValue())
                    com.setDummy();

                keyState = Boolean.TRUE;
            }
            else if (com.isYMKeyOFFWrite())
            {
                // already off
                if ((keyState != null) && !keyState.booleanValue())
                    com.setDummy();

                keyState = Boolean.FALSE;
            }
        }
    }

    private void optimizeKeySeqCommands(List<XGMFMCommand> fmChannelCommands)
    {
        XGMFMCommand lastKeyOFF = null;
        XGMFMCommand lastKeyON = null;

        for (XGMFMCommand com : fmChannelCommands)
        {
            if (com.isYMKeyOFFWrite())
            {
                // combine key on-off seq
                if (lastKeyON != null)
                {
                    lastKeyON.setDummy();
                    lastKeyON = null;
                    com.toKeySeq(true);
                }
                else
                    lastKeyOFF = com;
            }
            else if (com.isYMKeyONWrite())
            {
                // combine key off-on seq
                if (lastKeyOFF != null)
                {
                    lastKeyOFF.setDummy();
                    lastKeyOFF = null;
                    com.toKeySeq(false);
                }
                else
                    lastKeyON = com;
            }
            else if (com.isYMSetTL() || com.isYMKeyAdvWrite() || com.isYMLoadInst() || com.isYMWrite())
            {
                // can't combine
                lastKeyOFF = null;
                lastKeyON = null;
            }
        }
    }

    private void combineSetFreqKeyCommands(List<XGMFMCommand> fmChannelCommands)
    {
        boolean canCombine = false;
        XGMFMCommand lastKeyOFF = null;
        XGMFMCommand lastKeyON = null;

        // safe key-on combine (start from last frame)
        for (int c = fmChannelCommands.size() - 1; c >= 0; c--)
        {
            final XGMFMCommand com = fmChannelCommands.get(c);

            if (com.isYMFreqWrite())
            {
                if (canCombine)
                {
                    com.setYMFreqKeyON();
                    lastKeyON.setDummy();
                }
                canCombine = false;
            }
            else if (com.isYMKeyONWrite())
            {
                canCombine = true;
                lastKeyON = com;
            }
            else if (com.isYMKeyOFFWrite())
                canCombine = false;
            else if (com.isYMSetTL() || com.isYMKeyAdvWrite() || com.isYMLoadInst() || com.isYMWrite())
                canCombine = false;
        }

        // safe key-off combine (start from first frame)
        for (int c = 0; c < fmChannelCommands.size(); c++)
        {
            final XGMFMCommand com = fmChannelCommands.get(c);

            if (com.isYMFreqWrite())
            {
                if (canCombine)
                {
                    com.setYMFreqKeyOFF();
                    lastKeyOFF.setDummy();
                }
                canCombine = false;
            }
            else if (com.isYMKeyOFFWrite())
            {
                canCombine = true;
                lastKeyOFF = com;
            }
            else if (com.isYMKeyONWrite())
                canCombine = false;
            else if (com.isYMSetTL() || com.isYMKeyAdvWrite() || com.isYMLoadInst() || com.isYMWrite())
                canCombine = false;
        }

        boolean hasKeyOn = false;
        boolean canCombineOFF = false;
        boolean canCombineON = false;
        XGMFMCommand setFreq = null;

        // isolated key-on / key-off combine
        for (XGMFMCommand com : fmChannelCommands)
        {
            if (com.isYMKeyOFFWrite())
            {
                // combine key off only if we don't have a keyOn before
                if (!hasKeyOn)
                {
                    canCombineOFF = true;
                    lastKeyOFF = com;
                }
            }
            else if (com.isYMKeyONWrite())
            {
                canCombineON = true;
                hasKeyOn = true;
                lastKeyON = com;
            }
            else if (com.isYMSetTL() || com.isYMKeyAdvWrite() || com.isYMLoadInst() || com.isYMWrite())
            {
                // not yet meet the set freq command ? --> cancel combination
                if (setFreq == null)
                {
                    canCombineOFF = false;
                    canCombineON = false;
                }
            }
            else if (com.isYMFreqWrite())
            {
                setFreq = com;
                if (com.isYMFreqWithKeyON())
                    hasKeyOn = true;
            }
        }

        if (setFreq != null)
        {
            if (canCombineOFF && !setFreq.isYMFreqWithKeyOFF())
            {
                setFreq.setYMFreqKeyOFF();
                lastKeyOFF.setDummy();
            }
            if (canCombineON && !setFreq.isYMFreqWithKeyON())
            {
                setFreq.setYMFreqKeyON();
                lastKeyON.setDummy();
            }
        }
    }

    private void packFMFreqCommands(List<XGMFMCommand> fmChannelCommands)
    {
        final int lastFreq[] = new int[] {-1, -1, -1, -1};

        for (XGMFMCommand com : fmChannelCommands)
        {
            // reset state
            if (com.isLoopStart())
                Arrays.fill(lastFreq, -1);
            else if (com.isYMFreqWrite())
            {
                final int freq = com.getYMFreqValue();
                final int slot = com.isYMFreqSpecialWrite() ? com.getYMSlot() : 0;

                // cannot pack set Freq with key event
                if ((lastFreq[slot] != -1) && !com.isYMFreqWithKeyWrite())
                {
                    // compute delta
                    final int delta = freq - lastFreq[slot];

                    if (delta == 0)
                    {
                        // this can happen when we removed duplicate set freq --> set dummy
                        // System.out.println("FM freq delta = 0");
                        com.setDummy();
                    }
                    else if (Math.abs(delta) <= 128)
                        com.toFreqDelta(delta);
                }

                lastFreq[slot] = freq;
            }
        }
    }

    private void convertFMTLCommands(List<XGMFMCommand> fmChannelCommands)
    {
        final int lastTL[] = new int[] {-1, -1, -1, -1};

        for (XGMFMCommand com : fmChannelCommands)
        {
            // reset state
            if (com.isLoopStart())
                Arrays.fill(lastTL, -1);
            else if (com.isYMLoadInst())
            {
                // set TL state from load_inst command
                for (int s = 0; s < 4; s++)
                    lastTL[s] = com.data[1 + 4 + s] & 0x7F;
            }
            else if (com.isYMSetTL())
            {
                final int tl = com.getYMTLValue();
                final int slot = com.getYMSlot();

                if (lastTL[slot] != -1)
                {
                    // compute delta
                    final int delta = tl - lastTL[slot];

                    if (delta == 0)
                    {
                        if (Launcher.verbose)
                            System.out.println("TL delta = 0");
                        com.setDummy();
                    }
                    else if (Math.abs(delta) <= 64)
                        com.toTLDelta(delta);
                }

                lastTL[slot] = tl;
            }
        }
    }

    private int useExtWaitFMCommand(List<XGMFMCommand> fmFrameCommands)
    {
        // no need to process
        if (fmFrameCommands.size() <= 1)
            return -1;

        final XGMFMCommand waitCom = fmFrameCommands.get(fmFrameCommands.size() - 1);

        // we should have the wait command in last position otherwise we may have reached the end of the track ! (loop
        // command)
        if (!waitCom.isWait(false))
            return -1;
        // don't convert if we have a multi frame wait
        if (waitCom.getWaitFrame() > 1)
            return -1;

        boolean[] hasKeyCh = new boolean[6];
        Arrays.fill(hasKeyCh, false);

        // start from last command before wait
        for (int c = fmFrameCommands.size() - 2; c >= 0; c--)
        {
            final XGMFMCommand com = fmFrameCommands.get(c);

            // can add wait and no key com found for this channel ?
            if (com.supportWait() && !hasKeyCh[com.getChannel()])
            {
                // add it
                com.addWait();
                // make wait command dummy
                waitCom.setDummy();
                // return index of current command
                return c;
            }
            // better to not swap command order then...
            else if (com.isYMKeyWrite())
                hasKeyCh[com.getChannel()] = true;
        }

        return -1;
    }

    private void optimizeFMCommands()
    {
        // 1. replace FM write by load instrument com
        // 2. replace FM write by set TL com
        // 3. replace FM write by panning com
        // 4. remove duplicate set ch freq in a single frame (keep last one)
        // 5. cleanup duplicate Key OFF/ON (start from end of frame)
        // 6. combine key ON/OFF with set FREQ com
        // 7. replace set FREQ with set FREQ low com when possible
        // 8. last pack key commands:
        // OFF followed by ON in same frame without FM write in between --> ON
        // ON followed by OFF in same frame without FM write in between --> OFF
        // 9. pack wait (should be last)

        if (Launcher.verbose)
        {
            int numKeyCom = 0;
            int numKeyAdvCom = 0;
            int numSetFreqCom = 0;
            int numFMSetCom = 0;
            int numWaitCom = 0;

            for (XGMFMCommand com : FMcommands)
            {
                if (com.isYMKeyFastWrite())
                    numKeyCom++;
                else if (com.isYMKeyAdvWrite())
                    numKeyAdvCom++;
                else if (com.isYMFreqWrite())
                    numSetFreqCom++;
                else if (com.isYMWrite())
                    numFMSetCom++;
                else if (com.isWait(true))
                    numWaitCom++;
            }

            if (Launcher.verbose)
            {
                System.out.println("FM command stats before optimization:");
                System.out.println("FM set = " + numFMSetCom + " - set freq = " + numSetFreqCom + " - key = " + numKeyCom + " - key adv = " + numKeyAdvCom
                        + " - wait = " + numWaitCom);
            }
        }

        final List<XGMFMCommand> newCommands = new ArrayList<>();
        final YM2612State ymState = new YM2612State();

        Map<Integer, List<XGMFMCommand>> commandsPerFrame = getFMCommandsPerFrame();
        List<Integer> frames = new ArrayList<>(commandsPerFrame.keySet());
        // sort frames
        Collections.sort(frames);

        // do frame optimization
        for (Integer frame : frames)
        {
            // get commands for this frame
            final List<XGMFMCommand> frameCommands = commandsPerFrame.get(frame);

            for (int channel = 0; channel < 6; channel++)
            {
                final int port = (channel >= 3) ? 1 : 0;
                final int ch = (channel >= 3) ? channel - 3 : channel;

                // get commands for current channel
                final List<XGMFMCommand> fmChannelCommands = XGMFMCommand.filterChannel(frameCommands, channel, false, false);

                int startInd = 0;
                while (startInd < fmChannelCommands.size())
                {
                    // need to split on key command as envelop settings are updated on key on/off event
                    int endInd = XGMFMCommand.findNextYMKeyCommand(fmChannelCommands, startInd);

                    // get YMWrite commands for current channel
                    final List<XGMFMCommand> fmWriteChannelCommands = XGMFMCommand.filterYMWrite(fmChannelCommands, startInd, endInd);

                    // FM write commands optimization
                    if (!fmWriteChannelCommands.isEmpty())
                    {
                        // clear
                        newCommands.clear();

                        // convert to load inst command
                        newCommands.addAll(optimizeLoadInst(port, ch, fmWriteChannelCommands, ymState));
                        // convert to set TL
                        newCommands.addAll(optimizeSetTL(port, ch, fmWriteChannelCommands));
                        // convert to set panning
                        newCommands.addAll(optimizeSetPanning(port, ch, fmWriteChannelCommands, ymState));

                        // update YM state from remaining commands
                        ymState.updateState(fmWriteChannelCommands);

                        if (!newCommands.isEmpty())
                        {
                            int index;

                            // get index of first command
                            index = FMcommands.indexOf(fmWriteChannelCommands.get(0));
                            // insert new commands here
                            FMcommands.addAll(index, newCommands);

                            // get index of first command
                            index = fmChannelCommands.indexOf(fmWriteChannelCommands.get(0));
                            // insert new commands here
                            fmChannelCommands.addAll(index, newCommands);
                            // need to adjust end index
                            endInd += newCommands.size();

                            // update state from new commands
                            ymState.updateState(newCommands);
                        }
                    }

                    startInd = endInd + 1;
                }

                // FM commands optimization
                if (!fmChannelCommands.isEmpty())
                {
                    // remove duplicate set freq
                    removeDuplicateSetFreq(fmChannelCommands);
                    // cleanup key commands
                    cleanKeyCommands(fmChannelCommands);
                    // combine key / set freq commands
                    combineSetFreqKeyCommands(fmChannelCommands);
                    // optimize key sequence commands
                    optimizeKeySeqCommands(fmChannelCommands);
                }
            }
        }

        // need to update time before processing global optimizations
        updateTimes();

        // do global optimizations
        for (int channel = 0; channel < 6; channel++)
        {
            // get commands for current channel (with wait commands as well)
            final List<XGMFMCommand> fmChannelCommands = XGMFMCommand.filterChannel(FMcommands, channel, false, true);

            // pack FREQ to FREQ DELTA when possible
            packFMFreqCommands(fmChannelCommands);
            // convert set TL to delta TL when possible
            convertFMTLCommands(fmChannelCommands);
        }

        packWaitFM();

        // update commands per frame
        commandsPerFrame = getFMCommandsPerFrame();
        frames = new ArrayList<>(commandsPerFrame.keySet());
        // sort frames
        Collections.sort(frames);

        // do last frame optimization
        for (Integer frame : frames)
        {
            // get commands for this frame
            final List<XGMFMCommand> frameCommands = commandsPerFrame.get(frame);

            // convert to ext wait commands when possible
            final int indOpt = useExtWaitFMCommand(frameCommands);

            // optimized command is not the last frame command ?
            if ((indOpt != -1) && (indOpt != (frameCommands.size() - 2)))
            {
                // swap with last command
                final int ind1 = FMcommands.indexOf(frameCommands.get(indOpt));
                final int ind2 = FMcommands.indexOf(frameCommands.get(frameCommands.size() - 2));
                Collections.swap(FMcommands, ind1, ind2);
            }
        }

        // rebuild final / cleaned list
        rebuildFMCommands();

        if (Launcher.verbose)
        {
            int numLoadInstCom = 0;
            int numTLCom = 0;
            int numTLDeltaCom = 0;
            int numPANCom = 0;
            int numSetFreqCom = 0;
            int numSetFreqDelta = 0;
            int numKeyCom = 0;
            int numKeySeqCom = 0;
            int numKeyAdvCom = 0;
            int numFMSetCom = 0;
            int numWaitCom = 0;

            for (XGMFMCommand com : FMcommands)
            {
                if (com.isYMLoadInst())
                    numLoadInstCom++;
                else if (com.isYMSetTL())
                    numTLCom++;
                else if (com.isYMTLDelta())
                    numTLDeltaCom++;
                else if (com.isYMPAN())
                    numPANCom++;
                else if (com.isYMFreqWrite())
                    numSetFreqCom++;
                else if (com.isYMFreqDeltaWrite())
                    numSetFreqDelta++;
                else if (com.isYMKeyFastWrite())
                    numKeyCom++;
                else if (com.isYMKeySequence())
                    numKeySeqCom++;
                else if (com.isYMKeyAdvWrite())
                    numKeyAdvCom++;
                else if (com.isYMWrite())
                    numFMSetCom++;
                else if (com.isWait(true))
                    numWaitCom++;
            }

            if (Launcher.verbose)
            {
                System.out.println("Generated " + numLoadInstCom + " FM Load inst. commands");
                System.out.println("Generated " + numTLCom + " FM set TL commands");
                System.out.println("Generated " + numTLDeltaCom + " FM TL delta commands");
                System.out.println("Generated " + numPANCom + " FM set PAN commands");
                System.out.println("FM command stats after optimization:");
                System.out.println("FM set = " + numFMSetCom + " - set freq = " + numSetFreqCom + " - set freq delta = " + numSetFreqDelta + " - key = "
                        + numKeyCom + " - key adv = " + numKeyAdvCom + " - key seq = " + numKeySeqCom + " - wait = " + numWaitCom);
            }
        }
    }

    public void rebuildFMCommands()
    {
        List<XGMFMCommand> newCommands = new ArrayList<>();

        // rebuild final / cleaned list
        int frameSize = 0;
        for (XGMFMCommand com : FMcommands)
        {
            if (!com.isDummy())
            {
                // above max frame size (take 1 extra byte for frame delay marker) ?
                if ((frameSize + com.size) > (XGCPacker.FRAME_MAX_SIZE - 1))
                {
                    System.out.println("Warning: maximum frame size exceeded (FM frame #" + com.getFrame(pal) + ")");

                    // insert frame delay command (0xF0)
                    newCommands.add(new XGMFMCommand.FrameDelayCommand());
                    // reset frame size
                    frameSize = 0;
                }
                else if (com.isWait(false))
                    // reset frame size
                    frameSize = 0;
                else
                    // increment frame size
                    frameSize += com.size;

                // add command
                newCommands.add(com);
            }
        }

        FMcommands.clear();
        FMcommands.addAll(newCommands);

        updateOffsets();
        updateTimes();
    }

    public void rebuildPSGCommands()
    {
        List<XGMPSGCommand> newCommands = new ArrayList<>();

        int frameSize = 0;
        for (XGMPSGCommand com : PSGcommands)
        {
            if (!com.isDummy())
            {
                // above max frame size (take 1 extra byte for frame delay marker) ?
                if ((frameSize + com.size) > (XGCPacker.FRAME_MAX_SIZE - 1))
                {
                    System.out.println("Warning: maximum frame size exceeded (PSG frame #" + com.getFrame(pal) + ")");

                    // insert frame delay command (0xF0)
                    newCommands.add(new XGMPSGCommand.FrameCommand());
                    // reset frame size
                    frameSize = 0;
                }
                else if (com.isWait(false))
                    // reset frame size
                    frameSize = 0;
                else
                    // increment frame size
                    frameSize += com.size;

                // add command
                newCommands.add(com);
            }
        }

        PSGcommands.clear();
        PSGcommands.addAll(newCommands);

        updateOffsets();
        updateTimes();
    }

    private void removeDuplicatedFreqEnv(List<XGMPSGCommand> psgFrameChannelCommands)
    {
        // start from last command
        boolean freqLowSet = false;
        boolean freqSet = false;
        boolean envSet = false;
        for (int c = psgFrameChannelCommands.size() - 1; c >= 0; c--)
        {
            final XGMPSGCommand com = psgFrameChannelCommands.get(c);

            if (com.isEnv())
            {
                // ENV is already set in the frame --> set dummy
                if (envSet)
                    com.setDummy();
                else
                    envSet = true;
            }
            else if (com.isFreq())
            {
                // FREQ is already set in the frame --> set dummy
                if (freqSet)
                    com.setDummy();
                else
                    freqSet = true;
            }
            else if (com.isFreqLow())
            {
                // FREQ is already set in the frame --> set dummy
                if (freqLowSet || freqSet)
                    com.setDummy();
                else
                    freqLowSet = true;
            }
        }
    }

    private int useExtWaitPSGCommand(List<XGMPSGCommand> psgFrameCommands)
    {
        // no need to process
        if (psgFrameCommands.size() < 2)
            return -1;

        final XGMPSGCommand waitCom = psgFrameCommands.get(psgFrameCommands.size() - 1);

        // we should have the wait command in last position otherwise we may have reached the end of the track ! (loop
        // command)
        if (!waitCom.isWait(false))
            return -1;
        // don't convert if we have a multi frame wait
        if (waitCom.getWaitFrame() > 1)
            return -1;

        // start from last command before wait
        for (int c = psgFrameCommands.size() - 2; c >= 0; c--)
        {
            final XGMPSGCommand com = psgFrameCommands.get(c);

            // can add wait ?
            if (com.supportWait())
            {
                // add it
                com.addWait();
                // make wait command dummy
                waitCom.setDummy();
                // return index of current command
                return c;
            }
        }

        return -1;
    }

    private void removeSilentPSGFreqCommands(List<XGMPSGCommand> psgChannelCommands)
    {
        boolean silent = false;
        XGMPSGCommand lastDummyFreqCom = null;
        XGMPSGCommand lastDummyFreqLowCom = null;
        int lastFreq = 0;

        for (XGMPSGCommand com : psgChannelCommands)
        {
            if (com.isDummy())
                continue;

            // loop start ? --> reset state
            if (com.isLoopStart())
            {
                silent = false;
                lastDummyFreqCom = null;
                lastDummyFreqLowCom = null;
            }
            else if (com.isEnv())
            {
                // changed to silent ?
                if (com.getEnv() == 0xF)
                {
                    silent = true;
                    // reset it
                    lastDummyFreqCom = null;
                    lastDummyFreqLowCom = null;
                }
                else
                {
                    // changed from silent to audible ?
                    if (silent)
                    {
                        // restore last remove setFreq command
                        if (lastDummyFreqCom != null)
                        {
                            lastDummyFreqCom.clearDummy();
                            lastDummyFreqCom.setFreq(lastFreq);
                        }
                        else if (lastDummyFreqLowCom != null)
                        {
                            lastDummyFreqLowCom.clearDummy();
                            lastDummyFreqLowCom.setFreqLow(lastFreq & 0xF);
                        }

                        silent = false;
                    }
                }
            }
            else if (com.isFreq())
            {
                // channel is currently silent ? --> set dummy
                if (silent)
                {
                    lastFreq = com.getFreq();
                    com.setDummy();
                    // store last removed setFreq com
                    lastDummyFreqCom = com;
                }
            }
            else if (com.isFreqLow())
            {
                // channel is currently silent ? --> set dummy
                if (silent)
                {
                    lastFreq = (lastFreq & 0x3F0) | com.getFreqLow();
                    com.setDummy();
                    // store last removed setFreq com
                    lastDummyFreqLowCom = com;
                }
            }
        }
    }

    private void packPSGFreqCommands(List<XGMPSGCommand> psgChannelCommands)
    {
        int lastFreq = -1;
        for (XGMPSGCommand com : psgChannelCommands)
        {
            if (com.isDummy())
                continue;

            // loop start ? --> reset state
            if (com.isLoopStart())
                lastFreq = -1;
            else if (com.isFreq())
            {
                int freq = com.getFreq();

                if (lastFreq != -1)
                {
                    // compute delta
                    final int delta = freq - lastFreq;

                    // can happen with the 'removeSilentPSGFreqCommands' optimization pass
                    if (delta == 0)
                        com.setDummy();
                    else if (Math.abs(delta) <= 4)
                        com.toFreqDelta(delta);
                    // only low part changed ? --> change to freq low (same size but faster processing and potentially
                    // compress better)
                    else if ((lastFreq & 0xFF0) == (freq & 0xFF0))
                        com.toFreqLow(freq & 0xF);
                }

                lastFreq = freq;
            }
            else if (com.isFreqLow())
            {
                if (lastFreq != -1)
                {
                    // new freq
                    final int freq = (lastFreq & 0xFF0) | com.getFreqLow();
                    // compute delta
                    final int delta = freq - lastFreq;

                    // can happen with the 'removeSilentPSGFreqCommands' optimization pass
                    if (delta == 0)
                        com.setDummy();
                    else if (Math.abs(delta) <= 4)
                        com.toFreqDelta(delta);

                    lastFreq = freq;
                }
            }
        }
    }

    private void convertPSGEnvCommands(List<XGMPSGCommand> psgChannelCommands)
    {
        int lastEnv = -1;

        for (XGMPSGCommand com : psgChannelCommands)
        {
            if (com.isDummy())
                continue;

            // loop start ? --> reset state
            if (com.isLoopStart())
                lastEnv = -1;
            else if (com.isEnv())
            {
                final int env = com.getEnv();

                if (lastEnv != -1)
                {
                    // compute delta
                    final int delta = env - lastEnv;

                    if (delta == 0)
                    {
                        if (Launcher.verbose)
                            System.out.println("env delta = 0");
                        com.setDummy();
                    }
                    else if (Math.abs(delta) <= 4)
                        com.toEnvDelta(delta);
                }

                lastEnv = env;
            }
        }
    }

    private void optimizePSGCommands()
    {
        // NEW: add this for better optimization
        // 1. remove duplicate set FREQ and set ENV commands per frame / channel
        // 2. remove all FREQ commands when ENV keep silent
        // 3. replace FREQ by FREQ_LOW when possible
        // 5. pack wait
        // 4. replace ENV/FREQ/FREQ_LOW with .WAIT extension when possible

        Map<Integer, List<XGMPSGCommand>> commandsPerFrame = getPSGCommandsPerFrame();
        List<Integer> frames = new ArrayList<>(commandsPerFrame.keySet());
        // sort frames
        Collections.sort(frames);

        // do frame optimization
        for (Integer frame : frames)
        {
            // get commands for this frame
            final List<XGMPSGCommand> frameCommands = commandsPerFrame.get(frame);

            for (int channel = 0; channel < 4; channel++)
            {
                // get commands for current channel (with wait commands as well)
                final List<XGMPSGCommand> psgFrameChannelCommands = XGMPSGCommand.filterChannel(frameCommands, channel, false, false);

                // remove duplicate
                removeDuplicatedFreqEnv(psgFrameChannelCommands);
            }
        }

        // do global optimizations
        for (int channel = 0; channel < 4; channel++)
        {
            // get commands for current channel (with wait commands as well)
            final List<XGMPSGCommand> psgChannelCommands = XGMPSGCommand.filterChannel(PSGcommands, channel, true, true);

            // remove inaudible set FREQ command (only for channel 0-1)
            if (channel < 2)
                removeSilentPSGFreqCommands(psgChannelCommands);
            // pack FREQ to FREQ DELTA / LOW when possible
            packPSGFreqCommands(psgChannelCommands);
            // convert ENV to ENV DELTA when possible
            convertPSGEnvCommands(psgChannelCommands);
        }

        if (Launcher.verbose)
        {
            int numEnv = 0;
            int numFreq = 0;
            int numWaitCom = 0;
            int converted = 0;

            for (XGMPSGCommand com : PSGcommands)
            {
                if (com.isDummy())
                    continue;

                if (com.isFreq())
                    numFreq++;
                else if (com.isFreqLow())
                    numFreq++;
                else if (com.isFreqDelta())
                    converted++;
                else if (com.isEnv())
                    numEnv++;
                else if (com.isWait(true))
                    numWaitCom++;
            }

            if (Launcher.verbose)
            {
                System.out.println("PSG command stats: set tone = " + (numFreq + converted) + " - set env = " + numEnv);
                System.out.println("Packed " + converted + " PSG set tone commands");
                System.out.println("PSG wait command before optimization = " + numWaitCom);
            }
        }

        // build cleaned list (needed for proper wait packing)
        final List<XGMPSGCommand> newCommands = new ArrayList<>();
        for (XGMPSGCommand com : PSGcommands)
        {
            if (!com.isDummy())
                newCommands.add(com);
        }

        PSGcommands.clear();
        PSGcommands.addAll(newCommands);

        // pack wait
        packWaitPSG();

        // update commands per frame
        commandsPerFrame = getPSGCommandsPerFrame();
        frames = new ArrayList<>(commandsPerFrame.keySet());
        // sort frames
        Collections.sort(frames);

        // do last frame optimization
        for (Integer frame : frames)
        {
            // get commands for this frame
            final List<XGMPSGCommand> frameCommands = commandsPerFrame.get(frame);

            // convert to ext wait commands when possible
            final int indOpt = useExtWaitPSGCommand(frameCommands);

            // optimized command is not the last frame command ?
            if ((indOpt != -1) && (indOpt != (frameCommands.size() - 2)))
            {
                // swap with last command
                final int ind1 = PSGcommands.indexOf(frameCommands.get(indOpt));
                final int ind2 = PSGcommands.indexOf(frameCommands.get(frameCommands.size() - 2));
                Collections.swap(PSGcommands, ind1, ind2);
            }
        }

        if (Launcher.verbose)
        {
            int numWaitCom = 0;

            for (XGMPSGCommand com : PSGcommands)
            {
                if (com.isWait(true))
                    numWaitCom++;
            }

            System.out.println("PSG wait command after optimization = " + numWaitCom);
        }

        // rebuild final / cleaned list
        rebuildPSGCommands();
    }

    private void optimizeCommands()
    {
        optimizeFMCommands();
        optimizePSGCommands();
    }

    private void optimizeSamples()
    {
        final int numSample = samples.size();

        // start from end as we delete merged samples
        for (int s = samples.size() - 1; s >= 0; s--)
            mergeSample(s);

        // reset sample ids
        for (int s = 0; s < samples.size(); s++)
        {
            final XGMSample sample = samples.get(s);
            updateSampleCommands(sample.id, s + 1, -1);
            sample.id = s + 1;
        }

        // maximum number of sample reached ?
        if (samples.size() >= (124 - 1))
        {
            System.err.println("Warning: XGM cannot have more than 123 samples, some samples will be lost !");

            // remove extra samples
            while (samples.size() > 123)
                samples.remove(samples.size() - 1);

            return;
        }

        rebuildFMCommands();

        if ((samples.size() != numSample) && !Launcher.silent)
            System.out.println("Merged " + (numSample - samples.size()) + " sample(s)");
    }

    private XGMSample findMatchingSample(XGMSample sample)
    {
        XGMSample bestMatch = null;
        double bestScore = 0d;

        for (XGMSample s : samples)
        {
            if (s != sample)
            {
                final double score = s.getSimilarityScore(sample);
                if (score > bestScore)
                {
                    bestMatch = s;
                    bestScore = score;
                }
            }
        }

        // accept only if score >= 1
        if (bestScore >= 1)
            return bestMatch;

        return null;
    }

    private void mergeSample(int sampleIndex)
    {
        final XGMSample sample = samples.get(sampleIndex);
        // find if already exist
        final XGMSample matchingSample = findMatchingSample(sample);

        // we found a duplicate ?
        if (matchingSample != null)
        {
            final boolean sameDuration = Math.round(sample.getLength() / 60) == Math.round(matchingSample.getLength() / 60);
            // update VGM so it now uses the matching sample id
            updateSampleCommands(sample.id, matchingSample.id, sameDuration ? -1 : (sample.getLength() * 44100L) / XGMSample.XGM_FULL_RATE);
            // remove duplicate from samples
            samples.remove(sampleIndex);

            // if (Launcher.verbose)
            System.out.println("Found duplicated sample #" + sample.id + " (merged with #" + matchingSample.id + ")");
        }
    }

    private static List<XGMFMCommand> compileYMCommands(Map<Integer, List<VGMCommand>> ymChannelSetCommands, Map<Integer, Integer> ymFreqCommands,
            List<VGMCommand> ymMiscCommands, Map<Integer, VGMCommand> ymKeyCommands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        // channel set YM commands first (sorted by channel)
        for (Entry<Integer, List<VGMCommand>> entry : ymChannelSetCommands.entrySet())
            result.addAll(XGMFMCommand.createYMCHCommands(entry.getValue(), entry.getKey().intValue()));
        // then frequency YM command (sorted by channel)
        for (Entry<Integer, Integer> entry : ymFreqCommands.entrySet())
        {
            final int ch = entry.getKey().intValue();
            result.add(XGMFMCommand.createYMFreqCommand(ch & 7, (ch & 8) != 0, entry.getValue().intValue(), false, false));
        }
        // then YM misc commands
        if (!ymMiscCommands.isEmpty())
            result.addAll(XGMFMCommand.createYMMiscCommands(ymMiscCommands));
        // then key commands
        result.addAll(XGMFMCommand.createYMKeyCommands(ymKeyCommands.values()));

        // done
        ymChannelSetCommands.clear();
        ymFreqCommands.clear();
        ymMiscCommands.clear();
        ymKeyCommands.clear();

        return result;
    }

    /**
     * Find the FM LOOP Start command index
     */
    protected int getFMLoopStartCommandIndex()
    {
        for (int i = 0; i < FMcommands.size(); i++)
            if (FMcommands.get(i).isLoopStart())
                return i;

        return -1;
    }

    /**
     * Find the FM LOOP Start command
     */
    protected XGMFMCommand getFMLoopStartCommand()
    {
        final int index = getFMLoopStartCommandIndex();

        if (index != -1)
            return FMcommands.get(index);

        return null;
    }

    /**
     * Find the FM LOOP command
     */
    protected XGMFMCommand getFMLoopCommand()
    {
        for (XGMFMCommand command : FMcommands)
            if (command.isLoop())
                return command;

        return null;
    }

    // /**
    // * Return the position of the command pointed by the loop
    // */
    // protected int getFMLoopPointedCommandIndex()
    // {
    // final LoopCommand loopCommand = (LoopCommand) getFMLoopCommand();
    //
    // if (loopCommand != null)
    // return getFMCommandIndexAtOffset(loopCommand.getLoopAddr());
    //
    // return -1;
    // }
    //
    // /**
    // * Return the command pointed by the loop
    // */
    // public XGMFMCommand getFMLoopPointedCommand()
    // {
    // final int index = getFMLoopPointedCommandIndex();
    //
    // if (index != -1)
    // return FMcommands.get(index);
    //
    // return null;
    // }

    /**
     * Find the PSG LOOP Start command index
     */
    protected int getPSGLoopStartCommandIndex()
    {
        for (int i = 0; i < PSGcommands.size(); i++)
            if (PSGcommands.get(i).isLoopStart())
                return i;

        return -1;
    }

    /**
     * Find the PSG LOOP Start command
     */
    protected XGMPSGCommand getPSGLoopStartCommand()
    {
        final int index = getPSGLoopStartCommandIndex();

        if (index != -1)
            return PSGcommands.get(index);

        return null;
    }

    /**
     * Find the PSG LOOP command
     */
    protected XGMPSGCommand getPSGLoopCommand()
    {
        for (XGMPSGCommand command : PSGcommands)
            if (command.isLoop())
                return command;

        return null;
    }

    /**
     * Update time information for all command
     */
    public void updateTimes()
    {
        int time;

        time = 0;
        for (XGMFMCommand com : FMcommands)
        {
            com.time = time;

            if (pal)
                time += com.getWaitFrame() * 882;
            else
                time += com.getWaitFrame() * 735;
        }
        time = 0;
        for (XGMPSGCommand com : PSGcommands)
        {
            com.time = time;

            if (pal)
                time += com.getWaitFrame() * 882;
            else
                time += com.getWaitFrame() * 735;
        }
    }

    /**
     * Update origin offset information for all command
     */
    public void updateOffsets()
    {
        Command.computeOffsets(FMcommands, 0);
        Command.computeOffsets(PSGcommands, 0);
    }

    public int getTotalTime()
    {
        final int size = FMcommands.size();
        if (size == 0)
            return 0;

        return FMcommands.get(size - 1).time;
    }

    public int getTotalTimeInFrame()
    {
        return getTotalTime() / (pal ? 882 : 735);
    }

    public int getTotalTimeInSecond()
    {
        return getTotalTimeInFrame() / (pal ? 50 : 60);
    }

    public int getTimeFrom(VGMCommand from)
    {
        return getTotalTime() - from.time;
    }

    public int getLoopDurationInSecond()
    {
        return getLoopDurationInFrame() / (pal ? 50 : 60);
    }

    public int getLoopDurationInFrame()
    {
        final XGMFMCommand loopStartCom = getFMLoopStartCommand();
        return (loopStartCom != null) ? (getTotalTime() - loopStartCom.time) / (pal ? 882 : 735) : 0;
    }

    /**
     * Return elapsed time when specified command happen
     */
    public int getFMCommandIndexAtTime(int time)
    {
        return Command.getCommandIndexAtTime(FMcommands, time);
    }

    public int getFMCommandIndexAtOffset(int offset)
    {
        return Command.getCommandIndexAtOffset(FMcommands, offset);
    }

    public XGMFMCommand getFMCommandAtOffset(int offset)
    {
        return (XGMFMCommand) Command.getCommandAtOffset(FMcommands, offset);
    }

    /**
     * Return elapsed time when specified command happen
     */
    public XGMFMCommand getFMCommandAtTime(int time)
    {
        return (XGMFMCommand) Command.getCommandAtTime(FMcommands, time);
    }

    public XGMSample getSample(int id)
    {
        for (XGMSample sample : samples)
            if (sample.id == id)
                return sample;

        return null;
    }

    public int getSampleLen(int id)
    {
        final XGMSample sample = getSample(id);
        if (sample != null)
            return sample.getLength();

        return 0;
    }

    public XGMSample getSampleByOriginId(int id)
    {
        for (XGMSample sample : samples)
            if (sample.originId == id)
                return sample;

        return null;
    }

    public XGMSample getSampleByOriginAddress(int addr)
    {
        for (XGMSample sample : samples)
            if (sample.originAddr == addr)
                return sample;

        return null;
    }

    public List<Integer> getLoopSplittedFMMusicFrameOffsets(boolean beforeLoop) throws IOException
    {
        final List<Integer> result = new ArrayList<>();
        final int loopStartComIndex = getFMLoopStartCommandIndex();
        final int loopInd = (loopStartComIndex == -1) ? FMcommands.size() : loopStartComIndex;

        if (beforeLoop)
        {
            for (int i = 0; i < loopInd; i++)
            {
                final XGMFMCommand command = FMcommands.get(i);

                if (command.isWait(false) || command.isLoop() || command.isFrameDelay())
                    result.add(Integer.valueOf(command.getOriginOffset() + command.size));
            }
        }
        else
        {
            final int baseOffset = (loopStartComIndex != -1) ? FMcommands.get(loopStartComIndex).getOriginOffset() : 0;

            for (int i = loopInd; i < FMcommands.size(); i++)
            {
                final XGMFMCommand command = FMcommands.get(i);

                if (command.isWait(false) || command.isLoop() || command.isFrameDelay())
                    result.add(Integer.valueOf((command.getOriginOffset() + command.size) - baseOffset));
            }
        }

        return result;
    }

    public List<Integer> getLoopSplittedPSGMusicFrameOffsets(boolean beforeLoop) throws IOException
    {
        final List<Integer> result = new ArrayList<>();
        final int loopStartComIndex = getPSGLoopStartCommandIndex();
        final int loopInd = (loopStartComIndex == -1) ? PSGcommands.size() : loopStartComIndex;

        if (beforeLoop)
        {
            for (int i = 0; i < loopInd; i++)
            {
                final XGMPSGCommand command = PSGcommands.get(i);

                if (command.isWait(false) || command.isLoop())
                    result.add(Integer.valueOf(command.getOriginOffset() + command.size));
            }
        }
        else
        {
            final int baseOffset = (loopStartComIndex != -1) ? PSGcommands.get(loopStartComIndex).getOriginOffset() : 0;

            for (int i = loopInd; i < PSGcommands.size(); i++)
            {
                final XGMPSGCommand command = PSGcommands.get(i);

                if (command.isWait(false) || command.isLoop())
                    result.add(Integer.valueOf((command.getOriginOffset() + command.size) - baseOffset));
            }
        }

        return result;
    }

    public List<Integer> getFMMusicFrameOffsets() throws IOException
    {
        final List<Integer> result = new ArrayList<>();

        for (XGMFMCommand command : FMcommands)
        {
            if (command.isWait(false) || command.isLoop())
                result.add(Integer.valueOf(command.getOriginOffset() + command.size));
        }

        return result;
    }

    public List<Integer> getPSGMusicFrameOffsets() throws IOException
    {
        final List<Integer> result = new ArrayList<>();

        for (XGMPSGCommand command : PSGcommands)
        {
            if (command.isWait(false) || command.isLoop())
                result.add(Integer.valueOf(command.getOriginOffset() + command.size));
        }

        return result;
    }

    public byte[] getFMMusicDataArray() throws IOException
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        for (XGMFMCommand command : FMcommands)
            result.write(command.data);

        return result.toByteArray();
    }

    public byte[] getPSGMusicDataArray() throws IOException
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        for (XGMPSGCommand command : PSGcommands)
            result.write(command.data);

        return result.toByteArray();
    }

    public byte[] getFMMusicDataArray(boolean beforeLoop) throws IOException
    {
        final int loopStartComIndex = getFMLoopStartCommandIndex();
        final int loopInd = (loopStartComIndex == -1) ? FMcommands.size() : loopStartComIndex;
        final ByteArrayOutputStream result = new ByteArrayOutputStream();
        int ind;

        if (beforeLoop)
        {
            ind = 0;
            // build data array for no looping sequence
            while (ind < loopInd)
                result.write(FMcommands.get(ind++).data);
        }
        else
        {
            ind = loopInd;
            // build data array for looping sequence
            while (ind < FMcommands.size())
                result.write(FMcommands.get(ind++).data);
        }

        return result.toByteArray();
    }

    public byte[] getPSGMusicDataArray(boolean beforeLoop) throws IOException
    {
        final int loopStartComIndex = getPSGLoopStartCommandIndex();
        final int loopInd = (loopStartComIndex == -1) ? PSGcommands.size() : loopStartComIndex;
        final ByteArrayOutputStream result = new ByteArrayOutputStream();
        int ind;

        if (beforeLoop)
        {
            ind = 0;
            // build data array for no looping sequence
            while (ind < loopInd)
                result.write(PSGcommands.get(ind++).data);
        }
        else
        {
            ind = loopInd;
            // build data array for looping sequence
            while (ind < PSGcommands.size())
                result.write(PSGcommands.get(ind++).data);
        }

        return result.toByteArray();
    }

    public byte[] getPackedFMMusicDataArray() throws IOException
    {
        // we need to split data block on start loop position
        // so we can properly restore loop offset after packing operation
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        // pack first part of data
        result.write(XGCPacker.pack(getFMMusicDataArray(true), getLoopSplittedFMMusicFrameOffsets(true), 0));
        // do we have a loop section --> set loop offset
        if (getFMLoopStartCommandIndex() != -1)
            setFMLoopAddress(result.size());
        // -1 = no loop
        else
            setFMLoopAddress(-1);
        // pack second part of data
        result.write(XGCPacker.pack(getFMMusicDataArray(false), getLoopSplittedFMMusicFrameOffsets(false), result.size()));

        return result.toByteArray();
    }

    public byte[] getPackedPSGMusicDataArray() throws IOException
    {
        // we need to split data block on start loop position
        // so we can properly restore loop offset after packing operation
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        // pack first part of data
        result.write(XGCPacker.pack(getPSGMusicDataArray(true), getLoopSplittedPSGMusicFrameOffsets(true), 0));
        // do we have a loop section --> set loop offset
        if (getPSGLoopStartCommandIndex() != -1)
            setPSGLoopAddress(result.size());
        // -1 = no loop
        else
            setPSGLoopAddress(-1);
        // pack second part of data
        result.write(XGCPacker.pack(getPSGMusicDataArray(false), getLoopSplittedPSGMusicFrameOffsets(false), result.size()));

        return result.toByteArray();
    }

    protected int getTotalMusicDataSize()
    {
        return getFMMusicDataSize() + getPSGMusicDataSize();
    }

    protected int getFMMusicDataSize()
    {
        int result = 0;

        for (XGMFMCommand command : FMcommands)
            result += command.size;

        return result;
    }

    protected int getPSGMusicDataSize()
    {
        int result = 0;

        for (XGMPSGCommand command : PSGcommands)
            result += command.size;

        return result;
    }

    protected int getPackedFMMusicDataSize() throws IOException
    {
        return getPackedFMMusicDataArray().length;
    }

    protected int getPackedPSGMusicDataSize() throws IOException
    {
        return getPackedPSGMusicDataArray().length;
    }

    protected byte[] getPCMDataArray() throws IOException
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        for (int s = 0; s < samples.size(); s++)
        {
            byte[] copy = samples.get(s).data.clone();

            // sign the sample
            for (int i = 0; i < copy.length; i++)
                copy[i] += 0x80;

            result.write(copy);
        }

        return result.toByteArray();
    }

    protected int getPCMDataSize()
    {
        int result = 0;

        for (XGMSample sample : samples)
            result += sample.data.length;

        return result;
    }

    public byte[] asByteArray() throws IOException
    {
        int offset;
        int data;
        int len;
        final ByteArrayOutputStream result = new ByteArrayOutputStream();

        // 0000: XGM2 id (ignored when compiled in ROM)
        if (!packed)
            result.write("XGM2".getBytes());
        // 0004: version (0x10 currently)
        result.write(0x10);

        // 0005: format description (see xgm2.txt)
        data = 0;
        // bit #0: NTSC / PAL information: 0=NTSC 1=PAL
        if (pal)
            data |= 1;
        // bit #1: multi tracks file: 0=No 1=Yes (always 0 here)
        // bit #2: GD3 tags: 0=No 1=Yes
        if (gd3 != null)
            data |= 4;
        // bit #3: packed FM / PSG / GD3 data blocks: 0=No 1=Yes
        if (packed)
            data |= 8;
        // write format
        result.write(data);

        // get FM and PSG data blocks
        byte[] pcmData = getPCMDataArray();
        byte[] fmData;
        byte[] psgData;

        if (packed)
        {
            fmData = getPackedFMMusicDataArray();
            psgData = getPackedPSGMusicDataArray();
        }
        else
        {
            fmData = getFMMusicDataArray();
            psgData = getPSGMusicDataArray();
        }

        // align on 256 bytes
        pcmData = Util.align(pcmData, 256, 0);
        fmData = Util.align(fmData, 256, 0);
        psgData = Util.align(psgData, 256, 0);

        // 0006-0007: SLEN = Sample data bloc size / 256 (ex: $0200 means 512*256 = 131072 bytes)
        data = pcmData.length >> 8;
        result.write(data >> 0);
        result.write(data >> 8);
        // 0008-0009: FMLEN = FM music data block size / 256 (ex: $0040 means 64*256 = 16384 bytes)
        data = fmData.length >> 8;
        result.write(data >> 0);
        result.write(data >> 8);
        // 000A-000B: PSGLEN = PSG music data block size / 256 (ex: $0020 means 32*256 = 8192 bytes)
        data = psgData.length >> 8;
        result.write(data >> 0);
        result.write(data >> 8);

        // 000C-0103: SID (sample id) table
        // size = 256-8 = 248 bytes so end of table will align on 256 bytes in ROM
        offset = 0;
        for (int s = 0; s < samples.size(); s++)
        {
            final XGMSample sample = samples.get(s);
            len = sample.data.length;

            // each entry of the table consist of 2 bytes for the address:
            // entry+$0: sample address / 256
            result.write(offset >> 8);
            result.write(offset >> 16);
            offset += len;
        }
        // required to get last sample size
        result.write(offset >> 8);
        result.write(offset >> 16);
        // fill with silent mark
        for (int s = samples.size() + 1; s < (248 / 2); s++)
        {
            result.write(0xFF);
            result.write(0xFF);
        }

        // 0104-xx04: sample data (see SLEN field for size)
        result.write(pcmData);
        // xx04-xx04: FM music data
        result.write(fmData);
        // xx04-xx04: PSG music data
        result.write(psgData);

        // xx04-xx04: GD3/XD3 data
        if (packed)
        {
            if (xd3 != null)
                result.write(xd3.asByteArray());
        }
        else if (gd3 != null)
            result.write(gd3.asByteArray());

        return result.toByteArray();
    }

    public void updateSampleCommands(int originId, int replaceId, long durationInSample)
    {
        // nothing to do..
        if ((originId == replaceId) && (durationInSample == -1))
            return;

        int i = 0;
        while (i < FMcommands.size())
        {
            final XGMFMCommand command = FMcommands.get(i++);

            // found a command for the original sample ?
            if (command.isPCM() && (command.getPCMId() == originId))
            {
                // --> set new sample id
                command.setPCMId(replaceId);

                // we have to change duration ?
                if (durationInSample != -1)
                {
                    // get duration
                    final long duration = durationInSample * (command.getPCMHalfRate() ? 2 : 1);
                    final long endTime = command.time + duration;
                    long endTimeFrame = -1;
                    boolean addStop = false;

                    // find position where to add stop command
                    while (i < FMcommands.size())
                    {
                        final XGMFMCommand tmpCom = FMcommands.get(i++);

                        // find another PCM command in between ? no need to stop then
                        if (tmpCom.isPCM())
                        {
                            i--;
                            break;
                        }
                        // duration reach ?
                        if (tmpCom.time >= endTime)
                        {
                            if (endTimeFrame == -1) endTimeFrame = tmpCom.time;
                            // next frame ?
                            else if (tmpCom.time > endTimeFrame)
                            {
                                // need to add stop PCM command
                                addStop = true;
                                i--;
                                break;
                            }
                        }
                    }

                    if (addStop)
                    {
                        FMcommands.add(i - 1, XGMFMCommand.createPCMStopCommand(command.getPCMChannel()));
                        i++;
                    }
                }
            }
        }
    }
}
