#ifndef STR_H
#define STR_H
bool String_Compare(const char* str1, const char* str2);
bool String_CaselessCompare(const char* str1, const char* str2);
size_t String_Length(const char* str);
size_t String_Copy(char* dst, size_t len, const char* src);
char* String_CopyUnsafe(char* dst, const char* src);
uint String_FormatError(uint code, char* buf, uint buflen);
void String_FormatBufVararg(char* buf, size_t len, const char* str, va_list* args);
void String_FormatBuf(char* buf, size_t len, const char* str, ...);
const char* String_AllocCopy(const char* str);
size_t String_GetArgument(const char* args, char* arg, int index);
int String_ToInt(const char* str);
#endif