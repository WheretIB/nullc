#pragma once

#ifdef NULLC_BUILD_X86_JIT
// Implementations of some functions, called from user binary code

__declspec(naked) void intPow()
{
	// http://en.wikipedia.org/wiki/Exponentiation_by_squaring
	//eax is the power
	//ebx is the number
	__asm
	{
		mov eax, [esp+4] ;
		mov ebx, [esp+8] ;
		mov edx, 1 ; //edx is the result
		whileStart: ; 
		cmp eax, 0 ; //while(power)
		jz whileEnd ;
		  test eax, 1 ; // if(power & 1)
		  je skipIf ;
			imul edx, ebx ; // result *= number
			sub eax, 1 ; //power--;
		  skipIf: ;
			imul ebx, ebx ; // number *= number
			shr eax, 1 ; //power /= 2
		  jmp whileStart ;
		whileEnd: ;
		ret ;
	}
}

__declspec(naked) void doublePow()
{
	// x**y
	// st0 == x
	// st1 == y
	__asm
	{
		push eax ; // сюда положим флаг контроля fpu
		mov word ptr [esp+2], 1F7Fh ; //сохраним свой с окурглением к 0

		fstcw word ptr [esp] ; //сохраним флаг контроля
		fldcw word ptr [esp+2] ; //установим его

		fldz ;
		fcomip st,st(1) ; // сравним x с 0.0
		je Zero ; // если так, вернём 0.0

		fyl2x ; //st0 = y*log2(x)
		fld st(0) ; //скопируем
		frndint ; //округлим вниз
		fsubr st, st(1) ; //st1=y*log2(x), st0=fract(y*log2(x))
		f2xm1 ; //st0=2**(fract(y*log2(x)))-1
		fld1 ; //положили один
		faddp st(1), st; //прибавили, st0=2**(fract(y*log2(x)))
		fscale ; //находим st0=2**(y*log2(x))

		Zero:
		fxch st(1) ; //поменяем st0 и st1
		fstp st(0) ; //уберём st0
		
		fldcw word ptr [esp] ; //востановим флаг контроля
		pop eax ;
		ret
	}
}

long long longMul(long long a, long long b)
{
	return b * a;
}

long long longDiv(long long a, long long b)
{
	return b / a;
}

long long longMod(long long a, long long b)
{
	return b % a;
}

long long longShl(long long a, long long b)
{
	return b << a;
}

long long longShr(long long a, long long b)
{
	return b >> a;
}

__declspec(naked) void longPow()
{
	// Так, число находится в 8 байтах с [esp+12]
	// Степень находится в 8 байтах с [esp+4] (вершину занимает eip)
	__asm
	{
		// Посмотрим на число, если оно равно 1 или 0, выйдем без расчёта
		cmp dword ptr [esp+16], 0 ;
		jne greaterThanOne ;
		cmp dword ptr [esp+12], 1 ;
		jg greaterThanOne ;
		mov eax, dword ptr [esp+12] ;
		xor edx, edx ;
		ret ;

		greaterThanOne:
		// Посмотрим на степень, она должна быть не больше 63, иначе вернём 0
		cmp dword ptr [esp+8], 0 ;
		jne returnZero ;
		cmp dword ptr [esp+4], 63 ;
		jle fullPower ;

		returnZero:
		xor eax, eax ;
		xor edx, edx ;
		ret ;

		fullPower:
		push edi ;
		push esi ; // Сохраним эти два регистра, потому что они будут держать более полезное число
		// стек: numHigh, numLow, expHigh, expLow, EIP,  EDI,  ESI
		//       esp+24   esp+20  esp+16   esp+12  esp+8 esp+4 esp

		// Число высунем в edi:esi
		mov edi, dword ptr [esp+24] ;
		mov esi, dword ptr [esp+20] ;
		// Результат пока будет распологатся на верхушке стека, где была степень
		push 0 ;
		push 1 ; // результат пока равен 1
		whileStart: ;
		mov eax, dword ptr [esp+12+8] ;
		or eax, eax ;//dword ptr [esp+16+8] ; //while(power)
		jz whileEnd ;
		  mov eax, dword ptr [esp+12+8]
		  test eax, 1 ;
		  jz skipIf ;
			push edi ;
			push esi ; // теперь в стеке текущий результат и число
			call longMul ;
			mov dword ptr [esp+8], eax ;
			mov dword ptr [esp+12], edx ; // Обновили результат
			add esp, 8 ; // оставили наверху только результат
			sub dword ptr [esp+12+8], 1 ;
			//sbb dword ptr [esp+16+8], 0 ; //power--;
		  skipIf: ;
			push edi ;
			push esi ; // положили число в стек
			xchg edi, dword ptr [esp+12] ;
			xchg esi, dword ptr [esp+8] ; // поменяли число и результат, теперь число в стеке дважды
			call longMul ; // number *= number
			mov dword ptr [esp+8], eax ;
			mov dword ptr [esp+12], edx ; // Обновили результат
			add esp, 8 ; // оставили наверху только квадрат числа
			xchg edi, dword ptr [esp+4] ;
			xchg esi, dword ptr [esp] ; // поменяли число и результат, результат опять в стеке
			shr dword ptr [esp+12+8], 1 ; //power /= 2
			//mov eax, dword ptr [esp+16+8]
			//shrd dword ptr [esp+12+8], eax, 1 ; //power /= 2
		  jmp whileStart ;
		whileEnd: ;
		mov edx, dword ptr [esp+4] ;
		mov eax, dword ptr [esp] ;
		add esp, 8 ;
		pop esi ;
		pop edi ;
		ret ;
	}
}

#endif
