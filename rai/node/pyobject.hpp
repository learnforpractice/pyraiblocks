/*
 * pyobject.hpp
 *
 *  Created on: Sep 8, 2017
 *      Author: newworld
 */

#ifndef PYOBJECT_HPP_
#define PYOBJECT_HPP_

#include <Python.h>
#include <string>
#include <vector>

PyObject* py_new_none();
PyObject* py_new_bool(int b);
PyObject* py_new_string(std::string s);
PyObject* py_new_int(int n);
PyObject* py_new_int64(long long n);
PyObject* py_new_uint64(unsigned long long n);
PyObject* py_new_float(double n);

PyObject* array_create();
int array_size(PyObject* arr);

void array_append(PyObject* arr, PyObject* v);
void array_append_string(PyObject* arr, std::string& s);
void array_append_int(PyObject* arr, int n);
void array_append_double(PyObject* arr, double n);
void array_append_uint64(PyObject* arr, unsigned long long n);

PyObject* dict_create();
void dict_add(PyObject* d, PyObject* key, PyObject* value);
bool dict_get_value(PyObject* d, const char* key, std::string& value);

PyObject* py_new_exception(const char* error);

using namespace std;

class PyArray {
  public:
   PyArray();
   ~PyArray();
   void append(PyObject* obj);
   void append(std::string s);
   void append(int n);
   void append(unsigned int n);
   void append(uint64_t n);
   void append(double n);
   PyObject* get();
   int size();
  private:
   PyObject* arr;
};

class PyDict {
  public:
   PyDict();
   PyDict(PyObject* dictObj);
   ~PyDict();
   void add(PyObject* key, PyObject* value);
   void add(std::string key, PyObject* value);
//   void add(std::string& key, PyObject* value);
   void add(const char* key, std::string value);
//   void add(std::string& key, std::string& value);
   void add(std::string key, std::string value);
   void add(std::string& key, long long n);
   bool get_value(const char* key, std::string& value);
   PyObject* get();
   int size();
  private:
   PyObject* pydict;
};

#endif /* PYOBJECT_HPP_ */
