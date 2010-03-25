// std.dynamic

void override(auto ref a, b);
void override(auto ref function, char[] code);

void eval_stub(){}

void eval(char[] code)
{
	override(eval_stub, code);
	eval_stub();
}
