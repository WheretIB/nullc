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
		fstp st ; //уберём st0
		
		fldcw word ptr [esp] ; //востановим флаг контроля
		pop eax ;
		ret
	}
}

// Реализация взята из llmul.asm
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
		or ecx, eax ; // проверяем, не равны ли верхние биты нулю
		mov ecx, dword ptr [esp+12] ; // в обоих ветвях ecx держит bLow
		jnz fullMult ; // если это не так, будем делать полностью
		// иначе имеется короткий вариант
		mov eax, dword ptr [esp+4] ; // aLow
		mul ecx ; // перемножим нижние dword'ы
		ret ; // и выйдем

		fullMult:
		mul ecx ; // aHigh*bLow
		mov ebx, eax ; // сохраним

		mov eax, dword ptr [esp+4] ; // aLow
		mul dword ptr [esp+16] ; // bHigh
		add ebx, eax ; // ebx = aHigh*bLow + aLow*bHigh

		mov eax, dword ptr [esp+4] ; // aLow
		mul ecx ; // aLow*bLow причём в eax и лишнее в edx
		add edx, ebx ; // прибавим ещё aHigh*bLow и aLow*bHigh

		ret ; // выйдем
	}
}

// Реализация взята из lldiv.asm
// Это настолько сложно, что оставлю, как есть
__declspec(naked) void longDiv()
{
	__asm
	{
		push    edi
        push    esi
        push    ebx

		; // Сохраним часть используемых регистров. После этого, стек будет выглядеть следующим образом:
		; //              -----------------
		; //              |               |
		; //              |---------------|
		; //              |               |+28
		; //              |--делимое (b)--|
		; //              |               |+24
		; //              |---------------|
		; //              |               |+20
		; //              |--делитель (a)-|
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

		; // Определим знак результата (edi = 0 или 2 если результат положительный, иначе отрицательный)
		; // Сделаем оба операнда позитивными.

        xor     edi, edi         ; // предположим что он положительный

        mov     eax, dword ptr [esp+28] ; // Старшее двойное слово делимого
        or      eax, eax		; // Проверим, какой у него знак
        jge     L1				; // Пропустим действия если он положителен
        inc     edi				; // Поменяем знак результата
        mov     edx, dword ptr [esp+24] ; // Младшее двойное слово делимого
        neg     eax				; // Сделаем положительным
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+28], eax ; // Сохраним положительное число
        mov     dword ptr [esp+24], edx
L1:
        mov     eax, dword ptr [esp+20] ; // Старшее двойное слово делителя
        or      eax, eax		; // Проверим, какой у него знак
        jge     L2				; // Пропустим действия если он положителен
        inc     edi             ; // Поменяем знак результата
        mov     edx, dword ptr [esp+16] ; // Младшее двойное слово делителя
        neg     eax             ; // Сделаем положительным
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+20], eax ; // Сохраним положительное число
        mov     dword ptr [esp+16], edx
L2:

		; // Теперь деление. Посмотрим сначала, может делитель меньше 4194304 тысяч.
		; // Если так, можно расчитать простым алгоритмом с обычным делением
		; // В противном случае, вещи становятся немного сложнее
		; // Заметка - eax сейчас содержит старшее двойное слово делителя

        or      eax, eax		; // Посмотрим, меньше ли он 4194304 тысяч
        jnz     L3				; // Неа, придётся идти тяжёлым путём
        mov     ecx, dword ptr [esp+16] ; // Загрузим делитель
        mov     eax, dword ptr [esp+28] ; // Загрузим старший dword делимого
        xor     edx, edx
        div     ecx             ; // eax <- старший dword частного
        mov     ebx, eax        ; // сохраним старший dword частного
        mov     eax, dword ptr [esp+24] ; // edx:eax <- остаток:младший dword делимого
        div     ecx             ; // eax <- младший dword частного
        mov     edx, ebx        ; // edx:eax <- частное
        jmp     short L4        ; // Установить знак, востановить стек и вернутся

		; // Тут у нас сложный вариант. eax всё ещё содержит старшее двойное слово делителя

L3:
        mov     ebx, eax         ; // ebx:ecx <- делитель
        mov     ecx, dword ptr [esp+16]
        mov     edx, dword ptr [esp+28] ; // edx:eax <- делимое
        mov     eax, dword ptr [esp+24]
