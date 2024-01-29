package sgdk.xgm2tool.struct;

import java.util.ArrayList;
import java.util.List;

import sgdk.xgm2tool.format.VGMCommand;

public class PSGState
{
    final int registers[][];
    final boolean init[][];
    int index;
    int type;

    public PSGState()
    {
        super();

        registers = new int[4][2];
        init = new boolean[4][2];

        index = -1;
        type = -1;

        clear();
    }

    public PSGState(PSGState state)
    {
        super();

        registers = new int[4][2];
        init = new boolean[4][2];

        index = -1;
        type = -1;

        for (int i = 0; i < 4; i++)
        {
            registers[i][0] = state.registers[i][0];
            registers[i][1] = state.registers[i][1];
            init[i][0] = state.init[i][0];
            init[i][1] = state.init[i][1];
        }
    }

    public void clear()
    {
        for (int i = 0; i < 4; i++)
        {
            registers[i][0] = 0;
            registers[i][1] = 0;
            init[i][0] = false;
            init[i][1] = false;
        }
    }

    public int get(int ind, int typ)
    {
        switch (typ)
        {
            case 0:
                // tone / noise
                if (ind == 3)
                    return registers[ind][typ] & 0x7;

                return registers[ind][typ] & 0x3FF;

            case 1:
                // volume
                return registers[ind][typ] & 0xF;
        }

        return 0;
    }

    public void write(int value)
    {
        if ((value & 0x80) != 0)
            writeLow(value & 0x7F);
        else
            writeHigh(value & 0x7F);
    }

    private void writeLow(int value)
    {
        index = (value >> 5) & 0x03;
        type = (value >> 4) & 0X01;

        if ((type == 0) && (index == 3))
        {
            registers[index][type] &= ~0x7;
            registers[index][type] |= value & 0x7;
        }
        else
        {
            registers[index][type] &= ~0xF;
            registers[index][type] |= value & 0xF;
        }

        init[index][type] = true;
    }

    private void writeHigh(int value)
    {
        if ((type == 0) && (index == 3))
        {
            registers[index][type] &= ~0x7;
            registers[index][type] |= value & 0x7;
        }
        else if (type == 1)
        {
            registers[index][type] &= ~0xF;
            registers[index][type] |= value & 0xF;
        }
        else
        {
            registers[index][type] &= ~0x3F0;
            registers[index][type] |= (value & 0x3F) << 4;
        }

        init[index][type] = true;
    }

    public boolean isSame(PSGState state, int ind, int typ)
    {
        if ((init[ind][typ] == false) && (state.init[ind][typ] == false))
            return true;

        return init[ind][typ] && (state.get(ind, typ) == get(ind, typ));
    }

    public boolean isLowSame(PSGState state, int ind, int typ)
    {
        if ((init[ind][typ] == false) && (state.init[ind][typ] == false))
            return true;

        return init[ind][typ] && ((state.get(ind, typ) & 0xF) == (get(ind, typ) & 0xF));
    }

    public boolean isHighSame(PSGState state, int ind, int typ)
    {
        if ((init[ind][typ] == false) && (state.init[ind][typ] == false))
            return true;

        return init[ind][typ] && ((state.get(ind, typ) & 0x3F0) == (get(ind, typ) & 0x3F0));
    }

    public boolean isDiff(PSGState state, int ind, int typ)
    {
        return !isSame(state, ind, typ);
    }

    public boolean isLowDiffOnly(PSGState state, int ind, int typ)
    {
        return !isLowSame(state, ind, typ) && isHighSame(state, ind, typ);
    }

    private VGMCommand createLowWriteCommand(int ind, int typ, int value)
    {
        return new VGMCommand(
                new byte[] {VGMCommand.WRITE_SN76489, (byte) (0x80 | (ind << 5) | (typ << 4) | (value & 0xF))});
    }

    private static List<VGMCommand> createWriteCommands(int ind, int typ, int value)
    {
        final List<VGMCommand> result = new ArrayList<VGMCommand>();

        // rebuild data
        result.add(new VGMCommand(
                new byte[] {VGMCommand.WRITE_SN76489, (byte) (0x80 | (ind << 5) | (typ << 4) | (value & 0xF))}));
        if ((typ == 0) && (ind != 3))
            result.add(new VGMCommand(new byte[] {VGMCommand.WRITE_SN76489, (byte) (0x00 | ((value >> 4) & 0x3F))}));

        return result;
    }

    /**
     * Returns commands list to update to the specified PSG state
     */
    public List<VGMCommand> getDelta(PSGState state)
    {
        final List<VGMCommand> result = new ArrayList<VGMCommand>();

        for (int ind = 0; ind < 4; ind++)
        {
            for (int typ = 0; typ < 2; typ++)
            {
                if (typ == 0)
                {
                    // value different on low bits only --> add single command
                    if (isLowDiffOnly(state, ind, typ))
                        result.add(createLowWriteCommand(ind, typ, state.get(ind, typ)));
                    // value is different --> add commands
                    else if (isDiff(state, ind, typ))
                        result.addAll(createWriteCommands(ind, typ, state.get(ind, typ)));
                }
                else
                {
                    // value is different --> add commands
                    if (isDiff(state, ind, typ))
                        result.addAll(createWriteCommands(ind, typ, state.get(ind, typ)));
                }
            }
        }

        return result;
    }
}
