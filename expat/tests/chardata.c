/*  chardata.c
 *
 *
 */

#include <check.h>
#include <stdio.h>
#include <string.h>

#include "chardata.h"


static int
xmlstrlen(const XML_Char *s)
{
    int len = 0;
    while (s[len] != 0)
        ++len;
    return len;
}


void
CharData_Init(CharData *storage)
{
    storage->count = -1;
}

void
CharData_AppendString(CharData *storage, const char *s)
{
    int maxchars = sizeof(storage->data) / sizeof(storage->data[0]);
    int len = strlen(s);

    if (storage->count < 0)
        storage->count = 0;
    if ((len + storage->count) > maxchars) {
        len = (maxchars - storage->count);
    }
    if (len + storage->count < sizeof(storage->data)) {
        memcpy(storage->data + storage->count, s, len);
        storage->count += len;
    }
}

void
CharData_AppendXMLChars(CharData *storage, const XML_Char *s, int len)
{
    int maxchars = sizeof(storage->data) / sizeof(storage->data[0]);

    if (storage->count < 0)
        storage->count = 0;
    if (len < 0)
        len = xmlstrlen(s);
    if ((len + storage->count) > maxchars) {
        len = (maxchars - storage->count);
    }
    if (len + storage->count < sizeof(storage->data)) {
        memcpy(storage->data + storage->count, s,
               len * sizeof(storage->data[0]));
        storage->count += len;
    }
}

bool
CharData_CheckString(CharData *storage, const char *expected)
{
    char buffer[1024];
    int len = strlen(expected);
    int count = (storage->count < 0) ? 0 : storage->count;

    if (len != count) {
        sprintf(buffer, "wrong number of data characters: got %d, expected %d",
                count, len);
        fail(buffer);
        return false;
    }
    if (memcmp(expected, storage->data, len) != 0) {
        fail("got bad data bytes");
        return false;
    }
    return true;
}

bool
CharData_CheckXMLChars(CharData *storage, const XML_Char *expected)
{
    char buffer[1024];
    int len = strlen(expected);
    int count = (storage->count < 0) ? 0 : storage->count;

    if (len != count) {
        sprintf(buffer, "wrong number of data characters: got %d, expected %d",
                count, len);
        fail(buffer);
        return false;
    }
    if (memcmp(expected, storage->data, len * sizeof(storage->data[0])) != 0) {
        fail("got bad data bytes");
        return false;
    }
    return true;
}
