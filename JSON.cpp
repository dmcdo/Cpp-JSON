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
    case JSON::ARRAY:
        return "Array";
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
    case JSON::ARRAY:
        _array = new JSON::Array(*other._array);
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
    case JSON::ARRAY:
        if (_array != NULL)
        {
            delete _array;
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
    int line = 1;
    std::vector<JSON::_Node *> nodes;

    if (s == NULL)
    {
        throw DecodeException(0);
    }

    // Parse JSON String
    try
    {
        s = _cstring_next_non_whitespace(s, &line);
        if (*s != '{')
        {
            throw JSON::DecodeException(line);
        }

        s++;
        while (true)
        {
            s = _cstring_next_non_whitespace(s, &line);
            if (*s == '}')
            {
                s++;
                break;
            }
            if (*s != '"')
            {
                throw JSON::DecodeException(line);
            }

            JSON::_Node *node = new JSON::_Node();
            s = _json_consume_string(s, &node->_key, line);
            s = _cstring_next_non_whitespace(s, &line);
            if (*s != ':')
            {
                throw JSON::DecodeException(line);
            }

            s = _cstring_next_non_whitespace(s + 1, &line);
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
                node->_type = JSON::OBJECT;
                node->_object = new Object(s, &s);
                break;
            case '[':
                node->_type = JSON::ARRAY;
                node->_array = new Array(s, &s);
                break;

            default:
                throw JSON::DecodeException(line);
            }

            node->_next = NULL;
            nodes.push_back(node);

            s = _cstring_next_non_whitespace(s, &line);
            if (*s == ',')
            {
                s++;
            }
        }
    }
    catch (DecodeException &ex)
    {
        for (auto &node : nodes)
        {
            delete node;
        }

        throw ex;
    }

    // Reorganize vector of values into hash map
    _size = nodes.size();
    _map = new JSON::_Node *[_size]();

    for (auto &node : nodes)
    {
        size_t index = _cstring_hash(node->_key->c_str()) % _size;

        if (_map[index] == NULL)
        {
            _map[index] = node;
        }
        else
        {
            JSON::_Node *parent;
            for (parent = _map[index]; parent->_next != NULL; parent = parent->_next)
                ;

            parent->_next = node;
        }
    }

    return s;
}

JSON::Object::Object(const JSON::Object &other)
{
    _size = other._size;
    _map = new JSON::_Node *[_size];

    for (size_t i = 0; i < _size; i++)
    {
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
        JSON::_Node *cur = _map[i];
        while (cur != NULL)
        {
            JSON::_Node *next = cur->_next;
            delete cur;
            cur = next;
        }
    }

    delete[] _map;
}

size_t JSON::Object::size()
{
    return _size;
}

JSON::_Node *JSON::Object::_get(const char *key, bool check_type, JSON::Type expected_type)
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
            if (cur->_type == expected_type || !check_type)
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
    return _get(key, true, JSON::OBJECT)->_object;
}

JSON::Array *JSON::Object::get_array(const char *key)
{
    return _get(key, true, JSON::ARRAY)->_array;
}

std::string &JSON::Object::get_string(const char *key)
{
    return *_get(key, true, JSON::STRING)->_string;
}

const char *JSON::Object::get_cstring(const char *key)
{
    return _get(key, true, JSON::STRING)->_string->c_str();
}

double JSON::Object::get_number(const char *key)
{
    return _get(key, true, JSON::NUMBER)->_number;
}

