#pragma once

// Implementations of some functions, called from user binary code

__declspec(naked) void intPow()
{
	// http://en.wikipedia.org/wiki/Exponentiation_by_squaring
	//eax is the power
	//ebx is the number
	__asm
	{
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
		push eax ; // ���� ������� ���� �������� fpu
		mov word ptr [esp+2], 1F7Fh ; //�������� ���� � ����������� � 0

		fstcw word ptr [esp] ; //�������� ���� ��������
		fldcw word ptr [esp+2] ; //��������� ���

		fldz ;
		fcomip st,st(1) ; // ������� x � 0.0
		je Zero ; // ���� ���, ����� 0.0

		fyl2x ; //st0 = y*log2(x)
		fld st(0) ; //���������
		frndint ; //�������� ����
		fsubr st, st(1) ; //st1=y*log2(x), st0=fract(y*log2(x))
		f2xm1 ; //st0=2**(fract(y*log2(x)))-1
		fld1 ; //�������� ����
		faddp st(1), st; //���������, st0=2**(fract(y*log2(x)))
		fscale ; //������� st0=2**(y*log2(x))

		Zero:
		fxch st(1) ; //�������� st0 � st1
		fstp st ; //����� st0
		
		fldcw word ptr [esp] ; //���������� ���� ��������
		pop eax ;
		ret
	}
}

// ���������� ����� �� llmul.asm
__declspec(naked) void longMul()
{
	// (aHigh*2^32+aLow)*(bHigh*2^32+bLow) =
	// aHigh*bHigh*2^64 + aLow*bHigh*2^32 + bLow*aHigh*2^32 + bLow*aLow =
	// aLow*bHigh*2^32 + bLow*aHigh*2^32 + bLow*aLow.
	//
	// esp+16 esp+12 esp+8 esp+4 esp
	// bHigh  bLow   aHigh aLow  EIP
	__asm
	{
		mov eax, dword ptr [esp+8] ; // aHigh
		mov ecx, dword ptr [esp+16] ; // bHigh
		or ecx, eax ; // ���������, �� ����� �� ������� ���� ����
		mov ecx, dword ptr [esp+12] ; // � ����� ������ ecx ������ bLow
		jnz fullMult ; // ���� ��� �� ���, ����� ������ ���������
		// ����� ������� �������� �������
		mov eax, dword ptr [esp+4] ; // aLow
		mul ecx ; // ���������� ������ dword'�
		ret ; // � ������

		fullMult:
		mul ecx ; // aHigh*bLow
		mov ebx, eax ; // ��������

		mov eax, dword ptr [esp+4] ; // aLow
		mul dword ptr [esp+16] ; // bHigh
		add ebx, eax ; // ebx = aHigh*bLow + aLow*bHigh

		mov eax, dword ptr [esp+4] ; // aLow
		mul ecx ; // aLow*bLow ������ � eax � ������ � edx
		add edx, ebx ; // �������� ��� aHigh*bLow � aLow*bHigh

		ret ; // ������
	}
}

// ���������� ����� �� lldiv.asm
// ��� ��������� ������, ��� �������, ��� ����
__declspec(naked) void longDiv()
{
	__asm
	{
		push    edi
        push    esi
        push    ebx

		; // �������� ����� ������������ ���������. ����� �����, ���� ����� ��������� ��������� �������:
		; //              -----------------
		; //              |               |
		; //              |---------------|
		; //              |               |+28
		; //              |--������� (b)--|
		; //              |               |+24
		; //              |---------------|
		; //              |               |+20
		; //              |--�������� (a)-|
		; //              |               |+16
		; //              |---------------|
		; //              | return addr** |+12
		; //              |---------------|
		; //              |      EDI      |+8
		; //              |---------------|
		; //              |      ESI      |+4
		; //              |---------------|
		; //      ESP---->|      EBX      |
		; //              -----------------

		; // ��������� ���� ���������� (edi = 0 ��� 2 ���� ��������� �������������, ����� �������������)
		; // ������� ��� �������� �����������.

        xor     edi, edi         ; // ����������� ��� �� �������������

        mov     eax, dword ptr [esp+28] ; // ������� ������� ����� ��������
        or      eax, eax		; // ��������, ����� � ���� ����
        jge     L1				; // ��������� �������� ���� �� �����������
        inc     edi				; // �������� ���� ����������
        mov     edx, dword ptr [esp+24] ; // ������� ������� ����� ��������
        neg     eax				; // ������� �������������
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+28], eax ; // �������� ������������� �����
        mov     dword ptr [esp+24], edx
L1:
        mov     eax, dword ptr [esp+20] ; // ������� ������� ����� ��������
        or      eax, eax		; // ��������, ����� � ���� ����
        jge     L2				; // ��������� �������� ���� �� �����������
        inc     edi             ; // �������� ���� ����������
        mov     edx, dword ptr [esp+16] ; // ������� ������� ����� ��������
        neg     eax             ; // ������� �������������
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+20], eax ; // �������� ������������� �����
        mov     dword ptr [esp+16], edx
