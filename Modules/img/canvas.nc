
class Canvas
{
	int width, height;
	
	float[4] color;
	float[] data;
	int[] dataI;
	
	bool lineAA;
}

Canvas Canvas(int width, height)
{
	Canvas ret;

	ret.data = new float[width*height*4];
	ret.dataI = new int[width*height];
	ret.width = width;
	ret.height = height;

	return ret;
}

void Canvas::Clear(float red, green, blue);
void Canvas::Clear(float red, green, blue, alpha);

void Canvas::SetColor(char red, green, blue);
void Canvas::SetAA(bool enable);

void Canvas::DrawLine(double x1, y1, x2, y2);
void Canvas::DrawRect(int x1, y1, x2, y2);
void Canvas::DrawRect(int x1, y1, x2, y2, double alpha);
void Canvas::DrawPoint(double x, y);
void Canvas::DrawPoint(double x, y, alpha);

float[] Canvas::GetData()
{
	return data;
}
