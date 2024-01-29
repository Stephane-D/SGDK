/**
 * 
 */
package sgdk.xgm2tool.format;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.tool.Util;

/**
 * VGM command descriptor class
 * 
 * @author Stephane Dallongeville
 */
public class VGMCommand extends Command
{
    public final static int DATA_BLOCK = 0x67;
    public final static int END = 0x66;
    public final static int SEEK = 0xE0;

    public final static int WRITE_SN76489 = 0x50;
    public final static int WRITE_YM2612_PORT0 = 0x52;
    public final static int WRITE_YM2612_PORT1 = 0x53;

    public final static int WAIT_NTSC_FRAME = 0x62;
    public final static int WAIT_PAL_FRAME = 0x63;

    public final static int STREAM_CONTROL = 0x90;
    public final static int STREAM_DATA = 0x91;
    public final static int STREAM_FREQUENCY = 0x92;
    public final static int STREAM_START_LONG = 0x93;
    public final static int STREAM_START = 0x95;
    public final static int STREAM_STOP = 0x94;

    public final static int LOOP_START = 0x30;
    public final static int LOOP_END = 0x31;

    public static int computeSize(byte[] data, int offset)
    {
        if (data.length == 0)
            return 0;

        final int command = Util.getInt8(data, offset);

        switch (command)
        {
            case 0x4F:
            case WRITE_SN76489:
                return 2;

            case 0x51:
            case WRITE_YM2612_PORT0:
            case WRITE_YM2612_PORT1:
            case 0x54:
            case 0x55:
            case 0x56:
            case 0x57:
            case 0x58:
            case 0x59:
            case 0x5A:
            case 0x5B:
            case 0x5C:
            case 0x5D:
            case 0x5E:
            case 0x5F:
                return 3;

            case 0x61:
                return 3;

            case WAIT_NTSC_FRAME:
            case WAIT_PAL_FRAME:
                return 1;

            case 0x66:
                return 1;

            case DATA_BLOCK:
                // data block start
                return 7 + Util.getInt32(data, offset + 0x03);

            case 0x68:
                // write data block start
                return 12 + Util.getInt24(data, offset + 0x09);

            case STREAM_CONTROL:
                return 5;
            case STREAM_DATA:
                return 5;
            case STREAM_FREQUENCY:
                return 6;
            case STREAM_START_LONG:
                return 11;
            case STREAM_STOP:
                return 2;
            case STREAM_START:
                return 5;
        }

        switch (command >> 4)
        {
            default:
                return 1;
            case 0x3:
                return 2;
            case 0x4:
                return 3;
            case 0x7:
                return 1;
            case 0x8:
                return 1;
            case 0xA:
                return 3;
            case 0xB:
                return 3;
            case 0xC:
                return 4;
            case 0xD:
                return 4;
            case 0xE:
                return 5;
            case 0xF:
                return 5;
        }
    }

    private static int lastPSGChannel = 0;
    
    public VGMCommand(byte[] data, int offset)
    {
        super(data);

        // compute size
        size = computeSize(data, offset);
        // then fix data array to not have any offset
        this.data = Arrays.copyOfRange(data, offset, offset + size);
    }

    public VGMCommand(byte[] data)
    {
        this(data, 0);

        // compute size
        size = data.length;
    }

    public VGMCommand(int command)
    {
        this(new byte[] {(byte) command});
    }

    public int getCommand()
    {
        if (this instanceof LoopStartCommand)
            return LOOP_START;

        return Util.getInt8(data, 0);
    }

    public boolean isDataBlock()
    {
        return getCommand() == DATA_BLOCK;
    }

    public int getDataBankId()
    {
        return Util.getInt8(data, 2);
    }

    public int getDataBlockLen()
    {
        return Util.getInt32(data, 3);
    }

    public boolean isSeek()
    {
        return getCommand() == SEEK;
    }

    public int getSeekAddress()
    {
        if (isSeek())
            return Util.getInt32(data, 0x01);

        return -1;
    }

    public boolean isEnd()
    {
        return getCommand() == END;
    }

    public boolean isLoopStart()
    {
        return this instanceof LoopStartCommand;
    }

    public boolean isPCM()
    {
        return (getCommand() & 0xF0) == 0x80;
    }

    public boolean isWait()
    {
        if (isShortWait())
            return true;
        return ((getCommand() >= 0x61) && (getCommand() <= 0x63));
    }

    public boolean isShortWait()
    {
        return (getCommand() & 0xF0) == 0x70;
    }

    public int getWaitValue()
    {
        if (isShortWait())
            return (getCommand() & 0x0F) + 1;
        if (isPCM())
            return getCommand() & 0x0F;

        switch (getCommand())
        {
            case 0x61:
                return Util.getInt16(data, 1);
            case WAIT_NTSC_FRAME:
                return 0x2DF;
            case WAIT_PAL_FRAME:
                return 0x372;
        }

        return 0;
    }

