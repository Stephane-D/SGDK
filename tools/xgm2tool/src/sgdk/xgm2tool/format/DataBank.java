package sgdk.xgm2tool.format;

public class DataBank
{
    final public int id;
    final public byte[] data;

    public DataBank(VGMCommand command)
    {
        super();

        if (!command.isDataBlock())
            throw new IllegalArgumentException(String.format("Incorrect sample data declaration at %6X !",
                    Integer.valueOf(command.getOriginOffset())));

        // id
        id = command.getDataBankId();
        // allocate data
        data = new byte[command.getDataBlockLen()];

        // copy bank data
        System.arraycopy(command.data, 7, data, 0, data.length);
    }

    public byte getSample(int offset)
    {
        return data[offset];
    }
}