L5:
        shr     ebx, 1           ; // Сдвинем делитель вправо на бит
        rcr     ecx, 1
        shr     edx, 1           ; // Сдвинем делимое вправо на бит
        rcr     eax, 1
        or      ebx, ebx
        jnz     short L5        ; // Будем это повторять, пока делитель не станет меньше 4194304 тысяч
        div     ecx             ; // Теперь разделим, проигнорировав остаток
        mov     esi, eax         ; // Сохраним частное

		; // Мы могли ошибиться на единицу, поэтому чтобы проверить, мы умножим частное
		; // на делитель и сравним результат с делимым
		; // Также нам надо проверить переполнение, которое может случится в случае, когда
		; // делимое близко к 2**64 и частное ошибочно на 1.

        mul     dword ptr [esp+20] ; // Частное * Старшее двойное слово делителя
        mov     ecx, eax
        mov     eax, dword ptr [esp+16]
        mul     esi             ; // Частное * Младшее двойное слово делителя
        add     edx, ecx         ; // edx:eax = Частное * Делитель
        jc      short L6        ; // Частное имеет погрешность в единицу

		; // Провести сравнение long чисел между делимым и результатом полученным при умножении делителя на частное
		; // находящееся в edx:eax. Если оригинальное делимое больше или равно, всё правильно, иначе
		; // отнимем один (1) от частного.

        cmp     edx, dword ptr [esp+28] ; // Сравним старшие двойные слова делимого и результата
        ja      short L6        ; // Если результат > оригинала, произведём вычитание
        jb      short L7        ; // если результат < оригинала, всё в порядке
        cmp     eax, dword ptr [esp+24] ; // Старшие двойные слова равны, проверим нижние
        jbe     short L7        ; // Если меньше или равно, всё в порядке, иначе, отнимем один
L6:
        dec     esi             ; // Отимаае один от частного
L7:
        xor     edx, edx         ; // edx:eax <- частное
        mov     eax, esi

		; // Осталось только подтереться. edx:eax содержит частное. Установим знак
		; // в соответствии с сохранённым значением, очистим стек и вернём управление

L4:
        dec     edi             ; // Проверим, чтобы узнать если рехультат отрицательный
        jnz     short L8        ; // если edi == 0, результат должен быть положительным
        neg     edx             ; // иначе, сделаем его отрицательным
        neg     eax
        sbb     edx, 0

		; // Востановим сохранённые регистры и возвратимся

L8:
        pop     ebx
        pop     esi
        pop     edi

        ret
	}
}

