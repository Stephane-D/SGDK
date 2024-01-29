package sgdk.xgm2tool.format;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import sgdk.tool.StringUtil;
import sgdk.xgm2tool.Launcher;
import sgdk.xgm2tool.struct.Sample;
import sgdk.xgm2tool.tool.Util;

public class SampleBank
{
    final List<InternalSample> samples;
    byte[] data;
    final int id;

    public SampleBank(VGMCommand command)
    {
        super();

        if (!command.isDataBlock())
            throw new IllegalArgumentException(String.format("Incorrect sample data declaration at %6X !", Integer.valueOf(command.getOriginOffset())));

        if (Launcher.verbose)
            System.out.println(String.format("Add data bank %6X:%2X", Integer.valueOf(command.getOriginOffset()), Integer.valueOf(command.getDataBankId())));

        // id
        id = command.getDataBankId();
        // data block
        data = Arrays.copyOfRange(command.data, 7, command.getDataBlockLen() + 7);

        samples = new ArrayList<>();

        // consider the whole bank as a single sample by default
        samples.add(new InternalSample(0, 0, data.length, 0));
    }

    public int getLength()
    {
        return data.length;
    }

    public void addBlock(VGMCommand command)
    {
        if (!command.isDataBlock())
            return;

        if (Launcher.verbose)
            System.out.println(
                String.format("Concat data block %6X to bank %2X", Integer.valueOf(command.getOriginOffset()), Integer.valueOf(command.getDataBankId())));

        // concat data block
        final byte[] newData = new byte[getLength() + command.getDataBlockLen()];

        System.arraycopy(data, 0, newData, 0, getLength());
        System.arraycopy(command.data, 7, newData, getLength(), command.getDataBlockLen());

        // add new sample corresponding to this data block
        samples.add(new InternalSample(samples.size(), getLength(), command.getDataBlockLen(), 0));

        // set new data
        data = newData;
    }

    public VGMCommand getDataBlockCommand()
    {
        final byte[] comData = new byte[getLength() + 7];

        // build data block header
        comData[0] = 0x67;
        comData[1] = 0x66;
        comData[2] = 0x00;
        Util.setInt32(comData, 3, getLength());

        // copy sample data
        System.arraycopy(data, 0, comData, 7, data.length);

        // return command
        return new VGMCommand(comData);
    }

    public List<VGMCommand> getDeclarationCommands()
    {
        final List<VGMCommand> result = new ArrayList<>();

        // rebuild data
        result.add(new VGMCommand(new byte[] {(byte) VGMCommand.STREAM_CONTROL, (byte) id, 0x02, 0x00, 0x2A}));
        result.add(new VGMCommand(new byte[] {(byte) VGMCommand.STREAM_DATA, (byte) id, (byte) id, 0x01, 0x00}));

        return result;
    }

    public InternalSample getSampleByAddress(int address)
    {
        for (InternalSample sample : samples)
        {
            if (sample.matchAddress(address))
                return sample;
        }

        return null;
    }

    public InternalSample getSampleById(int id)
    {
        for (InternalSample sample : samples)
            if (sample.id == id)
                return sample;

        return null;
    }

    public InternalSample addSample(int address, int len, int rate)
    {
        int adjLen = len;

        // end outside bank ? --> bound it to bank size
        if ((address + adjLen) > getLength())
            adjLen = getLength() - address;

        InternalSample result = getSampleByAddress(address);

        // not found --> create new sample
        if (result == null)
        {
            result = new InternalSample(samples.size(), address, adjLen, rate);
            samples.add(result);
        }
        else
        {
            // get address delta (as we allow a small delta when searching for the sample
            final int deltaAddr = address - result.addr;
            // adjust len from address delta
            adjLen -= deltaAddr;
            // bound to bank size if needed
            if ((result.addr + adjLen) > getLength())
                adjLen = getLength() - result.addr;

            // new defined sample ?
            if (result.rate == 0)
            {
                if (Launcher.verbose)
                    System.out.println(String.format("Sample #" + result.id + " confirmed [%6X-%6X]  len: %6X --> %6X   rate: %d --> %d Hz", result.addr,
                        result.addr + (adjLen - 1), result.len, adjLen, result.rate, rate));

                result.rate = rate;
                result.len = adjLen;
            }
            // adjust sample info
            else
            {
                // we apply smooth change of sample rate
                if (Util.isDiffRate(result.rate, rate))
                {
                    if (Launcher.verbose)
                        System.out.print(String.format("Sample #" + result.id + " changed   [%6X-%6X]  rate: %d --> %d", result.addr,
                            result.addr + (result.len - 1), result.rate, rate));

                    // smooth sample rate change
                    result.rate = (result.rate + rate) / 2;

                    if (Launcher.verbose)
                        System.out.println(String.format(" - adjusted to %d Hz", result.rate));
                }

                // adjust length only if longer
                if (result.len < adjLen)
                {
                    if (Launcher.verbose)
                        System.out.println(String.format("Sample #" + result.id + " modified  [%6X-%6X]  len: %6X --> %6X", result.addr, result.addr + (adjLen - 1),
                            result.len, adjLen));
                    result.len = adjLen;
                }

                // // get size of sample frame
                // int frameLen = Sample_getFrameSize(result);
                //
                // // less than 1 frame in size difference --> assume same sample
                // if (abs(adjLen - result->len) < frameLen)
                // {
                // // adjust sample length if needed
                // if (result->len < adjLen)
                // {
                // if (verbose)
                // printf("Sample modified [%6X-%6X] len: %6X --> %6X\n", dataOffset, dataOffset + (len - 1),
                // result->len,
                // adjLen);
                //
                // result->len = adjLen;
                // }
                // }
                // // too large difference in size --> assume we have a new sample here
                // else
                // {
                // if (verbose)
                // printf("Sample added [%6X-%6X] len: %6X rate: %d Hz\n", dataOffset, dataOffset + (adjLen - 1),
                // adjLen,
                // rate);
                //
                // result = Sample_create(getSizeLList(bank->samples), dataOffset, adjLen, rate);
                // insertAfterLList(bank->samples, result);
                // }
            }
        }

        return result;
    }

