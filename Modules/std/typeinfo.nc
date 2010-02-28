
int typeid:memberCount();
typeid typeid:memberType(int member);
char[] typeid:memberName(int member);
	
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

int typeid.size();
char[] typeid.name();
int operator==(typeid a, b);