    public int computeSize()
    {
        return computeSize(data, 0);
    }

    public boolean isPSGWrite()
    {
        return (getCommand() == WRITE_SN76489);
    }

    public boolean isPSGLowByteWrite()
    {
        return isPSGWrite() && ((getPSGValue() & 0x80) == 0x80);
    }

    public boolean isPSGHighByteWrite()
    {
        return isPSGWrite() && ((getPSGValue() & 0x80) == 0x00);
    }

    public boolean isPSGEnvWrite()
    {
        return isPSGLowByteWrite() && ((getPSGValue() & 0x10) == 0x10);
    }

    public boolean isPSGToneWrite()
    {
        // ok to assume that as we force PSG env write to use low byte write format
        return isPSGWrite() && !isPSGEnvWrite();
    }

    public boolean isPSGToneLowWrite()
    {
        return isPSGToneWrite() && isPSGLowByteWrite();
    }

    public boolean isPSGToneHighWrite()
    {
        return isPSGToneWrite() && isPSGHighByteWrite();
    }

    public int getPSGValue()
    {
        if (isPSGWrite())
            return Util.getInt8(data, 1);

        return -1;
    }

    public int getPSGChannel()
    {
        if (isPSGWrite())
        {
            if (isPSGLowByteWrite())
                lastPSGChannel = (getPSGValue() >> 5) & 3;
            
            return lastPSGChannel;
        }

        return -1;
    }

    public int getPSGFrequence()
    {
        if (isPSGToneHighWrite())
            return ((getPSGValue() & 0x3F) << 4);
        if (isPSGToneLowWrite())
            return (getPSGValue() & 0xF);

        return -1;
    }

    public int getPSGEnv()
    {
        if (isPSGEnvWrite())
            return (getPSGValue() & 0xF);

        return -1;
    }

    public boolean isYM2612Port0Write()
    {
        return (getCommand() == WRITE_YM2612_PORT0);
    }

    public boolean isYM2612Port1Write()
    {
        return (getCommand() == WRITE_YM2612_PORT1);
    }

    public boolean isYM2612Write()
    {
        return isYM2612Port0Write() || isYM2612Port1Write();
    }

    public int getYM2612Port()
    {
        if (isYM2612Port0Write())
            return 0;
        if (isYM2612Port1Write())
            return 1;

        return -1;
    }

    public int getYM2612Register()
    {
        if (isYM2612Write())
            return Util.getInt8(data, 1);

        return -1;
    }

    public int getYM2612Value()
    {
        if (isYM2612Write())
            return Util.getInt8(data, 2);

        return -1;
    }

    public boolean isYM2612KeyWrite()
    {
        return isYM2612Port0Write() && (getYM2612Register() == 0x28);
    }

    public boolean isYM2612KeyOnWrite()
    {
        return isYM2612KeyWrite() && ((getYM2612Value() & 0xF0) == 0xF0);
    }

    public boolean isYM2612KeyOffWrite()
    {
        return isYM2612KeyWrite() && ((getYM2612Value() & 0xF0) == 0x00);
    }

    // public int getYM2612KeyChannel()
    // {
    // if (isYM2612KeyWrite())
    // return getYM2612Value() & 0x7;
    //
    // return 0;
    // }

    public boolean isYM26120x2XWrite()
    {
        return isYM2612Port0Write() && ((getYM2612Register() & 0xF0) == 0x20);
    }

    public boolean isYM2612DACOn()
    {
        return isYM2612Port0Write() && ((getYM2612Register() & 0xFF) == 0x2B) && ((getYM2612Value() & 0x80) != 0);
    }

    public boolean isYM2612DACOff()
    {
        return isYM2612Port0Write() && ((getYM2612Register() & 0xFF) == 0x2B) && ((getYM2612Value() & 0x80) == 0);
    }

    public boolean isYM2612DACWrite()
    {
        return isYM2612Port0Write() && ((getYM2612Register() & 0xFF) == 0x2A);
    }

    public boolean isYM2612CanIgnoreWrite()
    {
        if (isYM2612Port0Write())
            return (getYM2612Register() != 0x28) && (getYM2612Register() != 0x22) && (getYM2612Register() < 0x30);
        else if (isYM2612Port1Write())
            return (getYM2612Register() < 0x30);

        return false;
    }

    public boolean isYM2612TimersWrite()
    {
        return isYM2612Port0Write() && (getYM2612Register() == 0x27);
    }

    public boolean isYM2612TimersNoSpecialNoCSMWrite()
    {
        return isYM2612TimersWrite() && ((getYM2612Value() & 0xC0) == 0x00);
    }

