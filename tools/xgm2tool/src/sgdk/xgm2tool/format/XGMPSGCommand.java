package sgdk.xgm2tool.format;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.tool.Util;

public class XGMPSGCommand extends Command
{
    public static List<XGMPSGCommand> filterChannel(List<XGMPSGCommand> commands, int channel, boolean getWait, boolean getLoopStart)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();

        for (XGMPSGCommand com : commands)
            if ((com.getChannel() == channel) || (getWait && com.isWait(true)) || (getLoopStart && com.isLoopStart()))
                result.add(com);

        return result;
    }

    public static List<XGMPSGCommand> filterEnv(List<XGMPSGCommand> commands)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();

        for (XGMPSGCommand com : commands)
            if (com.isEnv())
                result.add(com);

        return result;
    }

    public static List<XGMPSGCommand> filterFreq(List<XGMPSGCommand> commands)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();

        for (XGMPSGCommand com : commands)
            if (com.isFreq())
                result.add(com);

        return result;
    }

    public static boolean hasWaitCommand(List<XGMPSGCommand> newPSGCommands)
    {
        for (XGMPSGCommand com : newPSGCommands)
            if (com.getWaitFrame() > 0)
                return true;

        return false;
    }

    public final static int WAIT_SHORT = 0x00;
    public final static int WAIT_LONG = 0x0E;
    public final static int LOOP = 0x0F;

    public final static int FREQ_LOW = 0x10;

    public final static int FREQ = 0x20;
    public final static int FREQ_WAIT = 0x30;

    public final static int FREQ0_DELTA = 0x40;
    public final static int FREQ1_DELTA = 0x50;
    public final static int FREQ2_DELTA = 0x60;
    public final static int FREQ3_DELTA = 0x70;

    public final static int ENV0 = 0x80;
    public final static int ENV1 = 0x90;
    public final static int ENV2 = 0xA0;
    public final static int ENV3 = 0xB0;

    public final static int ENV0_DELTA = 0xC0;
    public final static int ENV1_DELTA = 0xD0;
    public final static int ENV2_DELTA = 0xE0;
    public final static int ENV3_DELTA = 0xF0;

    public final static int DUMMY = 0xFF;
    public final static int LOOP_START = 0xFE;

    static int computeSize(byte[] data, int offset)
    {
        final int command = Util.getInt8(data, offset);

        switch (command)
        {
            case WAIT_LONG:
                return 2;
            case LOOP:
                return 4;
        }

        switch (command & 0xF0)
        {
            default:
            case WAIT_SHORT:
                return 1;

            case FREQ:
            case FREQ_WAIT:
            case FREQ_LOW:
                return 2;

            case FREQ0_DELTA:
            case FREQ1_DELTA:
            case FREQ2_DELTA:
            case FREQ3_DELTA:
            case ENV0:
            case ENV1:
            case ENV2:
            case ENV3:
            case ENV0_DELTA:
            case ENV1_DELTA:
            case ENV2_DELTA:
            case ENV3_DELTA:
                return 1;
        }
    }

    boolean dummy;

    public XGMPSGCommand(byte[] data, int offset)
    {
        this(Arrays.copyOfRange(data, offset, offset + computeSize(data, offset)));
    }

    private XGMPSGCommand(byte[] data)
    {
        super(data);

        size = data.length;
        dummy = false;
    }

    private XGMPSGCommand(int command)
    {
        super(command);

        size = data.length;
        dummy = false;
    }

    public int getType()
    {
        if (this instanceof LoopStartCommand)
            return LOOP_START;

        final int com = getCommand();

        if (isDummy())
            return DUMMY;

        if (com == WAIT_LONG)
            return WAIT_LONG;
        if (com == LOOP)
            return LOOP;

        return com & 0xF0;
    }

    public boolean isWaitShort()
    {
        return (getType() == WAIT_SHORT);
    }

    public boolean isWaitLong()
    {
        return (getType() == WAIT_LONG);
    }

    public boolean isWait(boolean realWaitOnly)
    {
        if (realWaitOnly)
            return isWaitShort() || isWaitLong();

        return isWaitShort() || isWaitLong() || isFreqWait() || isFreqLowWait() || isFreqDeltaWait() || isEnvDeltaWait();
    }

    public int getWaitFrame()
    {
        if (isWaitLong())
            return Util.getInt8(data, 1) + 15;
        if (isWaitShort())
            return (Util.getInt8(data, 0) & 0xF) + 1;
        if (isWait(false))
            return 1;

        return 0;
    }

    public boolean isLoopStart()
    {
        return this instanceof LoopStartCommand;
    }

    public boolean isLoop()
    {
        return getType() == LOOP;
    }

    public boolean isFreqWait()
    {
        return (getType() == FREQ_WAIT);
    }

    public boolean isFreqNoWait()
    {
        return (getType() == FREQ);
    }

    public boolean isFreq()
    {
        return isFreqNoWait() || isFreqWait();
    }

    public boolean isFreqLow()
    {
        return (getType() == FREQ_LOW);
    }

    public boolean isFreqLowNoWait()
    {
        return isFreqLow() && ((data[0] & 1) == 0);
    }

    public boolean isFreqLowWait()
    {
        return isFreqLow() && ((data[0] & 1) != 0);
    }

    public boolean isFreqDelta()
    {
        return ((getType() == FREQ0_DELTA) || (getType() == FREQ1_DELTA) || (getType() == FREQ2_DELTA));
    }

    public boolean isFreqDeltaNoWait()
    {
        return isFreqDelta() && ((data[0] & 8) == 0);
    }

    public boolean isFreqDeltaWait()
    {
        return isFreqDelta() && ((data[0] & 8) != 0);
    }

    public boolean isEnv()
    {
        return (getType() == ENV0) || (getType() == ENV1) || (getType() == ENV2) || (getType() == ENV3);
    }

    public boolean isEnvDelta()
    {
        return (getType() == ENV0_DELTA) || (getType() == ENV1_DELTA) || (getType() == ENV2_DELTA) || (getType() == ENV3_DELTA);
    }

    public boolean isEnvDeltaNoWait()
    {
        return isEnvDelta() && ((data[0] & 8) == 0);
    }

    public boolean isEnvDeltaWait()
    {
        return isEnvDelta() && ((data[0] & 8) != 0);
    }

    public boolean supportWait()
    {
        // can this command be muted in com + wait ?
        return isEnvDelta() || isFreq() || isFreqLow() || isFreqDelta();
    }

    public boolean addWait()
    {
        if (supportWait())
        {
            if (isEnvDeltaNoWait())
                data[0] |= 8;
            else if (isFreqNoWait())
                data[0] += (FREQ_WAIT - FREQ);
            else if (isFreqLowNoWait())
                data[0] |= 1;
            else if (isFreqDeltaNoWait())
                data[0] |= 8;
            else
                return false;
        }

        return true;
    }

    @Override
    public int getChannel()
    {
        switch (getType())
        {
            case ENV0:
            case ENV0_DELTA:
            case FREQ0_DELTA:
                return 0;
            case ENV1:
            case ENV1_DELTA:
            case FREQ1_DELTA:
                return 1;
            case ENV2:
            case ENV2_DELTA:
            case FREQ2_DELTA:
                return 2;
            case ENV3:
            case ENV3_DELTA:
            case FREQ3_DELTA:
                return 3;
        }

        if (isFreq())
            return ((Util.getInt8(data, 0) >> 2) & 3);
        if (isFreqLow())
            return ((Util.getInt8(data, 1) >> 5) & 3);

        return -1;
    }

    public int getLoopAddr()
    {
        if (isLoop())
            return Util.getInt24(data, 1);

        return -1;
    }

    public void setLoopAddr(int address)
    {
        if (isLoop())
            Util.setInt24(data, 1, address);
    }

