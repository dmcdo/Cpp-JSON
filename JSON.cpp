#include "JSON.hpp"
#include <string.h>
#include <stdio.h>
#include <vector>
#include <string>

/*
 * String Methods
 */
size_t _cstring_hash(const char *s)
{
    if (s == NULL)
        return 0;

    size_t i = 0;
    size_t x = 1000003;

    for (const char *p = s; *p; p++, i++)
    {
        x = (1000003 * x) ^ *p;
    }

    return x ^ i;
}

const char *_cstring_next_non_whitespace(const char *s, int *line)
{
    for (; isspace(*s); s++)
    {
        if (*s == '\n')
        {
            *line += 1;
        }
    }

    return s;
}

const char *_json_type_to_cstring(JSON::Type type)
{
    switch (type)
    {
    case JSON::JSON_NULL:
        return "Null";
    case JSON::OBJECT:
        return "Object";
    case JSON::STRING:
        return "String";
    case JSON::NUMBER:
        return "Number";
    case JSON::BOOLEAN:
        return "Boolean";
    default:
        return NULL;
    }
}

std::string _dump_string(std::string &src)
{
    std::vector<std::string> codepoints;

    for (auto iter = src.rbegin(); iter < src.rend(); ++iter)
    {
        if (*iter == 0x7F)
        {
            codepoints.push_back("\\u007F");
        }
        else if ((unsigned char)*iter >= (unsigned char)' ')
        {
            char buf[] = {*iter, '\0'};
            codepoints.push_back(buf);
        }
        else
        {
            switch (*iter)
            {
            case '"':
                codepoints.push_back("\\\"");
                break;
            case '\\':
                codepoints.push_back("\\\\");
                break;
            case '/':
                codepoints.push_back("\\/");
                break;
            case '\b':
                codepoints.push_back("\\b");
                break;
            case '\f':
                codepoints.push_back("\\f");
                break;
            case '\n':
                codepoints.push_back("\\n");
                break;
            case '\r':
                codepoints.push_back("\\r");
                break;
            case '\t':
                codepoints.push_back("\\t");
                break;
            default:
                throw JSON::UnknownInternalException();
            }
        }
    }

    std::string repr = "\"";

    for (auto iter = codepoints.rbegin(); iter < codepoints.rend(); ++iter)
    {
        repr += *iter;
    }

    repr += "\"";
    return repr;
}

/*
 * Parsing of primitive types
 */
const char *__json_consume_phrase(const char *s, const char *phrase, int line)
{
    for (const char *p = phrase; *p; p++, s++)
    {
        if (*p != *s)
        {
            throw JSON::DecodeException(line);
        }
    }

    return s;
}
#define _JSON_CONSUME_NULL(s, line) (__json_consume_phrase(s, "null", line))
#define _JSON_CONSUME_TRUE(s, line) (__json_consume_phrase(s, "true", line))
#define _JSON_CONSUME_FALSE(s, line) (__json_consume_phrase(s, "false", line))

const char *_json_consume_number(const char *s, double *x, int line)
{
    std::string buffer;
    if (*s == '-')
    {
        buffer += *(s++);
    }


    if ((s[0] == '-' && !(s[1] >= '0' && s[1] <= '9')))
    {
        throw JSON::DecodeException(line);
    }

    sscanf(s, "%lf", x);

    bool dot = false;
    bool eee = false;
    for (s = *s == '-' ? s + 1 : s; true; s++)
    {
        if (*s >= '0' && *s <= '9')
        {
            continue;
        }
        if (!eee && *s == 'e')
        {
            eee = true;
            dot = true;

            if (s[1] == '-')
            {
                s++;
            }

            continue;
        }
        if (!dot && *s == '.')
        {
            dot = true;
            continue;
        }
        break;
    }

    return s;
}