    public boolean isYM2612GeneralWrite()
    {
        return (isYM2612Port0Write() || isYM2612Port1Write()) && (getYM2612Register() < 0x30);
    }

    public boolean isYM2612ChannelSet()
    {
        return (isYM2612Port0Write() || isYM2612Port1Write()) && (getYM2612Register() >= 0x30);
    }

    public boolean isYM2612FreqWrite()
    {
        final int port = getYM2612Port();

        if (port != -1)
        {
            final int reg = getYM2612Register();
            return ((reg >= 0xA0) && (reg < 0xB0));
        }

        return false;
    }

    public int getYM2612Channel()
    {
        final int port = getYM2612Port();

        if (port != -1)
        {
            final int reg = getYM2612Register();
            int ch;

            // special key write command ?
            if ((port == 0) && (reg == 0x28))
            {
                final int value = getYM2612Value();

                ch = value & 3;

                if (ch == 3)
                    return -1;

                ch += ((value & 4) != 0) ? 3 : 0;

                return ch;
            }
            // special case: channel 3 in special mode
            else if ((reg >= 0xA8) && (reg < 0xB0))
                ch = 2;
            // classic channel decoding
            else if ((reg >= 0x30) && (reg < 0xB8))
                ch = reg & 3;
            else
                return -1;

            if (ch != 3)
                return ch + (port * 3);
        }

        return -1;
    }

    @Override
    public int getChannel()
    {
        if (isYM2612Write())
            return getYM2612Channel();
        if (isPSGWrite())
            return getPSGChannel();

        return -1;
    }

    public int getYM2612PortChannel()
    {
        final int ch = getYM2612Channel();

        if (ch == -1)
            return -1;
        if (ch >= 3)
            return ch - 3;
        return ch;
    }

    public boolean isStream()
    {
        return isStreamControl() || isStreamData() || isStreamFrequency() || isStreamStart() || isStreamStartLong() || isStreamStop();
    }

    public boolean isStreamControl()
    {
        return (getCommand() == STREAM_CONTROL);
    }

    public boolean isStreamData()
    {
        return (getCommand() == STREAM_DATA);
    }

    public boolean isStreamFrequency()
    {
        return (getCommand() == STREAM_FREQUENCY);
    }

    public boolean isStreamStart()
    {
        return (getCommand() == STREAM_START);
    }

    public boolean isStreamStartLong()
    {
        return (getCommand() == STREAM_START_LONG);
    }

    public boolean isStreamStop()
    {
        return (getCommand() == STREAM_STOP);
    }

    public int getStreamId()
    {
        if (isStream())
            return Util.getInt8(data, 1);

        return -1;
    }

    public int getStreamBankId()
    {
        if (isStreamData())
            return Util.getInt8(data, 2);

        return -1;
    }

    public int getStreamBlockId()
    {
        if (isStreamStart())
            return Util.getInt8(data, 2);

        return -1;
    }

    public void setStreamBlockId(int value)
    {
        if (isStreamStart())
            Util.setInt8(data, 2, value);
    }

    public int getStreamFrenquency()
    {
        if (isStreamFrequency())
            return Util.getInt32(data, 2);

        return -1;
    }

    public int getStreamSampleAddress()
    {
        if (isStreamStartLong())
            return Util.getInt32(data, 2);

        return -1;
    }

    public void setStreamSampleAddress(int value)
    {
        if (isStreamStartLong())
            Util.setInt32(data, 2, value);
    }

    public int getStreamSampleSize()
    {
        if (isStreamStartLong())
            return Util.getInt32(data, 7);

        return -1;
    }

    public String getCommandDesc()
    {
        if (isStreamControl())
            return "Stream ctrl";
        if (isStreamData())
            return "Stream data";
        if (isStreamFrequency())
            return "Stream freq";
        if (isStreamStart())
            return "Stream start sh";
        if (isStreamStartLong())
            return "Stream start";
        if (isStreamStop())
            return "Stream stop";

        if (isDataBlock())
            return "Data block";
        if (isPCM())
            return "PCM";
        if (isSeek())
            return "PCM seek";
        if (isLoopStart())
            return "Loop start";
        if (isEnd())
            return "End";

        if (isPSGEnvWrite())
            return "PSG env #" + StringUtil.toHexaString(getPSGEnv(), 1);
        if (isPSGToneHighWrite())
            return "PSG tone high #" + StringUtil.toHexaString(getPSGFrequence(), 3);
        if (isPSGToneLowWrite())
            return "PSG tone low #" + StringUtil.toHexaString(getPSGFrequence(), 1);

        if (isShortWait())
            return "Short wait";
        if (isWait())
            return "Wait";

        if (isYM2612Write())
            return getYMCommandDesc(getYM2612Port(), getYM2612Register(), getYM2612Value());

        return "Others";
    }

