namespace memory
{
	bool		read_bool(char[] buffer, int offset);
	char		read_char(char[] buffer, int offset);
	short		read_short(char[] buffer, int offset);
	int			read_int(char[] buffer, int offset);
	long		read_long(char[] buffer, int offset);
	float		read_float(char[] buffer, int offset);
	double		read_double(char[] buffer, int offset);

	bool[]		read_bool_array(char[] buffer, int offset, int elements);
	char[]		read_char_array(char[] buffer, int offset, int elements);
	short[]		read_short_array(char[] buffer, int offset, int elements);
	int[]		read_int_array(char[] buffer, int offset, int elements);
	long[]		read_long_array(char[] buffer, int offset, int elements);
	float[]		read_float_array(char[] buffer, int offset, int elements);
	double[]	read_double_array(char[] buffer, int offset, int elements);

	void read(char[] buffer, int offset, bool ref value);
	void read(char[] buffer, int offset, char ref value);
	void read(char[] buffer, int offset, short ref value);
	void read(char[] buffer, int offset, int ref value);
	void read(char[] buffer, int offset, long ref value);
	void read(char[] buffer, int offset, float ref value);
	void read(char[] buffer, int offset, double ref value);

	void read(char[] buffer, int offset, bool[] value);
	void read(char[] buffer, int offset, char[] value);
	void read(char[] buffer, int offset, short[] value);
	void read(char[] buffer, int offset, int[] value);
	void read(char[] buffer, int offset, long[] value);
	void read(char[] buffer, int offset, float[] value);
	void read(char[] buffer, int offset, double[] value);

	void write(char[] buffer, int offset, bool value);
	void write(char[] buffer, int offset, char value);
	void write(char[] buffer, int offset, short value);
	void write(char[] buffer, int offset, int value);
	void write(char[] buffer, int offset, long value);
	void write(char[] buffer, int offset, float value);
	void write(char[] buffer, int offset, double value);

	void write(char[] buffer, int offset, bool[] value);
	void write(char[] buffer, int offset, char[] value);
	void write(char[] buffer, int offset, short[] value);
	void write(char[] buffer, int offset, int[] value);
	void write(char[] buffer, int offset, long[] value);
	void write(char[] buffer, int offset, float[] value);
	void write(char[] buffer, int offset, double[] value);

	void copy(char[] dst, int dstOffset, char[] src, int srcOffset, int size);
	void set(char[] dst, int dstOffset, char value, int size);
	int compare(char[] lhs, int lhsOffset, char[] rhs, int rhsOffset, int size);

	float	as_float(int value);
	double	as_double(long value);
	int		as_int(float value);
	long	as_long(double value);
}
