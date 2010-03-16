
class Canvas
{
	int width, height;
	
	int color;
	int[] data;
}

Canvas Canvas(int width, height)
{
	Canvas ret;
	
	ret.color = 0;
	ret.data = new int[width*height];
	ret.width = width;
	ret.height = height;

	return ret;
}

void Canvas:Clear(char red, green, blue);
void Canvas:Clear(char red, green, blue, alpha);

void Canvas:SetColor(char red, green, blue);

void Canvas:DrawLine(int x1, y1, x2, y2);
void Canvas:DrawRect(int x1, y1, x2, y2);
void Canvas:DrawPoint(int x, y);

int[] Canvas:GetData()
{
	return data;
}
