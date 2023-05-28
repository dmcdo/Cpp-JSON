# Cpp-JSON
A simple JSON implentation in C++.

## Usage
The `JSON::Object` and `JSON::Array` classes are instantiated by calling the constructor with an appropriate JSON string.
```cpp
JSON::Object obj = JSON::Object("{\"a\": 1, \"b\": 2}");
JSON::Array arr = JSON::Array("[\"Hello\", \"World\"]");
```

## Data Retrieval
In order to reconcile JSON's type flexibility with C++'s strict static typing, several different "get" methods are provided by the `JSON::Object` and `JSON::Array` classes depending on the type of data you are expecting at a given key.

```cpp
double a = obj.get_number("a"); // Object values are accessed by their key
string s = arr.get_string(0);   // Array values are accessed by their index
```

To check the type of a value stored at a given key, you can use either an "is" method or the `get_type()` method.
```cpp
assert obj.is_number("b");
assert arr.get_type(1) == JSON::STRING;
```

## Iteration
Object keys are not ordered, so the order they are iterated over is arbitrary. Arrays indexes will always be iterated over in order.
```cpp
for (string key : obj)
{
    cout << key << ": " << obj.get_number(key) << endl;
}

for (size_t index : arr)
{
    cout << arr.get_string(index) << ' ';
}

// Arrays can also be iterated over imperatively
for (size_t i = 0; i < arr.size(); i++)
{
    cout << arr.get_string(index) << ' ';
}
```
```
a: 1
b: 2
Hello World Hello World
```
## Convert an Object or Array back to JSON a string
The Object and Array classes provide a `to_string()` method, which will return a JSON string equivalent to the one it was initialized with. `to_string()` also optionally takes an unsigned integer as an argument, which will indent the JSON string.
```cpp
cout << "Unindented: " << endl << obj.to_string() << endl;
cout << "Indented: " << endl << obj.to_string(4) << endl;
```
```
Unindented:
{"a": 1.000000, "b", 2.000000}
Indented:
{
    "a": 1.000000,
    "b": 2.000000
}
```

## Exceptions
All of the below exceptions are child classes of the abstract `JSON::JSONException` class:

|Exception|Description|
|:--------|:----------|
|`JSON::DecodeException`|Thrown when an invalid string is provided to an Object or Array constructor.|
|`JSON::InvalidControlCharacterException`|Thrown when an ASCII value less than 0x20 is specified inside of a key or string value.|
|`JSON::InvalidKeyException`|Thrown when a "get" method is called on an Object and the key provided does not exist in the Object.|
|`JSON::InvalidIndexException`|Thrown when a  "get" method is called on an Array and the index provided is out of bounds.|
|`JSON::WrongTypeException`|Thrown when  the wrong "get" method is used on a value, for example, in the case of attempting to access a string value with the `get_number()` method.|
|`JSON::UnknownInternalException`|Please submit a bug report if you encounter this exception.|