L2:

		; // ������ �������. ��������� �������, ����� �������� ������ 4194304 �����.
		; // ���� ���, ����� ��������� ������� ���������� � ������� ��������
		; // � ��������� ������, ���� ���������� ������� �������
		; // ������� - eax ������ �������� ������� ������� ����� ��������

        or      eax, eax		; // ���������, ������ �� �� 4194304 �����
        jnz     L3				; // ���, ������� ���� ������ ����
        mov     ecx, dword ptr [esp+16] ; // �������� ��������
        mov     eax, dword ptr [esp+28] ; // �������� ������� dword ��������
        xor     edx, edx
        div     ecx             ; // eax <- ������� dword ��������
        mov     ebx, eax        ; // �������� ������� dword ��������
        mov     eax, dword ptr [esp+24] ; // edx:eax <- �������:������� dword ��������
        div     ecx             ; // eax <- ������� dword ��������
        mov     edx, ebx        ; // edx:eax <- �������
        jmp     short L4        ; // ���������� ����, ����������� ���� � ��������

		; // ��� � ��� ������� �������. eax �� ��� �������� ������� ������� ����� ��������

L3:
        mov     ebx, eax         ; // ebx:ecx <- ��������
        mov     ecx, dword ptr [esp+16]
        mov     edx, dword ptr [esp+28] ; // edx:eax <- �������
        mov     eax, dword ptr [esp+24]
L5:
        shr     ebx, 1           ; // ������� �������� ������ �� ���
        rcr     ecx, 1
        shr     edx, 1           ; // ������� ������� ������ �� ���
        rcr     eax, 1
        or      ebx, ebx
        jnz     short L5        ; // ����� ��� ���������, ���� �������� �� ������ ������ 4194304 �����
        div     ecx             ; // ������ ��������, �������������� �������
        mov     esi, eax         ; // �������� �������

		; // �� ����� ��������� �� �������, ������� ����� ���������, �� ������� �������
		; // �� �������� � ������� ��������� � �������
		; // ����� ��� ���� ��������� ������������, ������� ����� �������� � ������, �����
		; // ������� ������ � 2**64 � ������� �������� �� 1.

        mul     dword ptr [esp+20] ; // ������� * ������� ������� ����� ��������
        mov     ecx, eax
        mov     eax, dword ptr [esp+16]
        mul     esi             ; // ������� * ������� ������� ����� ��������
        add     edx, ecx         ; // edx:eax = ������� * ��������
        jc      short L6        ; // ������� ����� ����������� � �������

		; // �������� ��������� long ����� ����� ������� � ����������� ���������� ��� ��������� �������� �� �������
		; // ����������� � edx:eax. ���� ������������ ������� ������ ��� �����, �� ���������, �����
		; // ������� ���� (1) �� ��������.

        cmp     edx, dword ptr [esp+28] ; // ������� ������� ������� ����� �������� � ����������
        ja      short L6        ; // ���� ��������� > ���������, ��������� ���������
        jb      short L7        ; // ���� ��������� < ���������, �� � �������
        cmp     eax, dword ptr [esp+24] ; // ������� ������� ����� �����, �������� ������
        jbe     short L7        ; // ���� ������ ��� �����, �� � �������, �����, ������� ����
L6:
        dec     esi             ; // ������� ���� �� ��������
L7:
        xor     edx, edx         ; // edx:eax <- �������
        mov     eax, esi

		; // �������� ������ �����������. edx:eax �������� �������. ��������� ����
		; // � ������������ � ���������� ���������, ������� ���� � ����� ����������

L4:
        dec     edi             ; // ��������, ����� ������ ���� ��������� �������������
        jnz     short L8        ; // ���� edi == 0, ��������� ������ ���� �������������
        neg     edx             ; // �����, ������� ��� �������������
        neg     eax
        sbb     edx, 0

		; // ���������� ���������� �������� � �����������

