#ifndef SHARED_PARSER_FUNCTIONS
#define SHARED_PARSER_FUNCTIONS

#include <Arduino.h>

// #include <stdbool.h>
// #include <string>

typedef struct Buf_t {
  std::string tag;
  std::string end_tag;
  std::string value;
  std::string attribute;
  std::string attribute_value;
  bool parsed;
} Buf_t;

enum class ParseResult {
  success,
  crc_failed,
  unknown_tag,
  file_error,
  unexpexted_return
};

enum class ParsingState {
    pre_start_element,
    start_element,
    attribute,
    attribute_value,
    text_element,
    end_element
};

const wchar_t open_bracket      = 0x3c;
const wchar_t close_bracket     = 0x3e;
const wchar_t xml_comment       = 0x21;
const wchar_t xml_slash         = 0x2f;
const wchar_t xml_end_comment   = 0x2d;
const wchar_t sign_space        = 0x20;
const wchar_t sign_equal        = 0x3d;
const wchar_t sign_double_quote = 0x22;


int replaceXmlSpecialCharactersIsWideString(char * inputString, uint32_t inputStringLength);
Buf_t parseXmlLikeString(const char * inputWideString);
bool extractWString(char ** bufferStart, char * bufferEnd, std::string & string);
bool extractLineAndParseXml(char ** bufferStart, char * bufferEnd, Buf_t & xmlBuf);
	
#endif