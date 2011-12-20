#ifndef __BASE64_H__
#define __BASE64_H__

extern char *base64_encode(const char *str);
extern char *base64_decode(const char *data);
extern int base64_encode_common(char *buf, const char *charset, const char *str);
extern int base64_decode_common(char *buf, const char *str);

#endif
