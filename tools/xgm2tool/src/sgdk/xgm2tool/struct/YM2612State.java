package sgdk.xgm2tool.struct;

import java.util.ArrayList;
import java.util.List;

import sgdk.xgm2tool.format.VGMCommand;
import sgdk.xgm2tool.format.XGMFMCommand;
import sgdk.xgm2tool.tool.Util;

public class YM2612State
{
    final static int[][] duals = {{0x24, 0x25}, {0xA4, 0xA0}, {0xA5, 0xA1}, {0xA6, 0xA2}, {0xAC, 0xA8}, {0xAD, 0xA9}, {0xAE, 0xAA}};

    final int registers[][];
    final boolean init[][];

    public YM2612State()
    {
        super();

        registers = new int[2][256];
        init = new boolean[2][256];

        clear();
    }

    public YM2612State(YM2612State source)
    {
        super();

        registers = new int[2][256];
        init = new boolean[2][256];

        for (int i = 0; i < 0x100; i++)
        {
            registers[0][i] = source.registers[0][i];
            registers[1][i] = source.registers[1][i];
            init[0][i] = source.init[0][i];
            init[1][i] = source.init[1][i];
        }
    }

    public void clear()
    {
        for (int i = 0; i < 0x20; i++)
        {
            registers[0][i] = 0;
            registers[1][i] = 0;
            init[0][i] = false;
            init[1][i] = false;
        }
        for (int i = 0x20; i < 0x100; i++)
        {
            registers[0][i] = 0;
            registers[1][i] = 0;
            init[0][i] = false;
            init[1][i] = false;
        }
    }

    public int get(int port, int reg)
    {
        if (canIgnore(port, reg))
            return 0;

        return registers[port][reg];
    }

    public boolean set(int port, int reg, int value)
    {
        if (canIgnore(port, reg))
            return false;

        int newValue = value;

        if (port == 0)
        {
            // special case of KEY ON/OFF register
            if (reg == 0x28)
            {
                // invalid channel number ? --> ignore
                if ((value & 3) == 3)
                    return false;

                int oldValue = registers[port][value & 7];
                newValue &= 0xF0;

                if (oldValue != newValue)
                {
                    registers[port][value & 7] = newValue;
                    // always write when key state change
                    return true;
                }

                return false;
            }

            // special case of Timer register --> ignore useless bits
            if (reg == 0x27)
                newValue &= 0xC0;
        }

        if (!init[port][reg] || (registers[port][reg] != newValue))
        {
            registers[port][reg] = value;
            init[port][reg] = true;
            return true;
        }

        return false;
    }

    public boolean isDiff(YM2612State state, int port, int reg)
    {
        if (!state.init[port][reg])
            return false;
        if (canIgnore(port, reg))
            return false;
        if (!init[port][reg])
            return true;

        return get(port, reg) != state.get(port, reg);
    }

    /**
     * Return true if write can be ignored
     */
    public static boolean canIgnore(int port, int reg)
    {
        switch (reg)
        {
            case 0x22:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x2B:
                return (port == 1);
        }

        if ((reg >= 0x30) && (reg < 0xB8))
            return ((reg & 3) == 3);

        return true;
    }

    /**
     * Return the dual entry for this reg (or null if no dual entry)
     */
    public static int[] getDualReg(int reg)
    {
        for (int i = 0; i < duals.length; i++)
        {
            final int[] dual = duals[i];

            if ((dual[0] == reg) || (dual[1] == reg))
                return dual;
        }

        return null;
    }

