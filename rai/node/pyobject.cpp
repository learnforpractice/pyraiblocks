/*
 * pyobject.cpp
 *
 *  Created on: Sep 8, 2017
 *      Author: newworld
 */
#include "rai/node/pyobject.hpp"

static struct python_api api = {};

PyObject* (* py_new_none)();
PyObject* (* py_new_bool)(int b);
PyObject* (* py_new_string)(std::string s);
PyObject* (* py_new_int)(int n);
PyObject* (* py_new_int64)(long long n);
PyObject* (* py_new_uint64)(unsigned long long n);
PyObject* (* py_new_float)(double n);
PyObject* (* py_new_exception)(const char* error);

PyObject* (* array_create)();
int (* array_size)(PyObject* arr);

void (* array_append)(PyObject* arr, PyObject* v);
void (* array_append_string)(PyObject* arr, std::string& s);
void (* array_append_int)(PyObject* arr, int n);
void (* array_append_double)(PyObject* arr, double n);
void (* array_append_uint64)(PyObject* arr, unsigned long long n);

PyObject* (* dict_create)();
void (* dict_add)(PyObject* d, PyObject* key, PyObject* value);
bool (* dict_get_value)(PyObject* d, const char* key, std::string& value);


void register_python_api (struct python_api* _api)
{
	api = *_api;
	py_new_none = api.py_new_none;
	py_new_bool = api.py_new_bool;
	py_new_string = api.py_new_string;
	py_new_int = api.py_new_int;
	py_new_int64 = api.py_new_int64;
	py_new_uint64 = api.py_new_uint64;
	py_new_float = api.py_new_float;
	py_new_exception = api.py_new_exception;

	array_create = api.array_create;
	array_size = api.array_size;

	array_append = api.array_append;
	array_append_string = api.array_append_string;
	array_append_int = api.array_append_int;
	array_append_double = api.array_append_double;
	array_append_uint64 = api.array_append_uint64;

	dict_create = api.dict_create;
	dict_add = api.dict_add;
	dict_get_value = api.dict_get_value;
}


PyArray::PyArray() {
   arr = array_create();
}

PyArray::~PyArray() {
   Py_XDECREF(arr);
}

void PyArray::append(PyObject* obj) { array_append(arr, obj); }

void PyArray::append(std::string s) { array_append_string(arr, s); }

void PyArray::append(int n) { array_append_int(arr, n); }

void PyArray::append(unsigned int n) { array_append_int(arr, n); }

void PyArray::append(uint64_t n) { array_append_uint64(arr, n); }

void PyArray::append(double n) { array_append_double(arr, n); }

PyObject* PyArray::get() {
   Py_XINCREF(arr);
   return arr;
}

int PyArray::size()
{
   return array_size(arr);
}


PyDict::PyDict() {
   pydict = dict_create();
}

PyDict::PyDict(PyObject* dictObj) {
   pydict = dictObj;
   Py_XINCREF(pydict);
}

void PyDict::add(PyObject* key, PyObject* value) {
   if (key == NULL || value == NULL) {
      return;
   }
   dict_add(pydict, key, value);
}

void PyDict::add(std::string key, PyObject* value) {
   if (key.size() == 0 || value == NULL) {
      return;
   }
   PyObject* pykey = py_new_string(key);
   dict_add(pydict, pykey, value);
}

#if 0
void PyDict::add(std::string& key, PyObject* value) {
   if (key.size() == 0 || value == NULL) {
      return;
   }
   PyObject* pykey = py_new_string(key);
   dict_add(pydict, pykey, value);
}
#endif

void PyDict::add(const char* key, std::string value)
{
   std::string _key(key);
   PyObject* pykey = py_new_string(_key);
   PyObject* pyvalue = py_new_string(value);
   dict_add(pydict, pykey, pyvalue);
}

void PyDict::add(std::string key, std::string value) {
   PyObject* pykey = py_new_string(key);
   PyObject* pyvalue = py_new_string(value);
   dict_add(pydict, pykey, pyvalue);
}

#if 0
void PyDict::add(std::string& key, std::string& value) {
   PyObject* pykey = py_new_string(key);
   PyObject* pyvalue = py_new_string(value);
   dict_add(pydict, pykey, pyvalue);
}
#endif

void PyDict::add(std::string& key, long long n) {
   PyObject* pykey = py_new_string(key);
   PyObject* pyvalue = py_new_int64(n);
   dict_add(pydict, pykey, pyvalue);
}

int PyDict::size()
{
   return array_size(pydict);
}

PyObject* PyDict::get() {
   Py_XINCREF(pydict);
   return pydict;
}

bool PyDict::get_value(const char* key, string& value) {
   return dict_get_value(pydict, key, value);
}

PyDict::~PyDict() {
   Py_XDECREF(pydict);
}


