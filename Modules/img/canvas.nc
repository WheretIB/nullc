import img.canvas_ex;

Canvas Canvas(int width, height)
{
	Canvas ret;
	Canvas(&ret, width, height);
	return ret;
}

void Canvas:Clear(char red, green, blue);
void Canvas:Clear(char red, green, blue, alpha);

void Canvas:SetColor(char red, green, blue);

void Canvas:DrawLine(int x1, y1, x2, y2);
void Canvas:DrawRect(int x1, y1, x2, y2);

void Canvas:Destroy();