    public class InternalSample implements Comparable<InternalSample>
    {
        final int id;
        int addr;
        int len;
        int rate;

        public InternalSample(int id, int addr, int len, int rate)
        {
            super();

            this.id = id;
            this.addr = addr;
            this.len = len;
            this.rate = rate;

            if (Launcher.verbose)
                System.out.println(String.format("Sample #" + id + " added     [%6X-%6X]  len: %6X  rate: %d Hz", addr, addr + (len - 1), len, rate));
        }

        public SampleBank getBank()
        {
            return SampleBank.this;
        }

        public void setRate(int value)
        {
            if ((value != 0) && (rate != value))
            {
                if (Launcher.verbose)
                    System.out.println(String.format("Sample #" + id + " changed   [%6X-%6X]  rate: %d --> %d", addr,
                        addr + (len - 1), rate, value));
                rate = value;
            }
        }

        public int getFrameSize()
        {
            if (rate > 0)
                return rate / 60;

            // consider 4Khz by default for safety
            return 4000 / 60;
        }

        public boolean matchAddress(int address)
        {
            final int frameSize = getFrameSize();
            // allow a margin of 1 frame for address
            return Math.abs(addr - address) < frameSize;
        }

        public VGMCommand getSetRateCommand(int r)
        {
            return new VGMCommand(
                    new byte[] {(byte) VGMCommand.STREAM_FREQUENCY, (byte) SampleBank.this.id, (byte) ((r >> 0) & 0xFF), (byte) ((r >> 8) & 0xFF), 0x00, 0x00});
        }

        // public Command getStartCommand(boolean loop)
        // {
        // return new Command(new byte[] {(byte) Command.STREAM_START, (byte) SampleBank.this.id,
        // (byte) SampleBank.this.id, 0x00,
        // (byte) (loop ? 0x01 : 0x00)}, 0);
        // }

        public VGMCommand getStartLongCommand(int l)
        {
            final int adjLen = Math.min(l, len);
            return new VGMCommand(new byte[] {(byte) VGMCommand.STREAM_START_LONG, (byte) SampleBank.this.id, (byte) ((addr >> 0) & 0xFF),
                (byte) ((addr >> 8) & 0xFF), (byte) ((addr >> 16) & 0xFF), 0x00, 0x01, (byte) ((adjLen >> 0) & 0xFF), (byte) ((adjLen >> 8) & 0xFF),
                (byte) ((adjLen >> 16) & 0xFF), 0x00});
        }

        public VGMCommand getStartLongCommand()
        {
            return getStartLongCommand(len);
        }

        public VGMCommand getStopCommand()
        {
            return new VGMCommand(new byte[] {(byte) VGMCommand.STREAM_STOP, (byte) SampleBank.this.id});
        }

        public Sample toIsolatedSample()
        {
            final byte[] ndata = new byte[len];

            System.arraycopy(data, addr, ndata, 0, len);

            return new Sample(ndata, rate);
        }

        @Override
        public int compareTo(InternalSample is)
        {
            return Integer.compare(addr, is.addr);
        }

        @Override
        public String toString()
        {
            return "#" + id + " - rate:" + rate + " addr:" + StringUtil.toHexaString(addr, 6) + " len:" + StringUtil.toHexaString(len, 6);
        }
    }

    // remove unused part of the data array, pack it and update sample addresses
    // return map<sample_addr_old, sample_addr_new>
    public Map<Integer, Integer> optimize()
    {
        final Map<Integer, Integer> sampleAddrChanges = new HashMap<>();

        // sort samples by address
        Collections.sort(samples);

        // get samples total size
        // int len = 0;
        // for (InternalSample sample : samples)
        // len += sample.len;

        // rebuild contiguous buffer for all samples
        final byte[] newData = new byte[data.length];

        int addr = 0;
        int cumulatedOff = 0;
        for (InternalSample sample : samples)
        {
            if (sample.addr <= (addr + cumulatedOff))
                // set position on current sample start address
                addr = sample.addr - cumulatedOff;
            else
                // add delta to cumulated offset
                cumulatedOff += sample.addr - (addr + cumulatedOff);

            // copy sample data to new buffer
            System.arraycopy(data, sample.addr, newData, addr, sample.len);
            // store old_addr / new_addr couple
            sampleAddrChanges.put(Integer.valueOf(sample.addr), Integer.valueOf(addr));
            // then fix sample address
            sample.addr = addr;
            // next
            addr += sample.len;
        }

        // set the data
        data = Arrays.copyOf(newData, addr);

        return sampleAddrChanges;
    }
}