    public static VGMCommand getKeyCommand(List<VGMCommand> commands, int channel)
    {
        for (VGMCommand command : commands)
            if (command.isYM2612KeyWrite() && (command.getYM2612Channel() == channel))
                return command;

        return null;
    }

    public static VGMCommand getKeyOffCommand(List<VGMCommand> commands, int channel)
    {
        for (VGMCommand command : commands)
            if (command.isYM2612KeyOffWrite() && (command.getYM2612Channel() == channel))
                return command;

        return null;
    }

    public static VGMCommand createYMCommand(int port, int reg, int value)
    {
        if (port == 0)
            return new VGMCommand(new byte[] {WRITE_YM2612_PORT0, (byte) reg, (byte) value});

        return new VGMCommand(new byte[] {WRITE_YM2612_PORT1, (byte) reg, (byte) value});
    }

    public static List<VGMCommand> createYMCommands(int port, int baseReg, int value)
    {
        final List<VGMCommand> result = new ArrayList<>();

        for (int ch = 0; ch < 3; ch++)
            for (int operator = 0; operator < 4; operator++)
                result.add(createYMCommand(port, baseReg + ((operator & 3) << 2) + (ch & 3), value));

        return result;
    }

    public static List<VGMCommand> createWaitCommands(int wait)
    {
        final List<VGMCommand> result = new ArrayList<>();

        if (wait == 0x2DF)
            result.add(new VGMCommand(WAIT_NTSC_FRAME));
        else if (wait == 0x372)
            result.add(new VGMCommand(WAIT_PAL_FRAME));
        else
        {
            int remaining = wait;
            while (remaining > 0)
            {
                final byte[] data = new byte[3];
                final int w = Math.min(65535, remaining);

                data[0] = 0x61;
                Util.setInt16(data, 1, w);

                result.add(new VGMCommand(data));

                remaining -= w;
            }
        }

        return result;
    }

    @Override
    public String toString()
    {
        return getCommandDesc();
    }

    public static class LoopStartCommand extends VGMCommand
    {
        public LoopStartCommand()
        {
            super(new byte[] {});
        }
    }

    public static class ComChannelSorter implements Comparator<VGMCommand>
    {
        @Override
        public int compare(VGMCommand c1, VGMCommand c2)
        {
            return Integer.compare(c1.getYM2612Channel(), c2.getYM2612Channel());
        }
    }

    public static class ComChannelRegSorter implements Comparator<VGMCommand>
    {
        @Override
        public int compare(VGMCommand c1, VGMCommand c2)
        {
            int result = Integer.compare(c1.getYM2612Channel(), c2.getYM2612Channel());
            if (result == 0)
            {
                // freq registers ? high then low
                if (((c1.getYM2612Register() & 0xF4) == 0xA4) && (((c2.getYM2612Register() & 0xF4) == 0xA0)))
                    return -1;
                if (((c1.getYM2612Register() & 0xF4) == 0xA0) && (((c2.getYM2612Register() & 0xF4) == 0xA4)))
                    return 1;

                result = Integer.compare(c1.getYM2612Register(), c2.getYM2612Register());
            }
            return result;
        }
    }

    public static class LevelOFFCommLevelONSorter implements Comparator<VGMCommand>
    {
        @Override
        public int compare(VGMCommand c1, VGMCommand c2)
        {
            final int reg1 = c1.getYM2612Register();
            final int reg2 = c2.getYM2612Register();
            final int val1 = c1.getYM2612Value();
            final int val2 = c2.getYM2612Value();

            // TL register ?
            if ((reg1 & 0XF0) == 0x40)
            {
                // TL OFF ? --> should be first
                if ((val1 & 0x7F) == 0x7F)
                    return -1;
                // TL ON ? --> should be last
                return 1;
            }
            // SL (D1L) register ?
            if ((reg1 & 0XF0) == 0x80)
            {
                // SL OFF ? --> should be first
                if ((val1 & 0xF0) == 0xF0)
                    return -1;
                // SL ON ? --> should be last
                return 1;
            }

            // TL register ?
            if ((reg2 & 0XF0) == 0x40)
            {
                // TL OFF ? --> should be first
                if ((val2 & 0x7F) == 0x7F)
                    return 1;
                // TL ON ? --> should be last
                return -1;
            }
            // SL (D1L) register ?
            if ((reg2 & 0XF0) == 0x80)
            {
                // SL OFF ? --> should be first
                if ((val2 & 0xF0) == 0xF0)
                    return 1;
                // SL ON ? --> should be last
                return -1;
            }

            // other command --> we don't care order
            return 0;
        }
    }

    public static ComChannelSorter channelSorter = new ComChannelSorter();
    public static ComChannelRegSorter channelRegSorter = new ComChannelRegSorter();
    public static LevelOFFCommLevelONSorter normalComSorter = new LevelOFFCommLevelONSorter();
}
