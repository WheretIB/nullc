class File
{
	int flag;
	void ref id;
}

File File();
File File(char[] name, char[] access);
void File:Open(char[] name, char[] access);
void File:Close();

int File:Opened();

void File:Write(char data);
void File:Write(short data);
void File:Write(int data);
void File:Write(long data);
void File:Write(float data);
void File:Write(double data);

void File:Read(char ref data);
void File:Read(short ref data);
void File:Read(int ref data);
void File:Read(long ref data);
void File:Read(float ref data);
void File:Read(double ref data);

void File:Read(char[] arr);
void File:Write(char[] arr);
void File:Print(char[] arr);