const char *_json_consume_string(const char *s, std::string **r, int line)
{
    std::string *p = *r = new std::string();

    for (s++;; s++)
    {
        if (*s >= 0 && *s < ' ')
        {
            throw JSON::InvalidControlCharacterException();
        }

        if (*s == '\\')
        {
            switch (*++s)
            {
            case '"':
            case '\\':
            case '/':
                *p += *(s++);
                break;
            case 'b':
                *p += '\b';
                s++;
                break;
            case 'f':
                *p += '\f';
                s++;
                break;
            case 'n':
                *p += '\n';
                s++;
                break;
            case 'r':
                *p += '\r';
                s++;
                break;
            case 't':
                *p += '\t';
                s++;
                break;
            case 'u':
            {
                char u[5] = {0};

                s++;
                for (int i = 0; i < 4; i++, s++)
                {
                    if ((*s >= '0' && *s <= '9') ||
                        (*s >= 'A' && *s <= 'F') ||
                        (*s >= 'a' && *s <= 'f'))
                    {
                        u[i] = *s;
                    }
                    else
                    {
                        delete p;
                        throw JSON::DecodeException(line);
                    }
                }

                unsigned int t;
                sscanf(u, "%x", &t);

                if (t < 0x20)
                {
                    throw JSON::InvalidControlCharacterException();
                }
                if (t <= 0xFF)
                {
                    *p += (char)(t);
                }
                else
                {
                    *p += (char)(t >> 8);
                    *p += (char)(t & 0xFF);
                }
            }
            break;
            default:
                delete p;
                throw JSON::DecodeException(line);
            }
        }

        switch (*s)
        {
        case '\0':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
            delete p;
            throw JSON::DecodeException(line);
        case '"':
            goto end_of_string;
        default:
            *p += *s;
            break;
        }
    }

end_of_string:
    if (*s != '"')
    {
        delete p;
        throw JSON::DecodeException(line);
    }

    return s + 1;
}

/*
 * _Node
 */
JSON::_Node::_Node()
{
    _key = NULL;
    _type = JSON::JSON_NULL;
    _next = NULL;
}

JSON::_Node::_Node(const _Node &other)
{
    _type = other._type;
    
    if (other._key == NULL)
    {
        _key = NULL;
    }
    else
    {
        _key = new std::string(*other._key);
    }

    switch (other._type)
    {
    case JSON::OBJECT:
        _object = new JSON::Object(*other._object);
        break;
    case JSON::STRING:
        _string = new std::string(*other._string);
        break;
    case JSON::NUMBER:
        _number = other._number;
        break;
    case JSON::BOOLEAN:
        _boolean = other._boolean;
        break;
    default:
        break;
    }
}

JSON::_Node::~_Node()
{
    if (_key != NULL)
    {
        delete _key;
    }

    switch (_type)
    {
    case JSON::OBJECT:
        if (_object != NULL)
        {
            delete _object;
        }
        break;
    case JSON::STRING:
        if (_string != NULL)
        {
            delete _string;
        }
        break;
    default:
        break;
    }
}

/*
 * Object
 */
JSON::Object::Object(const char *s)
{
    _initialize(s);
}

JSON::Object::Object(std::string &s)
{
    _initialize(s.c_str());
}

JSON::Object::Object(const char *s, const char **r)
{
    *r = _initialize(s);
}