L8:
        pop     ebx
        pop     esi
        pop     edi

        ret
	}
}

// ���������� ����� �� llrem.asm
__declspec(naked) void longMod()
{
	__asm
	{
		push    edi
        push    esi
        push    ebx

		; // �������� ����� ������������ ���������. ����� �����, ���� ����� ��������� ��������� �������:
		; //              -----------------
		; //              |               |
		; //              |---------------|
		; //              |               |+28
		; //              |--������� (b)--|
		; //              |               |+24
		; //              |---------------|
		; //              |               |+20
		; //              |--�������� (a)-|
		; //              |               |+16
		; //              |---------------|
		; //              | return addr** |+12
		; //              |---------------|
		; //              |      EDI      |+8
		; //              |---------------|
		; //              |      ESI      |+4
		; //              |---------------|
		; //      ESP---->|      EBX      |
		; //              -----------------

		; // ��������� ���� ���������� (edi = 0 ��� 2 ���� ��������� �������������, ����� �������������)
		; // ������� ��� �������� �����������.

        xor     edi, edi         ; // ����������� ��� �� �������������

        mov     eax, dword ptr [esp+28] ; // ������� ������� ����� ��������
        or      eax, eax		; // ��������, ����� � ���� ����
        jge     L1				; // ��������� �������� ���� �� �����������
        inc     edi				; // �������� ���� ����������
        mov     edx, dword ptr [esp+24] ; // ������� ������� ����� ��������
        neg     eax				; // ������� �������������
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+28], eax ; // �������� ������������� �����
        mov     dword ptr [esp+24], edx
L1:
        mov     eax, dword ptr [esp+20] ; // ������� ������� ����� ��������
        or      eax, eax		; // ��������, ����� � ���� ����
        jge     L2				; // ��������� �������� ���� �� �����������
        inc     edi             ; // �������� ���� ����������
        mov     edx, dword ptr [esp+16] ; // ������� ������� ����� ��������
        neg     eax             ; // ������� �������������
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+20], eax ; // �������� ������������� �����
        mov     dword ptr [esp+16], edx
L2:

		; // ������ �������. ��������� �������, ����� �������� ������ 4194304 �����.
		; // ���� ���, ����� ��������� ������� ���������� � ������� ��������
		; // � ��������� ������, ���� ���������� ������� �������
		; // ������� - eax ������ �������� ������� ������� ����� ��������

        or      eax, eax		; // ���������, ������ �� �� 4194304 �����
        jnz     L3				; // ���, ������� ���� ������ ����
        mov     ecx, dword ptr [esp+16] ; // �������� ��������
        mov     eax, dword ptr [esp+28] ; // �������� ������� dword ��������
        xor     edx, edx
        div     ecx             ; // eax <- ������� dword ��������
        //mov     ebx, eax        ; // �������� ������� dword ��������
        mov     eax, dword ptr [esp+24] ; // edx:eax <- �������:������� dword ��������
        div     ecx             ; // eax <- ������� dword ��������
		mov     eax, edx        ; // edx:eax <- �������
		xor     edx, edx
		dec     edi             ; // �������� ������ ����� ����������
		jns     short L4        ; // �������� ����, ���������� ���� � ����� ����������
		jmp     short L8        ; // ���� � �������, ���������� ���� � ����� ����������

		; // ��� � ��� ������� �������. eax �� ��� �������� ������� ������� ����� ��������

L3:
        mov     ebx, eax         ; // ebx:ecx <- ��������
        mov     ecx, dword ptr [esp+16]
        mov     edx, dword ptr [esp+28] ; // edx:eax <- �������
        mov     eax, dword ptr [esp+24]
