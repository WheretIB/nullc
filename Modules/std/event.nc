// Users of the event class must also import the std.list module
import std.list;

// Template argument is the function type
class event<T>
{
	list<T> callbacks;
}

// Event call operators for various argument types
void operator()(event<@T> ref e)
{
	for(i in e.callbacks)
		i();
}

void operator()(event<@T> ref e, @A1 a1)
{
	for(i in e.callbacks)
		i(a1);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2)
{
	for(i in e.callbacks)
		i(a1, a2);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3)
{
	for(i in e.callbacks)
		i(a1, a2, a3);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11, @A12 a12)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11, @A12 a12, @A13 a13)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11, @A12 a12, @A13 a13, @A14 a14)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11, @A12 a12, @A13 a13, @A14 a14, @A15 a15)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
}

void operator()(event<@T> ref e, @A1 a1, @A2 a2, @A3 a3, @A4 a4, @A5 a5, @A6 a6, @A7 a7, @A8 a8, @A9 a9, @A10 a10, @A11 a11, @A12 a12, @A13 a13, @A14 a14, @A15 a15, @A16 a16)
{
	for(i in e.callbacks)
		i(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);
}

// Check if the event has some callbacks
bool bool(event<@T> ref e)
{
	return !e.callbacks.empty();
}

// Attach the callback to the event (a single callback can be attached only once)
void event::attach(T callback)
{
	if(callback in callbacks)
		return;
	callbacks.push_back(callback);
}

void operator+=(event<@T> ref e, @T callback)
{
    e.attach(callback);
}

// Detach the callback from the event
void event::detach(T callback)
{
	auto it = callbacks.find(callback);
	if(it)
		callbacks.erase(it);
}

void operator-=(event<@T> ref e, @T callback)
{
	e.detach(callback);
}

// Detach all callbacks
void event::detach_all()
{
	callbacks.clear();
}

// Short-hand for a generic argument list type
typedef auto ref[] EventArgs;

// Short-hand for a generic event handler function type
typedef void ref(auto ref, EventArgs) EventHandler;