const char *JSON::Object::_initialize(const char *s)
{
    if (s == NULL)
    {
        throw JSON::DecodeException(0);
    }

    int line = 1;
    std::vector<JSON::_Node *> nodes;

    try
    {
        s = _cstring_next_non_whitespace(s, &line);
        if (*s == '[')
        {
            _is_array = true;
        }
        else if (*s == '{')
        {
            _is_array = false;
        }
        else
        {
            throw JSON::DecodeException(line);
        }

        s = _cstring_next_non_whitespace(s + 1, &line);
        if (*s != (_is_array ? ']' : '}'))
        {
            while (true)
            {
                s = _cstring_next_non_whitespace(s, &line);

                // Anticipate next node
                JSON::_Node *node = new JSON::_Node();
                nodes.push_back(node);

                // Parse key
                if (_is_array)
                {
                    node->_key = new std::string(std::to_string(nodes.size() - 1));
                }
                else
                {
                    if (*s != '"')
                    {
                        throw JSON::DecodeException(line);
                    }

                    s = _json_consume_string(s, &node->_key, line);
                    s = _cstring_next_non_whitespace(s, &line);
                    if (*s != ':')
                    {
                        throw JSON::DecodeException(line);
                    }
                    else
                    {
                        s = _cstring_next_non_whitespace(s + 1, &line);
                    }
                }

                // Parse value
                switch (*s)
                {
                case 't':
                    s = _JSON_CONSUME_TRUE(s, line);
                    node->_type = JSON::BOOLEAN;
                    node->_boolean = true;
                    break;
                case 'f':
                    s = _JSON_CONSUME_FALSE(s, line);
                    node->_type = JSON::BOOLEAN;
                    node->_boolean = false;
                    break;
                case 'n':
                    s = _JSON_CONSUME_NULL(s, line);
                    node->_type = JSON::JSON_NULL;
                    break;
                case '"':
                    s = _json_consume_string(s, &node->_string, line);
                    node->_type = JSON::STRING;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                    s = _json_consume_number(s, &node->_number, line);
                    node->_type = JSON::NUMBER;
                    break;
                case '{':
                case '[':
                    node->_type = JSON::OBJECT;
                    node->_object = new Object(s, &s);
                    break;
                default:
                    throw JSON::DecodeException(line);
                }

                s = _cstring_next_non_whitespace(s, &line);
                if (*s == (_is_array ? ']' : '}'))
                {
                    s++;
                    break;
                }
                else if (*s == ',')
                {
                    s++;
                }
                else
                {
                    throw JSON::DecodeException(line);
                }
            }
        }
    }
    catch (JSON::DecodeException &ex)
    {
        for (auto &node : nodes)
        {
            delete node;
        }

        throw ex;
    }


    _size = nodes.size();
    _map = new JSON::_Node *[_size]();
    _ord = new JSON::_Node *[_size]();

    for (size_t i = 0; i < nodes.size(); i++)
    {
        _ord[i] = nodes[i];

        size_t index = _cstring_hash(nodes[i]->_key->c_str()) % _size;
        if (_map[index] == NULL)
        {
            _map[index] = nodes[i];
        }
        else
        {
            JSON::_Node *parent;
            for (parent = _map[index]; parent->_next != NULL; parent = parent->_next)
                ;

            parent->_next = nodes[i];
        }
    }

    return s;
}

JSON::Object::Object(const JSON::Object &other)
{
    _size = other._size;
    _map = new JSON::_Node *[_size];
    _ord = new JSON::_Node *[_size];

    for (size_t i = 0; i < _size; i++)
    {
        _ord[i] = new JSON::_Node(*other._ord[i]);

        if (other._map[i] == NULL)
        {
            _map[i] = NULL;
        }
        else
        {
            _map[i] = new JSON::_Node(*other._map[i]);

            JSON::_Node *parent = _map[i];
            JSON::_Node *src = other._map[i]->_next;
            for (; src != NULL; src++)
            {
                parent->_next = new JSON::_Node(*src);
            }
        }
    }
}

JSON::Object::~Object()
{
    for (size_t i = 0; i < _size; i++)
    {
        delete _ord[i];
    }

    delete[] _ord;
    delete[] _map;
}

size_t JSON::Object::size()
{
    return _size;
}

bool JSON::Object::is_array()
{
    return _is_array;
}

JSON::_Node *JSON::Object::_get(const char *key, JSON::Type expected_type)
{
    if (_size == 0 || key == NULL)
    {
        throw InvalidKeyException(key);
    }

    size_t index = _cstring_hash(key) % _size;

    for (JSON::_Node *cur = _map[index]; cur; cur = cur->_next)
    {
        if (strcmp(key, cur->_key->c_str()) == 0)
        {
            if (cur->_type == expected_type || expected_type == JSON::JSON_NULL)
            {
                return cur;
            }
            else
            {
                throw WrongTypeException(key, expected_type, cur->_type);
            }
        }
    }

    throw InvalidKeyException(key);
}

JSON::Object *JSON::Object::get_object(const char *key)
{
    return _get(key, JSON::OBJECT)->_object;
}

std::string &JSON::Object::get_string(const char *key)
{
    return *_get(key, JSON::STRING)->_string;
}

const char *JSON::Object::get_cstring(const char *key)
{
    return _get(key, JSON::STRING)->_string->c_str();
}

double JSON::Object::get_number(const char *key)
{
    return _get(key, JSON::NUMBER)->_number;
}