// Реализация взята из llrem.asm
__declspec(naked) void longMod()
{
	__asm
	{
		push    edi
        push    esi
        push    ebx

		; // Сохраним часть используемых регистров. После этого, стек будет выглядеть следующим образом:
		; //              -----------------
		; //              |               |
		; //              |---------------|
		; //              |               |+28
		; //              |--делимое (b)--|
		; //              |               |+24
		; //              |---------------|
		; //              |               |+20
		; //              |--делитель (a)-|
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

		; // Определим знак результата (edi = 0 или 2 если результат положительный, иначе отрицательный)
		; // Сделаем оба операнда позитивными.

        xor     edi, edi         ; // предположим что он положительный

        mov     eax, dword ptr [esp+28] ; // Старшее двойное слово делимого
        or      eax, eax		; // Проверим, какой у него знак
        jge     L1				; // Пропустим действия если он положителен
        inc     edi				; // Поменяем знак результата
        mov     edx, dword ptr [esp+24] ; // Младшее двойное слово делимого
        neg     eax				; // Сделаем положительным
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+28], eax ; // Сохраним положительное число
        mov     dword ptr [esp+24], edx
L1:
        mov     eax, dword ptr [esp+20] ; // Старшее двойное слово делителя
        or      eax, eax		; // Проверим, какой у него знак
        jge     L2				; // Пропустим действия если он положителен
        inc     edi             ; // Поменяем знак результата
        mov     edx, dword ptr [esp+16] ; // Младшее двойное слово делителя
        neg     eax             ; // Сделаем положительным
        neg     edx
        sbb     eax, 0
        mov     dword ptr [esp+20], eax ; // Сохраним положительное число
        mov     dword ptr [esp+16], edx
L2:

		; // Теперь деление. Посмотрим сначала, может делитель меньше 4194304 тысяч.
		; // Если так, можно расчитать простым алгоритмом с обычным делением
		; // В противном случае, вещи становятся немного сложнее
		; // Заметка - eax сейчас содержит старшее двойное слово делителя

        or      eax, eax		; // Посмотрим, меньше ли он 4194304 тысяч
        jnz     L3				; // Неа, придётся идти тяжёлым путём
        mov     ecx, dword ptr [esp+16] ; // Загрузим делитель
        mov     eax, dword ptr [esp+28] ; // Загрузим старший dword делимого
        xor     edx, edx
        div     ecx             ; // eax <- старший dword частного
        //mov     ebx, eax        ; // сохраним старший dword частного
        mov     eax, dword ptr [esp+24] ; // edx:eax <- остаток:младший dword делимого
        div     ecx             ; // eax <- младший dword частного
		mov     eax, edx        ; // edx:eax <- остаток
		xor     edx, edx
		dec     edi             ; // Проверим флажок знака результата
		jns     short L4        ; // поменяем знак, востановим стек и вернём управление
		jmp     short L8        ; // знак в порядке, востановим стек и вернём управление

		; // Тут у нас сложный вариант. eax всё ещё содержит старшее двойное слово делителя

L3:
        mov     ebx, eax         ; // ebx:ecx <- делитель
        mov     ecx, dword ptr [esp+16]
        mov     edx, dword ptr [esp+28] ; // edx:eax <- делимое
        mov     eax, dword ptr [esp+24]
L5:
        shr     ebx, 1           ; // Сдвинем делитель вправо на бит
        rcr     ecx, 1
        shr     edx, 1           ; // Сдвинем делимое вправо на бит
        rcr     eax, 1
        or      ebx, ebx
        jnz     short L5        ; // Будем это повторять, пока делитель не станет меньше 4194304 тысяч
        div     ecx             ; // Теперь разделим, проигнорировав остаток

		; // Мы могли ошибиться на единицу, поэтому чтобы проверить, мы умножим частное
		; // на делитель и сравним результат с делимым
		; // Также нам надо проверить переполнение, которое может случится в случае, когда
		; // делимое близко к 2**64 и частное ошибочно на 1.

		mov     ecx, eax         ; // сохраним копию частного в ecx
		mul     dword ptr [esp+20]
		xchg    ecx, eax         ; // Сохраним произведение, получим частное в eax
		mul     dword ptr [esp+16]
		add     edx,ecx         ; // edx:eax = Частное * Делитель
		jc      short L6        ; // Частное с погрешностью в единицу

		; // Провести сравнение long чисел между делимым и результатом полученным при умножении делителя на частное
		; // находящееся в edx:eax. Если оригинальное делимое больше или равно, всё правильно, иначе
		; // отнимем один (1) от частного.

        cmp     edx, dword ptr [esp+28] ; // Сравним старшие двойные слова делимого и результата
        ja      short L6        ; // Если результат > оригинала, произведём вычитание
        jb      short L7        ; // если результат < оригинала, всё в порядке
        cmp     eax, dword ptr [esp+24] ; // Старшие двойные слова равны, проверим нижние
        jbe     short L7        ; // Если меньше или равно, всё в порядке, иначе, отнимем один
L6:
        sub     eax, dword ptr [esp+16] ; // Отнимем делитель из результата
        sbb     edx, dword ptr [esp+20]
L7:
 
		; // Расчитаем остаток, отняв результат из оригинального делимого.
		; // Так как результат уже в регистре мы произведём вычитание в
		; // противоположном направлении и если надо сделаем результат отрицательным.
 
        sub     eax, dword ptr [esp+24] ; // Отнимем делимое из результата
        sbb     edx, dword ptr [esp+28]

		; // Теперь проверим флаг знака результата, чтобы узнать, должен ли результат быть положительным
		; // или отрицательным. Он сейчас отрицательный (потому что мы вычлели в 'неправильном'
		; // направлении), так что если флаг установлен, мы закончили, иначе, надо поменять знак
		; // чтобы сделать результат вновь положительным
 
        dec     edi             ; // проверим флаг знака результата
        jns     short L8        ; // результат в порядке, востановим стек и выйдем
L4:
        neg     edx             ; // иначе, поменяем знак результата
        neg     eax
        sbb     edx,0
 
		; // Востановим сохранённые регистры и возвратимся

L8:
        pop     edi
		pop     esi
        pop     ebx
 
        ret
	}
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

__declspec(naked) void longShl()
{
	// Так, число находится в 8 байтах с [esp+4]
	// Сдвиг находится в 8 байтах с [esp+4] (вершину занимает eip)
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
	// Так, число находится в 8 байтах с [esp+4]
	// Сдвиг находится в 8 байтах с [esp+4] (вершину занимает eip)
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