L5:
        shr     ebx, 1           ; // ������� �������� ������ �� ���
        rcr     ecx, 1
        shr     edx, 1           ; // ������� ������� ������ �� ���
        rcr     eax, 1
        or      ebx, ebx
        jnz     short L5        ; // ����� ��� ���������, ���� �������� �� ������ ������ 4194304 �����
        div     ecx             ; // ������ ��������, �������������� �������

		; // �� ����� ��������� �� �������, ������� ����� ���������, �� ������� �������
		; // �� �������� � ������� ��������� � �������
		; // ����� ��� ���� ��������� ������������, ������� ����� �������� � ������, �����
		; // ������� ������ � 2**64 � ������� �������� �� 1.

		mov     ecx, eax         ; // �������� ����� �������� � ecx
		mul     dword ptr [esp+20]
		xchg    ecx, eax         ; // �������� ������������, ������� ������� � eax
		mul     dword ptr [esp+16]
		add     edx,ecx         ; // edx:eax = ������� * ��������
		jc      short L6        ; // ������� � ������������ � �������

		; // �������� ��������� long ����� ����� ������� � ����������� ���������� ��� ��������� �������� �� �������
		; // ����������� � edx:eax. ���� ������������ ������� ������ ��� �����, �� ���������, �����
		; // ������� ���� (1) �� ��������.

        cmp     edx, dword ptr [esp+28] ; // ������� ������� ������� ����� �������� � ����������
        ja      short L6        ; // ���� ��������� > ���������, ��������� ���������
        jb      short L7        ; // ���� ��������� < ���������, �� � �������
        cmp     eax, dword ptr [esp+24] ; // ������� ������� ����� �����, �������� ������
        jbe     short L7        ; // ���� ������ ��� �����, �� � �������, �����, ������� ����
L6:
        sub     eax, dword ptr [esp+16] ; // ������� �������� �� ����������
        sbb     edx, dword ptr [esp+20]
L7:
 
		; // ��������� �������, ����� ��������� �� ������������� ��������.
		; // ��� ��� ��������� ��� � �������� �� ��������� ��������� �
		; // ��������������� ����������� � ���� ���� ������� ��������� �������������.
 
        sub     eax, dword ptr [esp+24] ; // ������� ������� �� ����������
        sbb     edx, dword ptr [esp+28]

		; // ������ �������� ���� ����� ����������, ����� ������, ������ �� ��������� ���� �������������
		; // ��� �������������. �� ������ ������������� (������ ��� �� ������� � '������������'
		; // �����������), ��� ��� ���� ���� ����������, �� ���������, �����, ���� �������� ����
		; // ����� ������� ��������� ����� �������������
 
        dec     edi             ; // �������� ���� ����� ����������
        jns     short L8        ; // ��������� � �������, ���������� ���� � ������
L4:
        neg     edx             ; // �����, �������� ���� ����������
        neg     eax
        sbb     edx,0
 
		; // ���������� ���������� �������� � �����������

L8:
        pop     edi
		pop     esi
        pop     ebx
 
        ret
	}
}

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

__declspec(naked) void longShl()
{
	// ���, ����� ��������� � 8 ������ � [esp+4]
	// ����� ��������� � 8 ������ � [esp+4] (������� �������� eip)
	__asm
	{
		cmp dword ptr [esp+8], 0 ;
		jne returnZero ;
		mov ecx, dword ptr [esp+4] ;
		cmp ecx, 64 ;
		jae returnZero ;

		cmp cl, 32 ;
		jae moreThan32 ;
		mov eax, dword ptr [esp+12] ;
		shld dword ptr [esp+16], eax, cl
		shl dword ptr [esp+12], cl
		ret

		moreThan32:
		mov eax, dword ptr [esp+12]
		mov dword ptr [esp+16], eax
		mov dword ptr [esp+12], 0
		and cl, 31
		shl dword ptr [esp+16], cl
		ret

		returnZero:
		mov dword ptr [esp+12], 0 ;
		mov dword ptr [esp+16], 0 ;
		ret ;
	}
}

__declspec(naked) void longShr()
{
	// ���, ����� ��������� � 8 ������ � [esp+4]
	// ����� ��������� � 8 ������ � [esp+4] (������� �������� eip)
	__asm
	{
		cmp dword ptr [esp+8], 0 ;
		jne returnSign ;
		mov ecx, dword ptr [esp+4] ;
		cmp ecx, 64 ;
		jae returnSign ;

		cmp cl, 32 ;
		jae moreThan32 ;
		mov edx, dword ptr [esp+16] ;
		shrd dword ptr [esp+12], edx, cl
		sar dword ptr [esp+16], cl
		ret

		moreThan32:
		mov eax, dword ptr [esp+16]
		sar dword ptr [esp+16], 31
		and cl, 31
		shl eax, cl
		mov dword ptr [esp+12], eax
		ret

		returnSign:
		sar dword ptr [esp+16], 31
		mov dword ptr [esp+12], 0xffffffff
		ret
	}
}