bool JSON::Object::get_boolean(const char *key)
{
    return _get(key, true, JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Object::get_type(const char *key)
{
    return _get(key, false, JSON::JSON_NULL)->_type;
}

JSON::Object *JSON::Object::get_object(std::string &key)
{
    return _get(key.c_str(), true, JSON::OBJECT)->_object;
}

JSON::Array *JSON::Object::get_array(std::string &key)
{
    return _get(key.c_str(), true, JSON::ARRAY)->_array;
}

std::string &JSON::Object::get_string(std::string &key)
{
    return *_get(key.c_str(), true, JSON::STRING)->_string;
}

const char *JSON::Object::get_cstring(std::string &key)
{
    return _get(key.c_str(), true, JSON::STRING)->_string->c_str();
}

double JSON::Object::get_number(std::string &key)
{
    return _get(key.c_str(), true, JSON::NUMBER)->_number;
}

bool JSON::Object::get_boolean(std::string &key)
{
    return _get(key.c_str(), true, JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Object::get_type(std::string &key)
{
    return _get(key.c_str(), false, JSON::JSON_NULL)->_type;
}

bool JSON::Object::is_object(const char *key)
{
    return get_type(key) == JSON::OBJECT;
}

bool JSON::Object::is_array(const char *key)
{
    return get_type(key) == JSON::ARRAY;
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

bool JSON::Object::is_array(std::string &key)
{
    return get_type(key) == JSON::ARRAY;
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

    std::string repr = tabzero + '{';

    for (auto iter = begin();;)
    {
        repr += linesep;
        repr += tabone;
        repr += _dump_string(*iter);
        repr += ": ";
        switch (get_type(*iter))
        {
        case JSON::JSON_NULL:
            repr += "null";
            break;
        case JSON::OBJECT:
            repr += &get_object(*iter)->to_string(indent, depth + 1)[indent * (depth + 1)];
            break;
        case JSON::ARRAY:
            repr += &get_array(*iter)->to_string(indent, depth + 1)[indent * (depth + 1)];
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

    repr += linesep + tabzero + '}';
    return repr;
}

JSON::Object::Iterator JSON::Object::begin()
{
    if (_size == 0)
    {
        return end();
    }

    size_t index = 0;
    for (; _map[index]->_key == NULL; index++)
        ;

    return JSON::Object::Iterator(_size, index, _map, _map[index]);
}

JSON::Object::Iterator JSON::Object::end()
{
    return JSON::Object::Iterator(_size, _size, _map, NULL);
}

JSON::Object::Iterator::Iterator(size_t size, size_t index, JSON::_Node **map, JSON::_Node *node)
{
    _size = size;
    _index = index;
    _map = map;
    _node = node;
}

JSON::Object::Iterator &JSON::Object::Iterator::operator++()
{
    if (_node->_next != NULL)
    {
        _node = _node->_next;
        return *this;
    }

    while (++_index < _size)
    {
        if (_map[_index] != NULL)
        {
            _node = _map[_index];
            return *this;
        }
    }

    _node = NULL;
    return *this;
}

JSON::Object::Iterator JSON::Object::Iterator::operator++(int)
{
    auto prev = JSON::Object::Iterator(_size, _index, _map, _node);
    ++(*this);
    return prev;
}

bool JSON::Object::Iterator::operator==(const JSON::Object::Iterator &other)
{
    return _map == other._map &&
           _node == other._node &&
           _index == other._index;
}

bool JSON::Object::Iterator::operator!=(const JSON::Object::Iterator &other)
{
    return !(*this == other);
}

std::string &JSON::Object::Iterator::operator*()
{
    return *_node->_key;
}

/*
 * Array
 */
JSON::Array::Array(const char *s)
{
    _initialize(s);
}

JSON::Array::Array(std::string &s)
{
    _initialize(s.c_str());
}

JSON::Array::Array(const char *s, const char **r)
{
    *r = _initialize(s);
}

const char *JSON::Array::_initialize(const char *s)
{
    int line = 1;
    std::vector<JSON::_Node *> nodes;

    if (s == NULL)
    {
        throw DecodeException(0);
    }

    s = _cstring_next_non_whitespace(s, &line);
    if (*s != '[')
    {
        throw JSON::DecodeException(line);
    }

    s = _cstring_next_non_whitespace(s + 1, &line);
    if (*s == ']')
    {
        _list = new JSON::_Node *[0];
        _size = 0;
        return s + 1;
    }

    while (true)
    {
        JSON::_Node *node = new JSON::_Node();
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
            node->_type = JSON::OBJECT;
            node->_object = new Object(s, &s);
            break;
        case '[':
            node->_type = JSON::ARRAY;
            node->_array = new Array(s, &s);
            break;

        default:
            throw JSON::DecodeException(line);
        }

        node->_next = NULL;
        nodes.push_back(node);

        s = _cstring_next_non_whitespace(s, &line);
        if (*s == ']')
        {
            s++;
            break;
        }
        if (*s != ',')
        {
            throw JSON::DecodeException(line);
        }

        s = _cstring_next_non_whitespace(s + 1, &line);
    }

    // Transfer vector to array
    _size = nodes.size();
    _list = new JSON::_Node *[_size];
    for (size_t i = 0; i < _size; i++)
    {
        _list[i] = nodes[i];
    }

    return s;
}

JSON::Array::Array(const JSON::Array &other)
{
    _size = other._size;
    _list = new JSON::_Node *[_size];

    for (size_t i = 0; i < _size; i++)
    {
        _list[i] = new JSON::_Node(*other._list[i]);
    }
}

JSON::Array::~Array()
{
    for (size_t i = 0; i < _size; i++)
    {
        delete _list[i];
    }

    delete[] _list;
}

size_t JSON::Array::size()
{
    return _size;
}

JSON::_Node *JSON::Array::_get(size_t i, bool check_type, JSON::Type expected_type)
{
    if (i >= _size)
    {
        throw InvalidIndexException(i);
    }

    if (_list[i]->_type == expected_type || !check_type)
    {
        return _list[i];
    }
    else
    {
        throw WrongTypeException(std::to_string(i).c_str(), expected_type, _list[i]->_type);
    }
}

JSON::Object *JSON::Array::get_object(size_t i)
{
    return _get(i, true, JSON::OBJECT)->_object;
}

JSON::Array *JSON::Array::get_array(size_t i)
{
    return _get(i, true, JSON::ARRAY)->_array;
}

const char *JSON::Array::get_cstring(size_t i)
{
    return _get(i, true, JSON::STRING)->_string->c_str();
}

std::string &JSON::Array::get_string(size_t i)
{
    return *_get(i, true, JSON::STRING)->_string;
}

double JSON::Array::get_number(size_t i)
{
    return _get(i, true, JSON::NUMBER)->_number;
}

bool JSON::Array::get_boolean(size_t i)
{
    return _get(i, true, JSON::BOOLEAN)->_boolean;
}

JSON::Type JSON::Array::get_type(size_t i)
{
    return _get(i, false, JSON::JSON_NULL)->_type;
}

bool JSON::Array::is_object(size_t i)
{
    return get_type(i) == JSON::OBJECT;
}

bool JSON::Array::is_array(size_t i)
{
    return get_type(i) == JSON::ARRAY;
}

bool JSON::Array::is_string(size_t i)
{
    return get_type(i) == JSON::STRING;
}

bool JSON::Array::is_number(size_t i)
{
    return get_type(i) == JSON::NUMBER;
}

bool JSON::Array::is_boolean(size_t i)
{
    return get_type(i) == JSON::BOOLEAN;
}

bool JSON::Array::is_null(size_t i)
{
    return get_type(i) == JSON::JSON_NULL;
}

std::string JSON::Array::to_string()
{
    return to_string(0, 0);
}

std::string JSON::Array::to_string(unsigned int indent)
{
    return to_string(indent, 0);
}

JSON::Array::Iterator JSON::Array::begin()
{
    return JSON::Array::Iterator(_list, _list);
}

JSON::Array::Iterator JSON::Array::end()
{
    return JSON::Array::Iterator(_list, _list + _size);
}

std::string JSON::Array::to_string(unsigned int indent, unsigned int depth)
{
    std::string tabzero(indent * depth, ' ');
    std::string tabone(indent * (depth + 1), ' ');
    std::string linesep = indent == 0 ? "" : "\r\n";

    // Construct string
    if (_size == 0)
    {
        return tabzero + "[]";
    }

    std::string repr = tabzero + '[';
    for (auto iter = begin();;)
    {
        repr += linesep;
        repr += tabone;

        switch (get_type(*iter))
        {
        case JSON::JSON_NULL:
            repr += "null";
            break;
        case JSON::OBJECT:
            repr += &get_object(*iter)->to_string(indent, depth + 1)[indent * (depth + 1)];
            break;
        case JSON::ARRAY:
            repr += &get_array(*iter)->to_string(indent, depth + 1)[indent * (depth + 1)];
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

    repr += linesep + tabzero + ']';
    return repr;
}

JSON::Array::Iterator::Iterator(JSON::_Node **begin, JSON::_Node **node)
{
    _begin = begin;
    _node = node;
}

JSON::Array::Iterator &JSON::Array::Iterator::operator++()
{
    ++_node;
    return *this;
}

JSON::Array::Iterator JSON::Array::Iterator::operator++(int)
{
    auto prev = JSON::Array::Iterator(_begin, _node);
    ++(*this);
    return prev;
}

bool JSON::Array::Iterator::operator==(const JSON::Array::Iterator &other)
{
    return _node == other._node;
}

bool JSON::Array::Iterator::operator!=(const JSON::Array::Iterator &other)
{
    return _node != other._node;
}

size_t JSON::Array::Iterator::operator*()
{
    return _node - _begin;
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

JSON::InvalidIndexException::InvalidIndexException(size_t index)
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