    /**
     * Returns commands list to update to the specified YM2612 state
     */
    public List<VGMCommand> getDelta(YM2612State state, boolean ignoreTimerWrites)
    {
        final List<VGMCommand> result = new ArrayList<VGMCommand>();

        // do dual reg first
        for (int i = 0; i < duals.length; i++)
        {
            final int[] dual = duals[i];
            final int reg0 = dual[0];
            final int reg1 = dual[1];

            // value is different
            if (isDiff(state, 0, reg0) || isDiff(state, 0, reg1))
            {
                // add commands
                result.add(VGMCommand.createYMCommand(0, reg0, state.get(0, reg0)));
                result.add(VGMCommand.createYMCommand(0, reg1, state.get(0, reg1)));
            }
            // port 1 too ?
            if (dual[0] > 0x30)
            {
                if (isDiff(state, 1, reg0) || isDiff(state, 1, reg1))
                {
                    // add commands
                    result.add(VGMCommand.createYMCommand(1, reg0, state.get(1, reg0)));
                    result.add(VGMCommand.createYMCommand(1, reg1, state.get(1, reg1)));
                }
            }
        }

        for (int port = 0; port < 2; port++)
        {
            for (int reg = 0; reg < 0x30; reg++)
            {
                // can ignore or special case of KEY ON/OFF register
                if (canIgnore(port, reg) || ((port == 0) && (reg == 0x28)))
                    continue;
                // ignore timer write
                if (ignoreTimerWrites && ((port == 0) && ((reg == 0x24) || (reg == 0x25) || (reg == 0x26))))
                    continue;
                // ignore timer write (special case of 0X27 register)
                if (ignoreTimerWrites && ((port == 0) && (reg == 0x27)))
                {
                    // same value for spe/CSM mode ? --> ignore
                    if ((registers[0][0x27] & 0xC0) == (state.registers[0][0x27] & 0xC0))
                        continue;
                }
                // ignore dual reg
                if (getDualReg(reg) != null)
                    continue;

                // value is different --> add command
                if (isDiff(state, port, reg))
                    result.add(VGMCommand.createYMCommand(port, reg, state.get(port, reg)));
            }

            for (int reg = 0x30; reg < 0xA0; reg += 0x10)
            {
                for (int ch = 0; ch < 3; ch++)
                {
                    for (int sl = 0; sl < 0x10; sl += 4)
                    {
                        final int r = reg + sl + ch;

                        // can ignore
                        if (canIgnore(port, r))
                            continue;
                        // ignore dual reg
                        if (getDualReg(r) != null)
                            continue;

                        // value is different --> add command
                        if (isDiff(state, port, r))
                            result.add(VGMCommand.createYMCommand(port, r, state.get(port, r)));
                    }
                }
            }

            for (int reg = 0xA0; reg < 0xB8; reg += 4)
            {
                for (int ch = 0; ch < 3; ch++)
                {
                    final int r = reg + ch;

                    // can ignore
                    if (canIgnore(port, r))
                        continue;
                    // ignore dual reg
                    if (getDualReg(r) != null)
                        continue;

                    // value is different --> add command
                    if (isDiff(state, port, r))
                        result.add(VGMCommand.createYMCommand(port, r, state.get(port, r)));
                }
            }
        }

        return result;
    }

