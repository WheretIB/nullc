import win.window_ex;
import img.canvas;

Window Window(char[] title, int x, y, width, height)
{
	Window ret;
	Window(&ret, title, x, y, width, height);
	return ret;
}

void Window::SetTitle(char[] title);
void Window::SetPosition(int x, y);
void Window::SetSize(int width, height);

int Window::GetPosX(){ return x; }
int Window::GetPosY(){ return y; }

void Window::DrawCanvas(Canvas ref c, int x, y);

void Window::Update();

void Window::Close();
