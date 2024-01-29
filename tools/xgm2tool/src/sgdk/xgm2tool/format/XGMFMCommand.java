package sgdk.xgm2tool.format;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.struct.YM2612State;
import sgdk.xgm2tool.tool.Util;

public class XGMFMCommand extends Command
{
    public static List<XGMFMCommand> filterChannel(List<XGMFMCommand> commands, int channel, boolean getWait, boolean getLoopStart)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (XGMFMCommand com : commands)
            if ((com.getChannel() == channel) || (getWait && com.isWait(true)) || (getLoopStart && com.isLoopStart()))
                result.add(com);

        return result;
    }

    public static List<XGMFMCommand> filterYMWrite(List<XGMFMCommand> commands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (XGMFMCommand com : commands)
            if (com.isYMWrite())
                result.add(com);

        return result;
    }

    public static List<XGMFMCommand> filterYMWrite(List<XGMFMCommand> commands, int startInd, int endInd)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        int ind = startInd;
        while (ind < endInd)
        {
            final XGMFMCommand com = commands.get(ind);

            if (com.isYMWrite())
                result.add(com);

            ind++;
        }

        return result;
    }

    public static List<XGMFMCommand> filterYMFreq(List<XGMFMCommand> commands, boolean wantFreqLow)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (XGMFMCommand com : commands)
            if (com.isYMFreqWrite() || (wantFreqLow && com.isYMFreqDeltaWrite()))
                result.add(com);

        return result;
    }

    public static int findNextYMKeyCommand(List<XGMFMCommand> commands, int startInd)
    {
        int ind = startInd;

        while (ind < commands.size())
        {
            if (commands.get(ind).isYMKeyWrite())
                return ind;
            ind++;
        }

        return ind;
    }

    // NEW DEFINE FOR XGM V2
    public final static int WAIT_SHORT = 0x00;
    public final static int WAIT_LONG = 0x0F;

    public final static int PCM = 0x10;

    public final static int FM_LOAD_INST = 0x20;
    public final static int FM_FREQ = 0x30;
    public final static int FM_KEY = 0x40;
    public final static int FM_KEY_SEQ = 0x50;

    public final static int FM0_PAN = 0x60;
    public final static int FM1_PAN = 0x70;

    public final static int FM_FREQ_WAIT = 0x80;
    public final static int FM_TL = 0x90;

    public final static int FM_FREQ_DELTA = 0xA0;
    public final static int FM_FREQ_DELTA_WAIT = 0xB0;
    public final static int FM_TL_DELTA = 0xC0;
    public final static int FM_TL_DELTA_WAIT = 0xD0;

    public final static int FM_WRITE = 0xE0;

    public final static int FRAME_DELAY = 0xF0;

    public final static int FM_KEY_ADV = 0xF8;
    public final static int FM_LFO = 0xF9;
    public final static int FM_CH3_SPECIAL_ON = 0xFA;
    public final static int FM_CH3_SPECIAL_OFF = 0xFB;
    public final static int FM_DAC_ON = 0xFC;
    public final static int FM_DAC_OFF = 0xFD;

    public final static int LOOP = 0xFF;

    // internal id to trace command to remove
    public final static int DUMMY = 0xF7;
    public final static int LOOP_START = 0xF6;

    static int computeSize(byte[] data, int offset)
    {
        final int command = Util.getInt8(data, offset);

        switch (command)
        {
            case WAIT_LONG:
                return 2;
            case FM_KEY_ADV:
                return 2;
            case FM_LFO:
                return 2;
            case FM_CH3_SPECIAL_ON:
            case FM_CH3_SPECIAL_OFF:
            case FM_DAC_ON:
            case FM_DAC_OFF:
                return 1;
            case FRAME_DELAY:
                return 1;
            case LOOP:
                return 4;
        }

        switch (command & 0XF0)
        {
            default:
            case WAIT_SHORT:
                return 1;

            case PCM:
                return 2;

            case FM_LOAD_INST:
                return 31;

            case FM_FREQ:
            case FM_FREQ_WAIT:
                return 3;

            case FM_KEY:
                return 1;
            case FM_KEY_SEQ:
                return 1;

            case FM_TL:
            case FM_TL_DELTA:
            case FM_TL_DELTA_WAIT:
                return 2;

            case FM0_PAN:
            case FM1_PAN:
                return 1;

            case FM_WRITE:
                return 1 + (2 * ((command & 0x07) + 1));

            case FM_FREQ_DELTA:
            case FM_FREQ_DELTA_WAIT:
                return 2;
        }
    }

    public XGMFMCommand(byte[] data, int offset)
    {
        this(Arrays.copyOfRange(data, offset, offset + computeSize(data, offset)));
    }

    XGMFMCommand(byte[] data)
    {
        super(data);

        size = data.length;
    }

    private XGMFMCommand(int command)
    {
        super(command);

        size = data.length;
    }

    public XGMFMCommand(byte[] data, int offset, int time)
    {
        this(data, offset);

        this.time = time;
    }

    public int getType()
    {
        if (this instanceof LoopStartCommand)
            return LOOP_START;

        final int com = getCommand();

        if (com == WAIT_LONG)
            return WAIT_LONG;
        if (com == FM_KEY_ADV)
            return FM_KEY_ADV;
        if (com == FM_LFO)
            return FM_LFO;
        if (com == FM_CH3_SPECIAL_ON)
            return FM_CH3_SPECIAL_ON;
        if (com == FM_CH3_SPECIAL_OFF)
            return FM_CH3_SPECIAL_OFF;
        if (com == FM_DAC_ON)
            return FM_DAC_ON;
        if (com == FM_DAC_OFF)
            return FM_DAC_OFF;
        if (com == FRAME_DELAY)
            return FRAME_DELAY;
        if (com == LOOP)
            return LOOP;
        if (com == DUMMY)
            return DUMMY;

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

        return isWaitShort() || isWaitLong() || isYMFreqWriteWait() || isYMFreqDeltaWriteWait() || isYMTLDeltaWait() || isFrameDelay();
    }

    public int getWaitFrame()
    {
        if (isWaitLong())
            return Util.getInt8(data, 1) + 16;
        if (isWaitShort())
            return (Util.getInt8(data, 0) & 0xF) + 1;
        if (isWait(false))
            return 1;

        return 0;
    }

    public boolean isDummy()
    {
        return getType() == DUMMY;
    }

    public boolean isFrameDelay()
    {
        return getType() == FRAME_DELAY;
    }

    public boolean isLoop()
    {
        return getType() == LOOP;
    }

    public boolean isLoopStart()
    {
        return this instanceof LoopStartCommand;
    }

    public boolean isPCM()
    {
        return getType() == PCM;
    }

    public boolean isYMLoadInst()
    {
        return getType() == FM_LOAD_INST;
    }

    public boolean isYMWrite()
    {
        return (getType() == FM_WRITE);
    }

    public boolean isYMFreqWriteNoWait()
    {
        return getType() == FM_FREQ;
    }

    public boolean isYMFreqWriteWait()
    {
        return getType() == FM_FREQ_WAIT;
    }

    public boolean isYMFreqWrite()
    {
        return isYMFreqWriteNoWait() || isYMFreqWriteWait();
    }

    public int getYMFreqValue()
    {
        if (isYMFreqWrite())
            return ((data[1] & 0x3F) << 8) | (data[2] & 0xFF);

        return -1;
    }

    public boolean isYMFreqSpecialWrite()
    {
        return isYMFreqWrite() && ((getCommand() & 8) != 0);
    }

    public boolean isYMFreqWithKeyON()
    {
        return isYMFreqWrite() && ((data[1] & 0x80) != 0);
    }

    public boolean isYMFreqWithKeyOFF()
    {
        return isYMFreqWrite() && ((data[1] & 0x40) != 0);
    }

    public boolean isYMFreqWithKeyWrite()
    {
        return isYMFreqWrite() && ((data[1] & 0xC0) != 0);
    }

    public void setYMFreqKeyON()
    {
        if (isYMFreqWrite())
            data[1] |= 0x80;
    }

    public void setYMFreqKeyOFF()
    {
        if (isYMFreqWrite())
            data[1] |= 0x40;
    }

    public boolean isYMFreqDeltaWriteNoWait()
    {
        return getType() == FM_FREQ_DELTA;
    }

    public boolean isYMFreqDeltaWriteWait()
    {
        return getType() == FM_FREQ_DELTA_WAIT;
    }

    public boolean isYMFreqDeltaWrite()
    {
        return isYMFreqDeltaWriteNoWait() || isYMFreqDeltaWriteWait();
    }

    public boolean isYMFreqDeltaSpecialWrite()
    {
        return isYMFreqDeltaWrite() && ((getCommand() & 8) != 0);
    }

    public int getYMFreqDeltaValue()
    {
        if (isYMFreqDeltaWrite())
        {
            final int delta = ((data[1] >> 1) & 0x7F) + 1;
            return ((data[1] & 1) != 0) ? -delta : delta;
        }

        return -1;
    }

    public boolean toFreqDelta(int delta)
    {
        // can only convert FREQ command
        if (isYMFreqWrite())
        {
            final int cmd = isYMFreqWriteWait() ? FM_FREQ_DELTA_WAIT : FM_FREQ_DELTA;
            final int deltav = (delta < 0) ? -(delta + 1) : (delta - 1);
            data = new byte[] {(byte) (cmd | data[0] & 0xF), (byte) (((delta < 0) ? 1 : 0) | (deltav << 1))};
            size = 2;
            return true;
        }

        return false;
    }

    public boolean isYMKeyFastWrite()
    {
        return getType() == FM_KEY;
    }

    public boolean isYMKeyONWrite()
    {
        return isYMKeyFastWrite() && ((data[0] & 8) != 0);
    }

    public boolean isYMKeyOFFWrite()
    {
        return isYMKeyFastWrite() && ((data[0] & 8) == 0);
    }

    public boolean isYMKeySequence()
    {
        return getType() == FM_KEY_SEQ;
    }

    public boolean isYMKeySequenceONOFF()
    {
        return isYMKeySequence() && ((data[0] & 8) != 0);
    }

    public boolean isYMKeySequenceOFFON()
    {
        return isYMKeySequence() && ((data[0] & 8) == 0);
    }

    public boolean isYMKeyAdvWrite()
    {
        return getType() == FM_KEY_ADV;
    }

    public boolean isYMKeyWrite()
    {
        return isYMKeyFastWrite() || isYMKeySequence() || isYMKeyAdvWrite();
    }

    public boolean toKeySeq(boolean onOff)
    {
        if (isYMKeyFastWrite())
        {
            data = new byte[] {(byte) (FM_KEY_SEQ | (data[0] & 0x7) | (onOff ? 8 : 0))};
            size = 1;
            return true;
        }

        return false;
    }

    public boolean isYMSetTL()
    {
        return getType() == FM_TL;
    }

    public int getYMTLValue()
    {
        if (isYMSetTL())
            return ((data[1] >> 1) & 0x7F);

        return -1;
    }

    public boolean isYMTLDeltaNoWait()
    {
        return getType() == FM_TL_DELTA;
    }

    public boolean isYMTLDeltaWait()
    {
        return getType() == FM_TL_DELTA_WAIT;
    }

    public boolean isYMTLDelta()
    {
        return isYMTLDeltaNoWait() || isYMTLDeltaWait();
    }

    public int getYMTLDelta()
    {
        if (isYMTLDelta())
        {
            final int delta = ((data[1] >> 2) & 0x3F) + 1;
            return ((data[1] & 2) != 0) ? -delta : delta;

        }

        return -1;
    }

    public boolean toTLDelta(int delta)
    {
        // can only convert set TL command
        if (isYMSetTL())
        {
            final int deltav = (delta < 0) ? -(delta + 1) : (delta - 1);
            data = new byte[] {(byte) (FM_TL_DELTA | data[0] & 0xF), (byte) ((data[1] & 1) | ((delta < 0) ? 2 : 0) | (deltav << 2))};
            size = 2;
            return true;
        }

        return false;
    }

    public boolean isYMPAN()
    {
        return (getType() == FM0_PAN) || (getType() == FM1_PAN);
    }

    public boolean isYMCH3SpecialMode()
    {
        return (getType() == FM_CH3_SPECIAL_ON) || (getType() == FM_CH3_SPECIAL_OFF);
    }

    public boolean isYMDACMode()
    {
        return (getType() == FM_DAC_ON) || (getType() == FM_DAC_OFF);
    }

    public boolean isYMLFO()
    {
        return getType() == FM_LFO;
    }

    public boolean isYMSetting()
    {
        return isYMCH3SpecialMode() || isYMDACMode() || isYMLFO();
    }

    public boolean supportWait()
    {
        // can this command be muted in com + wait ?
        return isYMFreqWrite() || isYMFreqDeltaWrite() || isYMTLDelta();
    }

    public void addWait()
    {
        if (supportWait())
        {
            if (isYMFreqWriteNoWait())
                data[0] += (FM_FREQ_WAIT - FM_FREQ);
            else if (isYMFreqDeltaWriteNoWait())
                data[0] += (FM_FREQ_DELTA_WAIT - FM_FREQ_DELTA);
            else if (isYMTLDeltaNoWait())
                data[0] += (FM_TL_DELTA_WAIT - FM_TL_DELTA);
        }
    }

    public int getPCMId()
    {
        if (getType() == PCM)
            return Util.getInt8(data, 1);

        return -1;
    }

    public void setPCMId(int value)
    {
        if (getType() == PCM)
            Util.setInt8(data, 1, value);
    }

    public int getPCMChannel()
    {
        if (getType() == PCM)
            return Util.getInt8(data, 0) & 3;

        return 0;
    }

    public boolean getPCMHalfRate()
    {
        if (getType() == PCM)
            return ((Util.getInt8(data, 0) & 4) != 0);

        return false;
    }

    public int getYMPort()
    {
        switch (getType())
        {
            case FM0_PAN:
                return 0;
            case FM1_PAN:
                return 1;

            case FM_LOAD_INST:
            case FM_FREQ:
            case FM_FREQ_WAIT:
            case FM_FREQ_DELTA:
            case FM_FREQ_DELTA_WAIT:
            case FM_KEY:
            case FM_KEY_SEQ:
                return (data[0] >> 2) & 1;

            case FM_WRITE:
                return (data[0] >> 3) & 1;

            case FM_TL:
            case FM_TL_DELTA:
            case FM_TL_DELTA_WAIT:
                return (data[1] >> 0) & 1;

            case FM_KEY_ADV:
                return (data[1] >> 2) & 1;
        }

        return 0;
    }

    public int getYMChannel()
    {
        switch (getType())
        {
            case FM_FREQ:
            case FM_FREQ_WAIT:
            case FM_FREQ_DELTA:
            case FM_FREQ_DELTA_WAIT:
                if ((data[0] & 8) != 0)
                    return 2;
            case FM_LOAD_INST:
            case FM_KEY:
            case FM_KEY_SEQ:
            case FM_TL:
            case FM_TL_DELTA:
            case FM_TL_DELTA_WAIT:
            case FM0_PAN:
            case FM1_PAN:
                return (data[0] & 3);

            case FM_WRITE:
                if ((data[1] & 0xF8) == 0xA8)
                    return 2;
            case FM_KEY_ADV:
                return (data[1] & 3);
        }

        return -1;
    }

    public int getYMGlobalChannel()
    {
        return (getYMPort() * 3) + getYMChannel();
    }

    @Override
    public int getChannel()
    {
        return getYMGlobalChannel();
    }

    public int getYMSlot()
    {
        switch (getType())
        {
            case FM_FREQ:
            case FM_FREQ_WAIT:
            case FM_FREQ_DELTA:
            case FM_FREQ_DELTA_WAIT:
                return ((data[0] & 8) != 0) ? ((data[0] & 3) + 1) : -1;
            case FM_TL:
            case FM_TL_DELTA:
            case FM_TL_DELTA_WAIT:
                return ((data[0] >> 2) & 3);
        }

        return -1;
    }

    public int getYMNumWrite()
    {
        if (isYMWrite())
            return (data[0] & 7) + 1;

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

    public void setDummy()
    {
        final byte[] newData = new byte[data.length + 1];

        // set DUMMY command followed by old content
        newData[0] = (byte) DUMMY;
        System.arraycopy(data, 0, newData, 1, data.length);

        // set data
        data = newData;
        size = newData.length;
    }

    private void setYMWrite(byte header, List<Integer> dataList)
    {
        // empty list ? --> set to dummy command
        if (dataList.isEmpty())
        {
            setDummy();
            return;
        }

        // just keep ignored writes
        final byte[] newData = new byte[dataList.size() + 1];

        newData[0] = header;

        // set new size
        newData[0] &= 0xF8;
        newData[0] |= (dataList.size() / 2) - 1;

        for (int i = 0; i < dataList.size(); i += 2)
        {
            newData[i + 1] = dataList.get(i + 0).byteValue();
            newData[i + 2] = dataList.get(i + 1).byteValue();
        }

        // update command
        data = newData;
        size = newData.length;
    }

    @Override
    public String toString()
    {
        String result = "";

        if (isWaitShort())
            result += "FM WAIT S #" + getWaitFrame();
        else if (isWaitLong())
            result += "FM WAIT L #" + getWaitFrame();
        else if (isPCM())
            result += "PCM #" + getPCMId();
        else if (isYMLoadInst())
            result += "FM LOADINST";
        else if (isYMFreqSpecialWrite())
            result += "FM FREQ" + (isYMFreqWriteWait() ? " W" : "") + " S" + getYMSlot() + " " + (((data[1] & 0x40) != 0) ? "x" : "")
                    + (((data[1] & 0x80) != 0) ? "o" : "");
        else if (isYMFreqWrite())
            result += "FM FREQ" + (isYMFreqWriteWait() ? " W" : "") + " " + (((data[1] & 0x40) != 0) ? "x" : "") + (((data[1] & 0x80) != 0) ? "o" : "");
        else if (isYMFreqDeltaSpecialWrite())
            result += "FM FREQD " + getYMFreqDeltaValue() + (isYMFreqDeltaWriteWait() ? " W" : "") + " S" + getYMSlot();
        else if (isYMFreqDeltaWrite())
            result += "FM FREQD " + getYMFreqDeltaValue() + (isYMFreqDeltaWriteWait() ? " W" : "");
        else if (isYMKeyONWrite())
            result += "FM KEY ON";
        else if (isYMKeyOFFWrite())
            result += "FM KEY OFF";
        else if (isYMKeyAdvWrite())
            result += "FM KEY ADV";
        else if (isYMKeySequence())
            result += "FM KEY SEQ";
        else if (isYMKeyWrite())
            result += "FM KEY";
        else if (isYMSetTL())
            result += "FM TL S" + getYMSlot();
        else if (isYMTLDelta())
            result += "FM TLD " + getYMTLDelta() + (isYMTLDeltaWait() ? " W." : "") + " S" + getYMSlot();
        else if (isYMPAN())
            result += "FM PAN";
        else if (isYMDACMode())
        {
            if (getType() == FM_DAC_ON)
                result += "DAC ON";
            else
                result += "DAC OFF";
        }
        else if (isYMLFO())
            result += "LFO";
        else if (isYMCH3SpecialMode())
            result += "CH2 spe";
        else if (isYMWrite())
            result += "FM WRITE";
        else if (isFrameDelay())
            result += "FRAME DELAY";
        else if (isLoopStart())
            result += "FM LOOP St";
        else if (isLoop())
            result += "FM LOOP #" + StringUtil.toHexaString(getLoopAddr(), 6);
        else if (isDummy())
            result += "FM DUMMY (" + new XGMFMCommand(data, 1).toString() + ")";

        return result;
    }

    public static XGMFMCommand createFrameCommand()
    {
        return new XGMFMCommand(XGMFMCommand.WAIT_SHORT);
    }

    public static List<XGMFMCommand> createYMKeyCommands(Collection<VGMCommand> commands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (VGMCommand com : commands)
            result.add(createYMKeyCommand(com));

        return result;
    }

    public static List<XGMFMCommand> createYMCHCommands(Collection<VGMCommand> commands, int channel)
    {
        final List<XGMFMCommand> result = new ArrayList<>();
        final List<VGMCommand> remaining = new ArrayList<>(commands);

        while (remaining.size() > 0)
            result.add(createYMCHCommand(remaining, channel));

        return result;
    }

    public static List<XGMFMCommand> createWaitCommands(int wait)
    {
        final List<XGMFMCommand> result = new ArrayList<>();
        int remain = wait;

        while (remain > 271)
        {
            result.add(new WaitLongCommand(271));
            remain -= 271;
        }
        if (remain > 15)
            result.add(new WaitLongCommand(remain));
        else
            result.add(new WaitShortCommand(remain));

        return result;
    }

    public static List<XGMFMCommand> createYMMiscCommands(List<VGMCommand> commands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (VGMCommand command : commands)
        {
            final XGMFMCommand xgmCom = createYMMiscCommand(command);

            if (xgmCom != null)
                result.add(xgmCom);
        }

        return result;
    }

    public static List<XGMFMCommand> createYMFreqCommands(List<VGMCommand> commands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();
        final List<VGMCommand> coupledCom = new ArrayList<>(2);

        int ind = 0;
        while (ind < commands.size())
        {
            coupledCom.clear();
            coupledCom.add(commands.get(ind++));
            coupledCom.add(commands.get(ind++));

            result.add(createYMFreqCommand(coupledCom, false, false));
        }

        return result;
    }

    public static List<XGMFMCommand> createPCMCommands(XGM xgm, List<VGMCommand> commands)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        for (VGMCommand command : commands)
        {
            if (command.isStreamStartLong() || command.isStreamStart() || command.isStreamStop())
            {
                XGMFMCommand xgmCommand = createPCMCommand(xgm, command, 0);

                if (xgmCommand != null)
                    result.add(xgmCommand);
            }
        }

        // while (result.size() > 1)
        // {
        // System.out.println("Sample command :" + result.get(0) + " ignored...");
        // result.remove(0);
        // }

        return result;
    }

    public static XGMFMCommand createYMMiscCommand(VGMCommand command)
    {
        final byte[] data;

        switch (command.getYM2612Register())
        {
            case 0x22:
                // LFO
                data = new byte[2];
                data[0] = (byte) FM_LFO;
                data[1] = (byte) command.getYM2612Value();
                break;

            case 0x27:
                // CH2 special mode
                data = new byte[1];
                if ((command.getYM2612Value() & 0x40) != 0)
                    data[0] = (byte) FM_CH3_SPECIAL_ON;
                else
                    data[0] = (byte) FM_CH3_SPECIAL_OFF;
                break;

            case 0x2B:
                // DAC enable
                data = new byte[1];
                if ((command.getYM2612Value() & 0x80) != 0)
                    data[0] = (byte) FM_DAC_ON;
                else
                    data[0] = (byte) FM_DAC_OFF;
                break;

            default:
                // ignore
                if (!Launcher.silent)
                    System.out.println(String.format("VGM --> XGM conversion: write to register %2X ignored", Integer.valueOf(command.getYM2612Register())));
                return null;
        }

        return new XGMFMCommand(data);
    }

    protected static XGMFMCommand createYMKeyCommand(VGMCommand command)
    {
        final byte[] data;

        final int value = command.getYM2612Value();
        final int keyWrite = value & 0xF0;
        final int ch = value & 0x07;

        // ALL OFF or ALL ON ?
        if ((keyWrite == 0x00) || (keyWrite == 0xF0))
        {
            data = new byte[1];
            // use FAST key command
            data[0] = (byte) (FM_KEY | ((keyWrite == 0) ? 0x00 : 0x08) | ch);
        }
        else
        {
            data = new byte[2];
            // use ADV key command
            data[0] = (byte) FM_KEY_ADV;
            data[1] = (byte) value;
        }

        return new XGMFMCommand(data);
    }

    private static XGMFMCommand createYMCHCommand(List<VGMCommand> commands, int channel)
    {
        final int size = Math.min(8, commands.size());
        final byte[] data = new byte[(size * 2) + 1];

        // set command
        data[0] = (byte) (FM_WRITE);
        // set port
        data[0] |= (channel >= 3) ? 8 : 0;
        // set size
        data[0] |= (size - 1);

        int off = 1;
        for (int i = 0; i < size; i++)
        {
            final VGMCommand command = commands.get(i);
            int reg = command.getYM2612Register();

            data[off++] = (byte) reg;
            data[off++] = (byte) command.getYM2612Value();
        }

        // remove elements we have done
        commands.subList(0, size).clear();

        return new XGMFMCommand(data);
    }

    private static XGMFMCommand createYMFreqCommand(List<VGMCommand> commands, boolean keyOffBefore, boolean keyOnAfter)
    {
        final byte[] data = new byte[3];

        // frequency is set with 2 VGM commands
        final VGMCommand comFreqHigh = commands.get(0);
        final VGMCommand comFreqLowh = commands.get(1);

        final int port = comFreqHigh.getYM2612Port();
        final int ch = comFreqHigh.getYM2612PortChannel();
        final int reg = comFreqHigh.getYM2612Register();
        final int spe = ((reg >= 0xA8) && (reg < 0xB0)) ? 1 : 0;

        data[0] = (byte) (FM_FREQ | (port << 3) | (spe << 2) | ch);
        data[1] = (byte) ((comFreqHigh.getYM2612Value() & 0x3F) | (keyOffBefore ? 0x40 : 0x00) | (keyOnAfter ? 0x80 : 0x00));
        data[2] = (byte) comFreqLowh.getYM2612Value();

        return new XGMFMCommand(data);
    }

    public static XGMFMCommand createYMFreqCommand(int channel, boolean special, int freq, boolean keyOffBefore, boolean keyOnAfter)
    {
        final byte[] data = new byte[3];

        final int port = (channel >= 3) ? 1 : 0;
        final int ch = (port == 1) ? ((channel + 1) & 3) : channel & 3;

        data[0] = (byte) (FM_FREQ | (port << 2) | (special ? 8 : 0) | ch);
        data[1] = (byte) (((freq >> 8) & 0x3F) | (keyOffBefore ? 0x40 : 0x00) | (keyOnAfter ? 0x80 : 0x00));
        data[2] = (byte) (freq & 0xFF);

        return new XGMFMCommand(data);
    }

    public static XGMFMCommand createPCMStopCommand(int channel)
    {
        final byte[] data = new byte[2];

        data[0] = (byte) (PCM | (channel & 0x3));
        // stop command (id #0)
        data[1] = 0;

        return new XGMFMCommand(data);
    }

    public static XGMFMCommand createPCMCommand(XGM xgm, VGMCommand command, int channel)
    {
        final byte[] data = new byte[2];
        XGMSample sample;

        data[0] = (byte) (PCM | (channel & 0x3));

        if (command.isStreamStartLong())
        {
            sample = xgm.getSampleByOriginAddress(command.getStreamSampleAddress());

            // no sample found
            if (sample == null)
            {
                System.err.println("Warning: no corresponding sample found in XGM (origin sample addr="
                        + StringUtil.toHexaString(command.getStreamSampleAddress(), 6) + ")");
                return null;
            }
        }
        else if (command.isStreamStart())
        {
            sample = xgm.getSampleByOriginId(command.getStreamBlockId() + 1);

            // no sample found
            if (sample == null)
            {
                System.err.println(
                        "Warning: no corresponding sample found in XGM (origin sample id=" + StringUtil.toHexaString(command.getStreamBlockId() + 1, 2) + ")");
                // FIXME: do we have to do that ??
                // sample = xgm.getSampleByOriginId(command.getStreamId() + 1);
                return null;
            }
        }
        else
        {
            // stop command (id #0)
            data[1] = 0;
            return new XGMFMCommand(data);
        }

        // half speed playback
        if (sample.halfRate)
            data[0] |= 4;
        data[1] = (byte) sample.id;

        return new XGMFMCommand(data);
    }

    public static List<XGMFMCommand> convertToSetPanningCommands(int port, int ch, XGMFMCommand com, YM2612State ymState)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        if (!com.isYMWrite())
            return result;

        final List<Integer> ignored = new ArrayList<>();
        final int num = (com.data[0] & 0x07) + 1;

        for (int i = 0; i < num; i++)
        {
            // reg
            final int reg = (Util.getInt8(com.data, (i * 2) + 1) & 0xFC) | ch;
            // value
            final int value = Util.getInt8(com.data, (i * 2) + 2);
            // pan value
            final int pan = (value >> 6) & 3;

            // panning write without LFO change ?
            if ((reg >= 0xB4) && (reg < 0xB8) && ((ymState.get(port, reg) & 0x3F) == (value & 0x3F)))
            {
                final byte[] data = new byte[1];

                data[0] = (byte) ((port == 0) ? FM0_PAN : FM1_PAN);
                // channel
                data[0] |= ch;
                // panning
                data[0] |= (pan << 2);

                result.add(new XGMFMCommand(data, 0, com.time));
            }
            else
            {
                // keep trace of ignored
                ignored.add(Integer.valueOf(reg));
                ignored.add(Integer.valueOf(value));
            }
        }

        // update original command
        com.setYMWrite(com.data[0], ignored);

        return result;
    }

    public static List<XGMFMCommand> convertToSetTLCommands(int port, int ch, XGMFMCommand com)
    {
        final List<XGMFMCommand> result = new ArrayList<>();

        if (!com.isYMWrite())
            return result;

        final List<Integer> ignored = new ArrayList<>();
        final int num = (com.data[0] & 0x07) + 1;

        for (int i = 0; i < num; i++)
        {
            // reg
            final int reg = Util.getInt8(com.data, (i * 2) + 1);
            // value
            final int value = Util.getInt8(com.data, (i * 2) + 2);

            // TL write ?
            if ((reg >= 0x40) && (reg < 0x50))
            {
                final byte[] data = new byte[2];
                final int s = (reg >> 2) & 3;

                data[0] = (byte) (FM_TL + (s << 2) + ch);
                data[1] = (byte) ((value & 0x7F) << 1);
                data[1] |= (byte) (port & 1);

                result.add(new XGMFMCommand(data, 0, com.time));
            }
            else
            {
                // keep trace of ignored
                ignored.add(Integer.valueOf(reg));
                ignored.add(Integer.valueOf(value));
            }
        }

        // update original command
        com.setYMWrite(com.data[0], ignored);

        return result;
    }

    public static XGMFMCommand convertToLoadInstCommand(int port, int ch, List<XGMFMCommand> fmWriteCHCommands, YM2612State ymState)
    {
        final byte[] data = new byte[31];
        final List<Integer> ignored = new ArrayList<>();

        data[0] = (byte) (FM_LOAD_INST | ((port & 1) << 2) | ((ch & 3) << 0));

        // initialize values from YM state
        int d = 1;
        int time = 0;

        // slot writes
        for (int r = 0; r < 7; r++)
            for (int s = 0; s < 4; s++)
                data[d++] = (byte) ymState.get(port, 0x30 + (r << 4) + (s << 2) + ch);

        // ch writes
        data[d++] = (byte) ymState.get(port, 0xB0 + ch);
        data[d] = (byte) ymState.get(port, 0xB4 + ch);

        for (XGMFMCommand com : fmWriteCHCommands)
        {
            if (!com.isYMWrite())
            {
                System.err.println("Warning: convertToLoadInstCommand - unexpected command #" + StringUtil.toHexaString(com.getType(), 2));
                continue;
            }

            // all commands should have same time here
            time = com.time;
            ignored.clear();
            final int num = com.getYMNumWrite();

            for (int i = 0; i < num; i++)
            {
                // reg
                final int reg = Util.getInt8(com.data, (i * 2) + 1);
                // value
                final int value = Util.getInt8(com.data, (i * 2) + 2);

                // slot write ?
                if ((reg >= 0x30) && (reg < 0xA0))
                {
                    final int r = (reg >> 4) & 0xF;
                    final int s = (reg >> 2) & 3;
                    // get data index
                    final int index = ((r - 3) * 4) + s;

                    data[index + 1] = (byte) value;
                }
                // channel write (ALGO or PAN/LFO register)
                else if ((reg >= 0xB0) && (reg <= 0xB8))
                    data[29 + ((reg >= 0xB4) ? 1 : 0)] = (byte) value;
                else
                {
                    // keep trace of ignored
                    ignored.add(Integer.valueOf(reg));
                    ignored.add(Integer.valueOf(value));
                }
            }

            // update original command
            com.setYMWrite(com.data[0], ignored);
        }

        return new XGMFMCommand(data, 0, time);
    }

    public static class WaitShortCommand extends XGMFMCommand
    {
        public WaitShortCommand(int wait)
        {
            super(new byte[] {(byte) (WAIT_SHORT | ((wait - 1) & 0xF))});
        }
    }

    public static class WaitLongCommand extends XGMFMCommand
    {
        public WaitLongCommand(int wait)
        {
            super(new byte[] {(byte) WAIT_LONG, (byte) (wait - 16)});
        }
    }

    public static class FrameCommand extends XGMFMCommand
    {
        public FrameCommand()
        {
            super(new byte[] {(byte) WAIT_SHORT});
        }
    }

    public static class FrameDelayCommand extends XGMFMCommand
    {
        public FrameDelayCommand()
        {
            super(new byte[] {(byte) FRAME_DELAY});
        }
    }

    public static class LoopCommand extends XGMFMCommand
    {
        public LoopCommand(int offset)
        {
            super(new byte[] {(byte) LOOP, (byte) (offset >> 0), (byte) (offset >> 8), (byte) (offset >> 16)});
        }
    }

    public static class LoopStartCommand extends XGMFMCommand
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