    public void updateState(List<XGMFMCommand> commands)
    {
        int port;
        int ch;
        int sl;
        int reg;
        int v;

        for (XGMFMCommand com : commands)
        {
            switch (com.getType())
            {
                case XGMFMCommand.FM_LFO:
                    set(0, 0x22, com.data[1]);
                    break;
                case XGMFMCommand.FM_CH3_SPECIAL_ON:
                    set(0, 0x27, (get(0, 0x27) & 0x3F) | 0x40);
                    break;
                case XGMFMCommand.FM_CH3_SPECIAL_OFF:
                    set(0, 0x27, get(0, 0x27) & 0x3F);
                    break;
                case XGMFMCommand.FM_DAC_ON:
                    set(0, 0x2B, (get(0, 0x2B) & 0x7F) | 0x80);
                    break;
                case XGMFMCommand.FM_DAC_OFF:
                    set(0, 0x2B, get(0, 0x2B) & 0x7F);
                    break;

                case XGMFMCommand.FM0_PAN:
                case XGMFMCommand.FM1_PAN:
                    port = (com.getType() == XGMFMCommand.FM0_PAN) ? 0 : 1;
                    ch = com.data[0] & 0x03;
                    v = (com.data[0] & 0x0C) << 4;
                    set(port, 0xB4 + ch, (get(port, 0xB4 + ch) & 0x3F) | v);
                    break;

                case XGMFMCommand.FM_WRITE:
                    port = (com.data[0] >> 3) & 1;
                    // num write
                    v = (com.data[0] & 0x07) + 1;
                    for (int i = 0; i < v; i++)
                        set(port, Util.getInt8(com.data, (i * 2) + 1), Util.getInt8(com.data, (i * 2) + 2));
                    break;

                case XGMFMCommand.FM_LOAD_INST:
                    port = (com.data[0] >> 2) & 1;
                    ch = com.data[0] & 0x03;
                    // data offset
                    v = 1;
                    // slot writes
                    for (int r = 0; r < 7; r++)
                        for (sl = 0; sl < 4; sl++)
                            set(port, 0x30 + (r << 4) + (sl << 2) + ch, Util.getInt8(com.data, v++));
                    // ch writes
                    set(port, 0xB0 + ch, Util.getInt8(com.data, v++));
                    set(port, 0xB4 + ch, Util.getInt8(com.data, v++));
                    break;

                case XGMFMCommand.FM_TL:
                    port = com.data[1] & 1;
                    ch = com.data[0] & 3;
                    sl = (com.data[0] >> 2) & 3;
                    reg = 0x40 + (sl << 2) + ch;
                    v = (com.data[1] >> 1) & 0x7F;
                    set(port, reg, v);
                    break;

                case XGMFMCommand.FM_TL_DELTA:
                case XGMFMCommand.FM_TL_DELTA_WAIT:
                    port = com.data[1] & 1;
                    ch = com.data[0] & 3;
                    sl = (com.data[0] >> 2) & 3;
                    reg = 0x40 + (sl << 2) + ch;
                    v = (com.data[1] >> 2) & 0x3F;
                    if ((com.data[1] & 2) != 0) v = -v;
                    // apply delta
                    set(port, reg, get(port, reg) + v);
                    break;

                case XGMFMCommand.FM_FREQ:
                case XGMFMCommand.FM_FREQ_WAIT:
                    port = (com.data[0] >> 2) & 1;
                    ch = com.data[0] & 3;
                    // special mode ?
                    if ((com.data[0] & 8) != 0)
                    {
                        // high byte first
                        set(port, 0xA6 + ch, com.data[2] & 0x3F);
                        // then low byte
                        set(port, 0xA2 + ch, com.data[1] & 0xFF);
                    }
                    else
                    {
                        // high byte first
                        set(port, 0xA4 + ch, com.data[2] & 0x3F);
                        // then low byte
                        set(port, 0xA0 + ch, com.data[1] & 0xFF);
                    }
                    break;

                case XGMFMCommand.FM_FREQ_DELTA:
                case XGMFMCommand.FM_FREQ_DELTA_WAIT:
                    port = (com.data[0] >> 2) & 1;
                    ch = com.data[0] & 3;
                    reg = (((com.data[0] & 8) != 0) ? 0x40 : 0x42) + ch;
                    v = (com.data[1] >> 1) & 0x7F;
                    if ((com.data[1] & 1) != 0) v = -v;
                    
                    // apply delta
                    v = (((get(port, reg + 4) & 0x3F) << 8) | (get(port, reg + 0) & 0xFF)) + v; 
                    // high byte first
                    set(port, reg + 4, (v >> 8) & 0x3F);
                    // then low byte
                    set(port, reg + 0, (v >> 0) & 0xFF);
                    break;

                case XGMFMCommand.FM_KEY:
                    ch = com.data[0] & 7;
                    // key on/off
                    set(0, 0x28, (((com.data[0] & 8) != 0) ? 0xF0 : 0x00) + ch);
                    break;

                case XGMFMCommand.FM_KEY_SEQ:
                    ch = com.data[0] & 7;
                    // key seq - on/off or off/on
                    set(0, 0x28, (((com.data[0] & 8) != 0) ? 0x00 : 0xF0) + ch);
                    break;

                case XGMFMCommand.FM_KEY_ADV:
                    set(0, 0x28, com.data[1]);
                    break;

                default:
                    // do nothing
                    break;
            }
        }
    }
}
