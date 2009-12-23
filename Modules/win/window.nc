import img.canvas;

class Window
{
	char[] title;

	int x, y;
	int width, height;
	
	int handle;
}

void Window(Window ref wnd, char[] title, int x, y, width, height);

Window Window(char[] title, int x, y, width, height)
{
	Window ret;
	Window(&ret, title, x, y, width, height);
	return ret;
}

void Window:SetTitle(char[] title);
void Window:SetPosition(int x, y);
void Window:SetSize(int width, height);

void Window:DrawCanvas(Canvas ref c, int x, y);

void Window:Close();