bool JSON::Object::get_boolean(const char *key)
{
    return _get(key, JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Object::get_type(const char *key)
{
    return _get(key, JSON::JSON_NULL)->_type;
}

JSON::Object *JSON::Object::get_object(std::string &key)
{
    return _get(key.c_str(), JSON::OBJECT)->_object;
}

std::string &JSON::Object::get_string(std::string &key)
{
    return *_get(key.c_str(), JSON::STRING)->_string;
}

const char *JSON::Object::get_cstring(std::string &key)
{
    return _get(key.c_str(), JSON::STRING)->_string->c_str();
}

double JSON::Object::get_number(std::string &key)
{
    return _get(key.c_str(), JSON::NUMBER)->_number;
}

bool JSON::Object::get_boolean(std::string &key)
{
    return _get(key.c_str(), JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Object::get_type(std::string &key)
{
    return _get(key.c_str(), JSON::JSON_NULL)->_type;
}

JSON::_Node *JSON::Object::_get_by_index(int index, JSON::Type expected_type)
{
    if (!_is_array)
    {
        return _get(std::to_string(index).c_str(), JSON::OBJECT);
    }

    if (index < 0 || (size_t)index >= _size)
    {
        throw InvalidIndexException(index);
    }

    if (expected_type && _ord[index]->_type != expected_type)
    {
        std::string s = std::to_string(index);
        throw WrongTypeException(s, expected_type, _ord[index]->_type);
    }

    return _ord[index];
}

JSON::Object *JSON::Object::get_object(int index)
{
    return _get_by_index(index, JSON::OBJECT)->_object;
}

std::string &JSON::Object::get_string(int index)
{
    return *_get_by_index(index, JSON::STRING)->_string;
}

const char *JSON::Object::get_cstring(int index)
{
    return _get_by_index(index, JSON::STRING)->_string->c_str();
}

double JSON::Object::get_number(int index)
{
    return _get_by_index(index, JSON::NUMBER)->_number;
}

bool JSON::Object::get_boolean(int index)
{
    return _get_by_index(index, JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Object::get_type(int index)
{
    return _get_by_index(index, JSON::JSON_NULL)->_type;
}

bool JSON::Object::is_object(const char *key)
{
    return get_type(key) == JSON::OBJECT;
}

bool JSON::Object::is_string(const char *key)
{
    return get_type(key) == JSON::STRING;
}

bool JSON::Object::is_number(const char *key)
{
    return get_type(key) == JSON::NUMBER;
}

bool JSON::Object::is_boolean(const char *key)
{
    return get_type(key) == JSON::BOOLEAN;
}

bool JSON::Object::is_null(const char *key)
{
    return get_type(key) == JSON::JSON_NULL;
}

bool JSON::Object::is_object(std::string &key)
{
    return get_type(key) == JSON::OBJECT;
}

bool JSON::Object::is_string(std::string &key)
{
    return get_type(key) == JSON::STRING;
}

bool JSON::Object::is_number(std::string &key)
{
    return get_type(key) == JSON::NUMBER;
}

bool JSON::Object::is_boolean(std::string &key)
{
    return get_type(key) == JSON::BOOLEAN;
}

bool JSON::Object::is_null(std::string &key)
{
    return get_type(key) == JSON::JSON_NULL;
}

std::string JSON::Object::to_string()
{
    return to_string(0, 0);
}

std::string JSON::Object::to_string(unsigned int indent)
{
    return to_string(indent, 0);
}

std::string JSON::Object::to_string(unsigned int indent, unsigned int depth)
{
    std::string tabzero(indent * depth, ' ');
    std::string tabone(indent * (depth + 1), ' ');
    std::string linesep = indent == 0 ? "" : "\r\n";

    // Construct string
    if (_size == 0)
    {
        return tabzero + "{}";
    }

    std::string repr = tabzero + (_is_array ? '[' : '{');

    for (auto iter = begin();;)
    {
        repr += linesep;
        repr += tabone;

        if (!_is_array)
        {
            repr += _dump_string(*iter);
            repr += ": ";
        }

        switch (get_type(*iter))
        {
        case JSON::JSON_NULL:
            repr += "null";
            break;
        case JSON::OBJECT:
            repr += &get_object(*iter)->to_string(indent, depth + 1)[indent * (depth + 1)];
            break;
        case JSON::STRING:
        {
            std::string s = get_string(*iter);
            repr += _dump_string(s);
            break;
        }
        case JSON::NUMBER:
            repr += std::to_string(get_number(*iter));
            break;
        case JSON::BOOLEAN:
            repr += get_boolean(*iter) ? "true" : "false";
            break;
        default:
            throw JSON::UnknownInternalException();
            break;
        }

        if (++iter != end())
        {
            repr += ", ";
        }
        else
        {
            break;
        }
    }

    repr += linesep + tabzero + (_is_array ? ']' : '}');
    return repr;
}

JSON::Object::Iterator JSON::Object::begin()
{
    if (_size == 0)
    {
        return end();
    }
    else
    {
        return JSON::Object::Iterator(_ord);
    }
}

JSON::Object::Iterator JSON::Object::end()
{
    return JSON::Object::Iterator(_ord + _size);
}

JSON::Object::Iterator::Iterator(JSON::_Node **p)
{
    _p = p;
}

JSON::Object::Iterator &JSON::Object::Iterator::operator++()
{
    ++_p;
    return *this;
}

JSON::Object::Iterator JSON::Object::Iterator::operator++(int)
{
    JSON::Object::Iterator prev = JSON::Object::Iterator(_p);
    ++(*this);
    return prev;
}

bool JSON::Object::Iterator::operator==(const JSON::Object::Iterator &other)
{
    return _p == other._p;
}

bool JSON::Object::Iterator::operator!=(const JSON::Object::Iterator &other)
{
    return !(*this == other);
}

std::string &JSON::Object::Iterator::operator*()
{
    return *(*_p)->_key;
}

/*
 * Define Exceptions
 */
JSON::JSONException::~JSONException()
{
    if (_message != NULL)
    {
        delete[] _message;
    }
}

const char *JSON::JSONException::what() const throw()
{
    return _message == NULL ? _static_message : _message;
}

JSON::InvalidKeyException::InvalidKeyException(const char *key)
{
    std::string message;
    message += "Attempted to access an element with key ";
    message += key ? "\"" : "";
    message += key ? key : "(NULL)";
    message += key ? "\"" : "";
    message += " but no such key exists.";

    _message = new char[message.length() + 1];
    strcpy(_message, message.c_str());
}

JSON::InvalidIndexException::InvalidIndexException(int index)
{
    std::string message;
    message += "Attempted to access an element at index ";
    message += std::to_string(index);
    message += " but the index is out of bounds.";

    _message = new char[message.length() + 1];
    if (_message == NULL)
    {
        throw std::bad_alloc();
    }

    strcpy(_message, message.c_str());
}

JSON::WrongTypeException::WrongTypeException(const char *key, JSON::Type expected, JSON::Type actual)
{
    _initialize(key, expected, actual);
}

JSON::WrongTypeException::WrongTypeException(std::string &key, JSON::Type expected, JSON::Type actual)
{
    _initialize(key.c_str(), expected, actual);
}

void JSON::WrongTypeException::_initialize(const char *key, JSON::Type expected, JSON::Type actual)
{
    std::string message;
    message += "Attempted to access an element with key \"";
    message += key;
    message += "\" as a ";
    message += _json_type_to_cstring(expected);
    message += " but the element is of type ";
    message += _json_type_to_cstring(actual);
    message += ".";

    _message = new char[message.length() + 1];
    if (_message == NULL)
    {
        throw std::bad_alloc();
    }

    strcpy(_message, message.c_str());
}

JSON::DecodeException::DecodeException(int line)
{
    std::string message;
    message += "Encountered unexpected or malformed token on line ";
    message += std::to_string(line);
    message += ".";

    _message = new char[message.length() + 1];
    if (_message == NULL)
    {
        throw std::bad_alloc();
    }

    strcpy(_message, message.c_str());
}

JSON::InvalidControlCharacterException::InvalidControlCharacterException()
{
    _static_message = "Encountered invalid control character in String.";
}

JSON::UnknownInternalException::UnknownInternalException()
{
    _static_message = "Encountered an unexpected internal state.";
}