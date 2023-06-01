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
            JSON::_Node **_p;

        public:
            Iterator(JSON::_Node **);
            Iterator &operator++();
            Iterator operator++(int);
            bool operator==(const Iterator &other);
            bool operator!=(const Iterator &other);
            std::string &operator*();
        };

        Object(const char *s);
        Object(std::string &s);
        Object(const char *, const char **);
        Object(const Object &);
        ~Object();

        JSON::Object::Iterator begin();
        JSON::Object::Iterator end();

        size_t size();
        bool is_array();

        JSON::Object *get_object(const char *key);
        JSON::Object *get_object(std::string &key);
        JSON::Object *get_object(int index);
        std::string &get_string(const char *key);
        std::string &get_string(std::string &key);
        std::string &get_string(int index);
        const char *get_cstring(const char *key);
        const char *get_cstring(std::string &key);
        const char *get_cstring(int index);
        double get_number(const char *key);
        double get_number(std::string &key);
        double get_number(int index);
        bool get_boolean(const char *key);
        bool get_boolean(std::string &key);
        bool get_boolean(int index);

        JSON::Type get_type(const char *key);
        JSON::Type get_type(std::string &key);
        JSON::Type get_type(int index);
        bool is_object(const char *key);
        bool is_object(std::string &key);
        bool is_object(int index);
        bool is_string(const char *key);
        bool is_string(std::string &key);
        bool is_string(int index);
        bool is_number(const char *key);
        bool is_number(std::string &key);
        bool is_number(int index);
        bool is_boolean(const char *key);
        bool is_boolean(std::string &key);
        bool is_boolean(int index);
        bool is_null(const char *key);
        bool is_null(std::string &key);
        bool is_numm(int index);

        std::string to_string();
        std::string to_string(unsigned int indent);
        std::string to_string(unsigned int indent, unsigned int depth);

    private:
        bool _is_array;
        size_t _size;
        JSON::_Node **_map;
        JSON::_Node **_ord;
        JSON::_Node *_get(const char *, Type);
        JSON::_Node *_get_by_index(int, Type);
        const char *_initialize(const char *);
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
        InvalidIndexException(int);
    };

    class WrongTypeException : public JSONException
    {
    private:
        void _initialize(const char *, Type, Type);
    public:
        WrongTypeException(const char *, Type, Type);
        WrongTypeException(std::string &, Type, Type);
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