import stringutil;

void AddErrorLocationInfo(char[] codeStart, StringRef errorPos, StringRef errorBuf)
{
	/*if(!errorBuf || !errorBufSize)
		return;

	char *errorCurr = errorBuf + strlen(errorBuf);

	if(!codeStart || !errorPos)
	{
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "\n");
		return;
	}

	const char *codeEnd = codeStart + strlen(codeStart);

	if(errorPos < codeStart || errorPos > codeEnd)
	{
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "\n");
		return;
	}

	const char *start = errorPos;
	const char *end = start == codeEnd ? start : start + 1;

	while(start > codeStart && *(start - 1) != '\r' && *(start - 1) != '\n')
		start--;

	while(*end && *end != '\r' && *end != '\n')
		end++;

	if(errorBufSize > unsigned(errorCurr - errorBuf))
		*(errorCurr++) = '\n';

	char *errorCurrBefore = errorCurr;

	if(unsigned line = GetErrorLocationLineNumber(codeStart, errorPos))
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at line %d: '", line);
	else
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at '");

	unsigned spacing = unsigned(errorCurr - errorCurrBefore);

	for(const char *pos = start; *pos && pos < end; pos++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = *pos == '\t' ? ' ' : *pos;
	}

	errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "'\n");

	for(unsigned i = 0; i < spacing + unsigned(errorPos - start); i++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = ' ';
	}

	errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "^\n");

	errorBuf[errorBufSize - 1] = '\0';*/
}
