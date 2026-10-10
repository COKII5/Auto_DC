namespace AutoDc.Protocol
{
    public static class Crc8
    {
        private const byte Polynomial = 0x07;
        private const byte HighBit = 0x80;
        private const int BitsPerByte = 8;

        public static byte Compute(byte[] bytes, int count)
        {
            byte crc = 0x00;
            for (int index = 0; index < count; index++)
            {
                crc ^= bytes[index];
                for (int bit = 0; bit < BitsPerByte; bit++)
                {
                    crc = (crc & HighBit) != 0 ? (byte)((crc << 1) ^ Polynomial) : (byte)(crc << 1);
                }
            }
            return crc;
        }
    }
}