//    public void setFreq(int freq)
//    {
//        if (isFreq())
//        {
//            data[0] = (byte) ((data[0] & 0xF0) | (freq & 0x0F));
//            data[1] = (byte) ((data[1] & 0xC0) | ((freq >> 4) & 0x3F));
//        }
//    }
    
    public void setFreq(int freq)
    {
        if (isFreq())
        {
            data[0] = (byte) ((data[0] & 0xFC) | ((freq >> 8) & 0x3));
            data[1] = (byte) (freq & 0xFF);
        }
    }

//    public int getFreq()
//    {
//        if (isFreq())
//            return ((data[1] & 0x3F) << 4) | ((data[0] & 0x0F) << 0);
//
//        return -1;
//    }
    
    public int getFreq()
    {
        if (isFreq())
            return ((data[0] & 0x3) << 8) | (data[1] & 0xFF);

        return -1;
    }

    public void setFreqLow(int freqLow)
    {
        if (isFreqLow())
            data[1] = (byte) ((data[1] & 0xF0) | (freqLow & 0x0F));
    }

    public int getFreqLow()
    {
        if (isFreqLow())
            return (data[1] & 0xF);

        return -1;
    }

    public void setFreqDelta(int delta)
    {
        if (isFreqDelta())
            data[0] = (byte) ((data[0] & 0xF8) | ((delta < 0) ? (4 | -delta) : delta));
    }

    public int getFreqDelta()
    {
        if (isFreqDelta())
        {
            final int delta = ((data[0] >> 0) & 3) + 1;
            return ((data[0] & 4) != 0) ? -delta : delta;
        }

        return 0;
    }

    public void toFreqDelta(int delta)
    {
        // only for freq or freq low command
        if (isFreq() || isFreqLow())
        {
            final int deltav = (delta < 0) ? -(delta + 1) : (delta - 1);
            final int v = ((isFreqWait() || isFreqLowWait()) ? 8 : 0) | ((delta < 0) ? 4 : 0) | deltav;

            switch (getChannel())
            {
                case 0:
                    data = new byte[] {(byte) (FREQ0_DELTA | v)};
                    size = 1;
                    break;
                case 1:
                    data = new byte[] {(byte) (FREQ1_DELTA | v)};
                    size = 1;
                    break;
                case 2:
                    data = new byte[] {(byte) (FREQ2_DELTA | v)};
                    size = 1;
                    break;
                case 3:
                    data = new byte[] {(byte) (FREQ3_DELTA | v)};
                    size = 1;
                    break;
            }
        }
    }
    
    public void toFreqLow(int freqLow)
    {
        // only for freq command
        if (isFreq())
        {
            final boolean wait = isFreqWait();
            final int ch = getChannel();

            // same size (2) so we don't need to re-allocate data
            data[0] = (byte) (FREQ_LOW | (wait?1:0));
            data[1] = (byte) (0x80 | (ch << 5) | freqLow);

        }
    }


    public int getEnv()
    {
        if (isEnv())
            return Util.getInt8(data, 0) & 0xF;

        return -1;
    }

    public int getEnvDelta()
    {
        if (isEnvDelta())
        {
            final int delta = ((data[0] >> 0) & 3) + 1;
            return ((data[0] & 4) != 0) ? -delta : delta;
        }

        return 0;
    }

    public void toEnvDelta(int delta)
    {
        // only for env command
        if (isEnv())
        {
            final int deltav = (delta < 0) ? -(delta + 1) : (delta - 1);
            final int v = ((delta < 0) ? 4 : 0) | deltav;

            switch (getChannel())
            {
                case 0:
                    data = new byte[] {(byte) (ENV0_DELTA | v)};
                    size = 1;
                    break;
                case 1:
                    data = new byte[] {(byte) (ENV1_DELTA | v)};
                    size = 1;
                    break;
                case 2:
                    data = new byte[] {(byte) (ENV2_DELTA | v)};
                    size = 1;
                    break;
                case 3:
                    data = new byte[] {(byte) (ENV3_DELTA | v)};
                    size = 1;
                    break;
            }
        }
    }

    public boolean isDummy()
    {
        return dummy;
    }

    public void clearDummy()
    {
        dummy = false;
    }

    public void setDummy()
    {
        dummy = true;
    }

    @Override
    public String toString()
    {
        String result = "";

        if (isDummy())
            result += "PSG DUMMY - " + new XGMPSGCommand(data).toString();
        else if (isWaitShort())
            result += "PSG WAIT S #" + getWaitFrame();
        else if (isWaitLong())
            result += "PSG WAIT L #" + getWaitFrame();
        else if (isEnv())
            result += "PSG ENV #" + StringUtil.toHexaString(getEnv(), 1);
        else if (isEnvDeltaNoWait())
            result += "PSG ENVD " + getEnvDelta();
        else if (isEnvDeltaWait())
            result += "PSG ENVD " + getEnvDelta() + " W";
        else if (isFreqNoWait())
            result += "PSG FREQ #" + StringUtil.toHexaString(getFreq(), 3);
        else if (isFreqWait())
            result += "PSG FREQ W #" + StringUtil.toHexaString(getFreq(), 3);
        else if (isFreqLowNoWait())
            result += "PSG FREQL #" + StringUtil.toHexaString(getFreqLow(), 1);
        else if (isFreqLowWait())
            result += "PSG FREQL W #" + StringUtil.toHexaString(getFreqLow(), 1);
        else if (isFreqDeltaNoWait())
            result += "PSG FREQD " + getFreqDelta();
        else if (isFreqDeltaWait())
            result += "PSG FREQD W " + getFreqDelta();
        else if (isLoopStart())
            result += "PSG LOOP St";
        else if (isLoop())
            result += "PSG LOOP #" + StringUtil.toHexaString(getLoopAddr(), 6);

        return result;
    }

    public static List<XGMPSGCommand> createWaitCommands(int wait)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();
        int remain = wait;

        while (remain > 270)
        {
            result.add(new WaitLongCommand(270));
            remain -= 270;
        }
        if (remain > 14)
            result.add(new WaitLongCommand(remain));
        else
            result.add(new WaitShortCommand(remain));

        return result;
    }

    public static List<XGMPSGCommand> createPSGCommands(List<VGMCommand> commands)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();
        final List<VGMCommand> remaining = new ArrayList<>(commands);
        final List<VGMCommand> toneCommands = new ArrayList<>();
        final List<VGMCommand> toRemove = new ArrayList<>();

        if (commands.isEmpty())
            return result;

        VGMCommand lowByteCom = null;
        // build complete tone commands
        for (VGMCommand com : commands)
        {
            // data write ?
            if (com.isPSGHighByteWrite())
            {
                if (lowByteCom != null)
                {
                    // previous command was low
                    if (lowByteCom.isPSGToneLowWrite())
                    {
                        // add complete set tone commands
                        toneCommands.add(lowByteCom);
                        toneCommands.add(com);
                    }
                    else
                    {
                        // overwrite previous env command data and remove current
                        lowByteCom.data[1] = (byte) ((lowByteCom.data[1] & 0xF0) | (com.getPSGValue() & 0x0F));
                        // remove current
                        toRemove.add(com);
                    }
                }
            }
            else
                lowByteCom = com;
        }

        // remove complete tone commands from remaining
        remaining.removeAll(toneCommands);
        remaining.removeAll(toRemove);

        // add complete tone commands
        result.addAll(createPSGToneCommands(toneCommands));

        // then add others command
        for (VGMCommand com : remaining)
            // should always be the case..
            if (com.isPSGLowByteWrite())
                result.add(createPSGByteCommand(com));

        return result;
    }

    public static List<XGMPSGCommand> createPSGToneCommands(List<VGMCommand> commands)
    {
        final List<XGMPSGCommand> result = new ArrayList<>();

        for (int i = 0; i < commands.size(); i += 2)
            result.add(createPSGToneCommand(commands.get(i + 0), commands.get(i + 1)));

        return result;
    }

    private static XGMPSGCommand createPSGByteCommand(VGMCommand command)
    {
        if (command.isPSGEnvWrite())
        {
            byte data;

            switch (command.getPSGChannel())
            {
                default:
                case 0:
                    data = (byte) ENV0;
                    break;
                case 1:
                    data = (byte) ENV1;
                    break;
                case 2:
                    data = (byte) ENV2;
                    break;
                case 3:
                    data = (byte) ENV3;
                    break;
            }

            // set value
            data |= (command.getPSGValue() & 0xF);

            // create PSG command
            return new XGMPSGCommand(new byte[] {data});
        }
        else if (command.isPSGToneLowWrite())
        {
            return new XGMPSGCommand(new byte[] {(byte) FREQ_LOW, (byte) (0x80 | (command.getPSGChannel() << 5) | (command.getPSGValue() & 0xF))});
        }
        else
        {
            System.err.println("Invalide PSG byte command: " + StringUtil.toHexaString(command.getPSGValue(), 2));
            return new XGMPSGCommand(new byte[] {0});
        }
    }

    private static XGMPSGCommand createPSGToneCommand(VGMCommand vgmCommandLow, VGMCommand vgmCommandHigh)
    {
        final byte[] data = new byte[2];

        // set command
        data[0] = FREQ;
        
//        // set low freq
//        data[0] |= vgmCommandLow.getPSGValue() & 0x0F;
//        // set value
//        data[1] = (byte) (vgmCommandHigh.getPSGValue() & 0x3F);
//        // set channel
//        data[1] |= (vgmCommandLow.getPSGChannel() & 3) << 6;

        // set channel
        data[0] |= vgmCommandLow.getPSGChannel() << 2;
        // set value
        data[0] |= (vgmCommandHigh.getPSGValue() >> 4) & 3;
        data[1] = (byte) ((vgmCommandHigh.getPSGValue() & 0xF) << 4);
        data[1] |= vgmCommandLow.getPSGValue() & 0xF;
        

        return new XGMPSGCommand(data);
    }

    public static class WaitShortCommand extends XGMPSGCommand
    {
        public WaitShortCommand(int wait)
        {
            super(new byte[] {(byte) (WAIT_SHORT | ((wait - 1) & 0xF))});
        }
    }

    public static class WaitLongCommand extends XGMPSGCommand
    {
        public WaitLongCommand(int wait)
        {
            super(new byte[] {(byte) WAIT_LONG, (byte) (wait - 15)});
        }
    }

    public static class FrameCommand extends XGMPSGCommand
    {
        public FrameCommand()
        {
            super(new byte[] {(byte) WAIT_SHORT});
        }
    }

    public static class LoopCommand extends XGMPSGCommand
    {
        public LoopCommand(int offset)
        {
            super(new byte[] {(byte) LOOP, (byte) (offset >> 0), (byte) (offset >> 8), (byte) (offset >> 16)});
        }
    }

    public static class LoopStartCommand extends XGMPSGCommand
    {
        public LoopStartCommand()
        {
            super(new byte[] {});
        }
    }

    public static class EndCommand extends LoopCommand
    {
        public EndCommand()
        {
            super(-1);
        }
    }
}
