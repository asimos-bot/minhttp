#include "minhttp.h"

#define NULL 0

#define MIN(a, b) (a < b ? a : b)
#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define CHECK_EOF() if(data == data_end) return NULL;

#define EXPECT_NO_CHECK(x) CHECK_EOF(); if(unlikely(*data != x)) return NULL; data++;

#define EXPECT(x) CHECK_EOF(); EXPECT_NO_CHECK(x); if(unlikely(data_end <= data)) return NULL;

#define UNTIL_NOT(x) for(; unlikely(data != data_end && *data == x); data++) CHECK_EOF();

#define EXPECT_NEWLINE() do {\
  if(unlikely(data_end == data)) return NULL;\
  switch(*data) {\
    case '\r': {\
      if(unlikely(data_end - data < 2)) return NULL;\
      if(likely(*(data+1) == '\n')) return data + 2;\
      return NULL;\
    }\
    case '\n': {\
      return data + 1;\
    }\
    default: {\
      return NULL;\
    }\
  }\
} while(0);

static const char *token_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
                                    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
                                    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static inline char* _mh_parse_path(char* data, char* data_end, char** string, uint32_t* string_len) {
  char* limit = *string_len ? MIN(data_end, data + *string_len) : data_end;
  *string = data;
  for(; likely(data < limit && *data != ' ' && *data != '\t'); data++);
  *string_len = data - *string;
  return data;
}

static inline char* _mh_parse_method(char* data, char* data_end, char** method, uint8_t* method_len) {
  char* limit = *method_len ? MIN(data_end, data + *method_len) : data_end;
  *method = data;
  for(; likely(data < limit && *data != ' ' && *data != '\t'); data++) {
    if(unlikely(!token_char_map[*data])) return NULL;
  }
  *method_len = data - *method;
  return data;
}

static inline char* _mh_parse_phrase(char* data, char* data_end, char** phrase, uint32_t* phrase_len) {
  char* limit = *phrase_len ? MIN(data_end, data + *phrase_len) : data_end;
  *phrase = data;
  for(; likely(data < limit && *data != '\r' && *data != '\n'); data++);
  *phrase_len = data - *phrase;
  return data;
}

static inline char* _mh_parse_status_code(char* data, char* data_end, uint16_t* status) {
  // buffer size left
  if(unlikely(data_end - data < 3)) return NULL;
  // are digits valid?
  if(unlikely((data[0] < '1' || '5' < data[0]) || (data[1] < '0' || '9' < data[1]) || (data[2] < '0' || '9' < data[2]))) return NULL;
  *status = 100 * (data[0] - '0') + 10 * (data[1] - '0') + data[2] - '0';
  return data + 3;
}

static inline char* _mh_parse_version(char* data, char* data_end, mh_version* version) {
  if(unlikely(data_end < data + 8)) return NULL;
  EXPECT_NO_CHECK('H');
  EXPECT_NO_CHECK('T');
  EXPECT_NO_CHECK('T');
  EXPECT_NO_CHECK('P');
  EXPECT_NO_CHECK('/');
  EXPECT_NO_CHECK('1');
  EXPECT_NO_CHECK('.');
  switch(*data) {
    case '0': {
      *version = HTTP_1;
      break;
    } 
    case '1': {
      *version = HTTP_1_1;
      break;
    }
    default: {
      return NULL;
    }
  }
  return data + 1;
}

char* mh_parse_request_first_line(char* data, char* data_end, char** method, uint8_t* method_len, char** path, uint32_t* path_len, mh_version* version) {
  if(unlikely(data_end < data)) return NULL;
  data = _mh_parse_method(data, data_end, method, method_len);
  if(unlikely(!data)) return NULL;
  UNTIL_NOT(' ');
  data = _mh_parse_path(data, data_end, path, path_len);
  if(unlikely(!data)) return NULL;
  UNTIL_NOT(' ');
  data = _mh_parse_version(data, data_end, version);
  if(unlikely(!data)) return NULL;
  EXPECT_NEWLINE();
}

char* mh_parse_response_first_line(char* data, char* data_end, mh_version* version, uint16_t* status, char** phrase, uint32_t* phrase_len) {
  if(unlikely(data_end < data)) return NULL;
  data = _mh_parse_version(data, data_end, version);
  if(unlikely(!data)) return NULL;
  UNTIL_NOT(' ');
  data = _mh_parse_status_code(data, data_end, status);
  if(unlikely(!data)) return NULL;
  if(unlikely(data && *data != '\r' && *data != '\n' && *data != ' ')) return NULL;
  UNTIL_NOT(' ');
  data = _mh_parse_phrase(data, data_end, phrase, phrase_len);
  if(unlikely(!data)) return NULL;
  EXPECT_NEWLINE();
  return data;
}

static inline char* _mh_parse_header_key(char* data, char* data_end, char** token_begin, uint16_t* token_len) {
  *token_begin = NULL;
  *token_len = 0;
  if(unlikely(*data == ':')) return NULL;
  for(; likely(data < data_end && *data != ':'); data++) {
    if(unlikely(*data == ' ' || *data == '\t')) return NULL;
    if(unlikely(!*token_begin)) {
        *token_begin = data;
    }
  }
  *token_len = data - *token_begin;
  return data == data_end ? NULL : data;
}

static inline char* _mh_parse_header_value(char* data, char* data_end, char** token_begin, uint16_t* token_len) {
  *token_begin = NULL;
  *token_len = 0;
  for(; likely(data < data_end && *data != '\n'); data++) {
    if(unlikely(!*token_begin)) {
      if(*data != ' ' && *data != '\t') {
        *token_begin = data;
      }
    }
  }
  *token_len = data - *token_begin;
  if(unlikely(data == data_end)) return NULL;
  // deal with carriage return
  if(likely(*token_len && (*token_begin)[(*token_len) - 1] == '\r')) (*token_len)--;
  // deal with whitespace at the end of the line
  while(likely((*token_len) && ((*token_begin)[(*token_len) - 1] == ' ' || (*token_begin)[(*token_len) - 1] == '\t'))) {
    (*token_len)--;
  }
  return data;
}

static inline char* _mh_parse_headers(char* data, char* data_end, mh_header* headers, uint32_t* num_headers) {
  uint32_t header_counter = 0;
  for(; header_counter < *num_headers; header_counter++) {
    CHECK_EOF();
    if(unlikely((*data == '\r' && *(data + 1) == '\n') || *data == '\n')) {
      data += 1 + (*data != '\n');
      goto done;
    }
    CHECK_EOF();
    if(unlikely((data = _mh_parse_header_key(data, data_end, &headers[header_counter].header_key_begin, &headers[header_counter].header_key_len)) == NULL)) return NULL;
    data++;
    if(unlikely((data = _mh_parse_header_value(data, data_end, &headers[header_counter].header_value_begin, &headers[header_counter].header_value_len)) == NULL)) return NULL;
    data++;
  }
done:
  *num_headers = header_counter;
  return data;
}

char* mh_parse_headers(char* data, char* data_end, mh_header* headers, uint32_t* num_headers) {
  data = _mh_parse_headers(data, data_end, headers, num_headers);
  if(unlikely(!data)) *num_headers = 0;
  return data;
}
