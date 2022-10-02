class File
{
	int flag;
	void ref id;
}
enum Seek
{
	SET,
	CUR,
	END
}

// Create an empty file object
File File();

// Open a file with the specified access
// Use Opened function to tell if the operation was successful
File File(char[] name, char[] access);

// Open a file with the specified access. Previous file will be closed.
// Use Opened function to tell if the operation was successful
void File::Open(char[] name, char[] access);

// Close the file
void File::Close();

// Check if the file is opened
bool File::Opened();
// Check if the file has ended
bool File::Eof();

// Seek inside the file from the 3 available origin points
void File::Seek(Seek origin, int shift = 0);
// Receive the current position inside the file
long File::Tell();

// Get the file size
long File::Size();

// Write a basic type value to the file (binary)
// functions returns true on success
// if the file was not opened or was closed, these functions will throw an error
bool File::Write(char data);
bool File::Write(short data);
bool File::Write(int data);
bool File::Write(long data);
bool File::Write(float data);
bool File::Write(double data);

// Read a basic type value from the file (binary)
// functions returns true on success
// if the file was not opened or was closed, these functions will throw an error
bool File::Read(char ref data);
bool File::Read(short ref data);
bool File::Read(int ref data);
bool File::Read(long ref data);
bool File::Read(float ref data);
bool File::Read(double ref data);

// Read an array of bytes from the file (array size wil tell how many bytes are read)
// function returns the number of bytes successfully read
// if the file was not opened or was closed, this function will throw an error
int File::Read(char[] arr);

// Write an array of bytes to the file (array size wil tell how many bytes are written)
// function returns the number of bytes successfully written
// if the file was not opened or was closed, this function will throw an error
int File::Write(char[] arr);

// Print a sequence on bytes to the file, stopping at the zero-termination symbol or at the end of the array
// function returns true if the sequence was fully written
// if the file was not opened or was closed, this function will throw an error
bool File::Print(char[] arr);

// Read the specified number of bytes into the array at a specified offset
// function returns the number of bytes successfully read
// if the array size is not enough to fit the number of bytes from the file at the specified offset, the function will throw an error
// if the file was not opened or was closed, this function will throw an error
int File::Read(char[] arr, int offset, int bytes);

// Write the specified number of bytes from the array at a specified offset
// function returns the number of bytes successfully written
// if the array size is not enough to get the number of bytes from the it at the specified offset, the function will throw an error
// if the file was not opened or was closed, this function will throw an error
int File::Write(char[] arr, int offset, int bytes);
