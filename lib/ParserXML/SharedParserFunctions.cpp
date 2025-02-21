#include "SharedParserFunctions.hpp"

// uint16_t var = 0;
int replaceXmlSpecialCharactersIsWideString(char *inputString, uint32_t inputStringLength)
{
    // use iSL
    int ret_val; // return length of the new xml string in case all characters are valid
    bool valid_xml_character_flag = true;
    size_t length = inputStringLength; // just for testing purposes
    char newStr[length];

    memset(newStr, 0, length);

    size_t newStrPtr = 0;
    for (size_t i = 0; i < length; ++i)
    {
        if (inputString[i] == L'&')
        {

            // Special character is found
            if ((i + 2) >= length)
            {
#ifdef DEBUG_PARSER
                printf("End of string on & char\n");
#endif // DEBUG_PARSER
                break;
            }

            if ((inputString[i + 1] == 'l') && (inputString[i + 2] == 't') && (inputString[i + 3] == ';'))
            {
                newStr[newStrPtr] = '<';
                i += 3;
            }
            else if ((inputString[i + 1] == 'g') && (inputString[i + 2] == 't') && (inputString[i + 3] == ';'))
            {
                newStr[newStrPtr] = '>';
                i += 3;
            }
            else if (((i + 3) < length) && (inputString[i + 1] == 'a') && (inputString[i + 2] == 'm') && (inputString[i + 3] == 'p') && (inputString[i + 4] == ';'))
            {
                newStr[newStrPtr] = '&';
                i += 4;
            }
            else if (((i + 4) < length) && (inputString[i + 1] == 'a') && (inputString[i + 2] == 'p') && (inputString[i + 3] == 'o') && (inputString[i + 4] == 's') && (inputString[i + 5] == ';'))
            {
                newStr[newStrPtr] = '\'';
                i += 5;
            }
            else if (((i + 4) < length) && (inputString[i + 1] == 'q') && (inputString[i + 2] == 'u') && (inputString[i + 3] == 'o') && (inputString[i + 4] == 't') && (inputString[i + 5] == ';'))
            {
                newStr[newStrPtr] = '\"';
                i += 5;
            }
            else
                valid_xml_character_flag = false; // Wrong special character
        }
        else
        {
            newStr[newStrPtr] = inputString[i];
        }

        newStrPtr++;
    }

    newStr[newStrPtr] = '\0';

    strcpy(inputString, newStr);

    if (valid_xml_character_flag)
        ret_val = newStrPtr + 1;
    else
        ret_val = 0;

    return ret_val;
}

Buf_t parseXmlLikeString(const char *inputWideString)
{
    static char str[3000];

    Buf_t buf;
    char smallbuf[2] = {0};
    size_t input_length = strlen(inputWideString); //-2 to cut \r\n
    int length = 0;
    ParsingState state = ParsingState::pre_start_element;

    memset(str, 0, 3000);

    for (uint16_t i = 0; i < input_length; ++i)
        str[i] = inputWideString[i];

    length = replaceXmlSpecialCharactersIsWideString(str, input_length);

    if (!length)
    {
        return buf;
    }

    buf.parsed = false;

    for (size_t i = 0; i < length; ++i)
    {
        switch (state)
        {
        case ParsingState::pre_start_element:
            if (str[i] != open_bracket)
            {
                continue;
            }

            if (str[i + 1] == xml_slash)
            {
                state = ParsingState::end_element; // i++?
                i++;
            }
            else
                state = ParsingState::start_element;

            break;
        case ParsingState::start_element:
            if (str[i] == sign_space)
            {
                state = ParsingState::attribute;
            }
            else if (str[i] == close_bracket)
            {
                state = ParsingState::text_element;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.tag.append(smallbuf);
            }
            break;
        case ParsingState::attribute:
            if (str[i] == sign_equal)
            {
                ++i;
                // If string is broken - return what we've collected, parsed field won't be true at this point
                if (i >= length)
                    return buf;

                if (str[i] == sign_double_quote)
                    state = ParsingState::attribute_value;
                else
                    return buf;
                // We can handle spaces if there is any requirement
            }
            else if (str[i] == close_bracket)
            {
                state = ParsingState::text_element;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.attribute.append(smallbuf);
            }
            break;
        case ParsingState::attribute_value:
            if (str[i] == sign_double_quote)
            {
                ++i;

                if (i >= length)
                    return buf;

                if (str[i] == close_bracket)
                    state = ParsingState::text_element;
                else
                    return buf;

                // We can handle spaces if there is any requirement
            }
            else
            {
                smallbuf[0] = str[i];
                buf.attribute_value.append(smallbuf);
            }
            break;
        case ParsingState::text_element:
            if (str[i] == open_bracket)
            {
                ++i;

                if (i >= length)
                    return buf;

                if (str[i] == xml_slash)
                {
                    state = ParsingState::end_element;
                    continue;
                }

                --i;
            }

            smallbuf[0] = str[i];
            buf.value.append(smallbuf);

            break;
        case ParsingState::end_element:
            if ((str[i] == close_bracket) || (str[i] == 0x000D) || (str[i] == 0x000A))
            {
                break;
            }
            else
            {
                smallbuf[0] = str[i];
                buf.end_tag.append(smallbuf);
            }
            break;
        }
    }

    buf.parsed = true;
    /*
    wcscpy(tag, buf.tag.c_str());
    wcscpy(end_tag, buf.end_tag.c_str());
    wcscpy(value, buf.value.c_str());
    wcscpy(attr, buf.attribute.c_str());
    wcscpy(attr_val, buf.attribute_value.c_str());
*/
    return buf;
}

bool extractWString(char **bufferStart, char *bufferEnd, std::string &string)
{
    while (*bufferStart != bufferEnd)
    {
        auto symbol = **bufferStart;

        *bufferStart += 1;

        string.push_back(symbol);

        if (symbol == static_cast<char>(0xa))
            return true;
    }

    return false;
}

bool extractLineAndParseXml(char **bufferStart, char *bufferEnd, Buf_t &xmlBuf)
{
    std::string string;
    auto result = extractWString(bufferStart, bufferEnd, string);
    if (!result)
    {
        return false;
    }

    xmlBuf = parseXmlLikeString(string.c_str());

    return true;
}
