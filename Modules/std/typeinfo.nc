class typeid
{
	int id;

	int memberCount();
	typeid memberType(int member);
	char[] memberName(int member);
}
char[] typename(auto ref type);
typeid typeid(auto ref type);

int isFunction(typeid type);
int isClass(typeid type);
int isSimple(typeid type);
int isArray(typeid type);
int isPointer(typeid type);

int isFunction(auto ref type);
int isClass(auto ref type);
int isSimple(auto ref type);
int isArray(auto ref type);
int isPointer(auto ref type);
