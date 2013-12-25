#pragma once

#ifdef NULLC_BUILD_X86_JIT
// Implementations of some functions, called from user binary code

int intPow(int power, int number)
{
	// http://en.wikipedia.org/wiki/Exponentiation_by_squaring
	if(power < 0)
		return number == 1 ? 1 : 0;

	int result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power /= 2;
	}
	return result;
}

double doublePow(double a, double b)
{
	return pow(b, a);
}

double doubleMod(double a, double b)
{
	return fmod(b, a);
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

#ifdef __linux

long long longPow(long long power, long long number)
{
	if(power < 0)
		return 0;
	long long result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

#else

__declspec(naked) void longPow()
{
	// ���, ����� ��������� � 8 ������ � [esp+12]
	// ������� ��������� � 8 ������ � [esp+4] (������� �������� eip)
	__asm
	{
		// ��������� �� �����, ���� ��� ����� 1 ��� 0, ������ ��� �������
		cmp dword ptr [esp+16], 0 ;
		jne greaterThanOne ;
		cmp dword ptr [esp+12], 1 ;
		jg greaterThanOne ;
		mov eax, dword ptr [esp+12] ;
		xor edx, edx ;
		ret ;

		greaterThanOne:
		// ��������� �� �������, ��� ������ ���� �� ������ 63, ����� ����� 0
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
		push esi ; // �������� ��� ��� ��������, ������ ��� ��� ����� ������� ����� �������� �����
		// ����: numHigh, numLow, expHigh, expLow, EIP,  EDI,  ESI
		//       esp+24   esp+20  esp+16   esp+12  esp+8 esp+4 esp

		// ����� ������� � edi:esi
		mov edi, dword ptr [esp+24] ;
		mov esi, dword ptr [esp+20] ;
		// ��������� ���� ����� ������������ �� �������� �����, ��� ���� �������
		push 0 ;
		push 1 ; // ��������� ���� ����� 1
		whileStart: ;
		mov eax, dword ptr [esp+12+8] ;
		or eax, eax ;//dword ptr [esp+16+8] ; //while(power)
		jz whileEnd ;
		  mov eax, dword ptr [esp+12+8]
		  test eax, 1 ;
		  jz skipIf ;
			push edi ;
			push esi ; // ������ � ����� ������� ��������� � �����
			call longMul ;
			mov dword ptr [esp+8], eax ;
			mov dword ptr [esp+12], edx ; // �������� ���������
			add esp, 8 ; // �������� ������� ������ ���������
			sub dword ptr [esp+12+8], 1 ;
			//sbb dword ptr [esp+16+8], 0 ; //power--;
		  skipIf: ;
			push edi ;
			push esi ; // �������� ����� � ����
			xchg edi, dword ptr [esp+12] ;
			xchg esi, dword ptr [esp+8] ; // �������� ����� � ���������, ������ ����� � ����� ������
			call longMul ; // number *= number
			mov dword ptr [esp+8], eax ;
			mov dword ptr [esp+12], edx ; // �������� ���������
			add esp, 8 ; // �������� ������� ������ ������� �����
			xchg edi, dword ptr [esp+4] ;
			xchg esi, dword ptr [esp] ; // �������� ����� � ���������, ��������� ����� � �����
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

#endif
