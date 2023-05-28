#ifndef CPP_JSON_HPP
#define CPP_JSON_HPP

#include <exception>
#include <string>

namespace JSON
{
    struct _Node;
    class Array;
    class Object;

    enum Type
    {
        JSON_NULL = 0,
        OBJECT,
        ARRAY,
        STRING,
        NUMBER,
        BOOLEAN
    };

    struct _Node
    {
    public:
        JSON::Type _type;
        union
        {
            JSON::Object *_object;
            JSON::Array *_array;
            std::string *_string;
            double _number;
            bool _boolean;
        };

        std::string *_key;
        _Node *_next;

        _Node();
        _Node(const _Node &);
        ~_Node();
    };

    class Object
    {
    public:
        class Iterator
        {
        private:
            size_t _size;
            size_t _index;
            JSON::_Node **_map;
            JSON::_Node *_node;

        public:
            Iterator(size_t, size_t, JSON::_Node **, JSON::_Node *);
            Iterator &operator++();
            Iterator operator++(int);
            bool operator==(const Iterator &other);
            bool operator!=(const Iterator &other);
            std::string &operator*();
        };

    private:
        size_t _size;
        JSON::_Node **_map;
        JSON::_Node *_get(const char *, bool, Type);
        const char *_initialize(const char *);

    public:
        Object(const char *s);
        Object(std::string &s);
        Object(const char *, const char **);
        Object(const Object &);
        ~Object();

        JSON::Object::Iterator begin();
        JSON::Object::Iterator end();

        size_t size();

        JSON::Object *get_object(const char *key);
        JSON::Object *get_object(std::string &key);
        JSON::Array *get_array(const char *key);
        JSON::Array *get_array(std::string &key);
        std::string &get_string(const char *key);
        std::string &get_string(std::string &key);
        const char *get_cstring(const char *key);
        const char *get_cstring(std::string &key);
        double get_number(const char *key);
        double get_number(std::string &key);
        bool get_boolean(const char *key);
        bool get_boolean(std::string &key);

        JSON::Type get_type(const char *key);
        JSON::Type get_type(std::string &key);
        bool is_object(const char *key);
        bool is_object(std::string &key);
        bool is_array(const char *key);
        bool is_array(std::string &key);
        bool is_string(const char *key);
        bool is_string(std::string &key);
        bool is_number(const char *key);
        bool is_number(std::string &key);
        bool is_boolean(const char *key);
        bool is_boolean(std::string &key);
        bool is_null(const char *key);
        bool is_null(std::string &key);

        std::string to_string();
        std::string to_string(unsigned int indent);
        std::string to_string(unsigned int indent, unsigned int depth);
    };

    class Array
    {
    public:
        class Iterator
        {
        private:
            JSON::_Node **_begin;
            JSON::_Node **_node;

        public:
            Iterator(JSON::_Node **, JSON::_Node **);
            Iterator &operator++();
            Iterator operator++(int);
            bool operator==(const Iterator &other);
            bool operator!=(const Iterator &other);
            size_t operator*();
        };

    private:
        size_t _size;
        JSON::_Node **_list;
        JSON::_Node *_get(size_t, bool, Type);
        const char *_initialize(const char *);

    public:
        Array(const char *s);
        Array(std::string &s);
        Array(const char *, const char **);
        Array(const Array &);
        ~Array();

        JSON::Array::Iterator begin();
        JSON::Array::Iterator end();

        size_t size();

        JSON::Object *get_object(size_t index);
        JSON::Array *get_array(size_t index);
        std::string &get_string(size_t index);
        const char *get_cstring(size_t index);
        double get_number(size_t index);
        bool get_boolean(size_t index);

        JSON::Type get_type(size_t index);
        bool is_object(size_t index);
        bool is_array(size_t index);
        bool is_string(size_t index);
        bool is_number(size_t index);
        bool is_boolean(size_t index);
        bool is_null(size_t index);

        std::string to_string();
        std::string to_string(unsigned int indent);
        std::string to_string(unsigned int indent, unsigned int depth);
    };

    class JSONException : public std::exception
    {
    protected:
        char *_message = NULL;
        const char *_static_message = NULL;

    public:
        virtual const char *what() const throw();
        ~JSONException();
    };

    class InvalidKeyException : public JSONException
    {
    public:
        InvalidKeyException(const char *);
    };

    class InvalidIndexException : public JSONException
    {
    public:
        InvalidIndexException(size_t);
    };

    class WrongTypeException : public JSONException
    {
    public:
        WrongTypeException(const char *, Type, Type);
    };

    class DecodeException : public JSONException
    {
    public:
        DecodeException(int);
    };

    class InvalidControlCharacterException : public JSONException
    {
    public:
        InvalidControlCharacterException();
    };

    class UnknownInternalException : public JSONException
    {
    public:
        UnknownInternalException();
    };
}

#endif