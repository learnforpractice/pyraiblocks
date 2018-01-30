/*
 * pyobject.cpp
 *
 *  Created on: Sep 8, 2017
 *      Author: newworld
 */
#include "pyobject.hpp"

static struct python_api api = {};

extern "C" struct python_api* get_python_api ()
{
	api.py_new_none = py_new_none;
	api.py_new_bool = py_new_bool;
	api.py_new_string = py_new_string;

	api.py_new_int = py_new_int;
	api.py_new_int64 = py_new_int64;
	api.py_new_uint64 = py_new_uint64;
	api.py_new_float = py_new_float;
	api.py_new_exception = py_new_exception;

	api.array_create = array_create;
	api.array_size = array_size;

	api.array_append = array_append;
	api.array_append_string = array_append_string;
	api.array_append_int = array_append_int;
	api.array_append_double = array_append_double;
	api.array_append_uint64 = array_append_uint64;

	api.dict_create = dict_create;
	api.dict_add = dict_add;
	api.dict_get_value = dict_get_value;
	return &api;
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


