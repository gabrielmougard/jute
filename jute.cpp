#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include "jute.h"
using namespace std;
using namespace jute;
string jValue::makesp(int d) {
	string s = "";
	while (d--) s += "  ";
	return s;
}
string jValue::to_string_d(int d) {
	if (type == JSTRING)   return string("\"") + svalue + string("\"");
	if (type == JNUMBER)   return svalue;
	if (type == JBOOLEAN)  return svalue;
	if (type == JNULL)     return "null";
	if (type == JOBJECT) {
		string s = string("{\n");
		for (int i=0;i<properties.size();i++) {
			s += makesp(d) + string("\"") + properties[i].first + string("\": ") + properties[i].second.to_string_d(d+1) + string(i==properties.size()-1?"":",") + string("\n");
		}
		s += makesp(d-1) + string("}");
		return s;
	}
	if (type == JARRAY) {
		string s = "[";
		for (int i=0;i<arr.size();i++) {
			if (i) s += ", ";
			s += arr[i].to_string_d(d+1);
		}
		s += "]";
		return s;
	}
	return "##";
}
jValue::jValue(jType tp = JUNKNOWN) {
	this->type = tp;
}

string jValue::to_string() {
	return to_string_d(1);
}
jType jValue::get_type() {
	return type;
}
void jValue::set_type(jType tp) {
	type = tp;
}
void jValue::add_property(string key, jValue v) {
	mpindex[key] = properties.size();
	properties.push_back(make_pair(key, v));
}
void jValue::add_element(jValue v) {
	arr.push_back(v);
}
void jValue::set_string(string s) {
	svalue = s;
}
int jValue::as_int() {
	stringstream ss;
	ss << svalue;
	int k;
	ss >> k;
	return k;
}
double jValue::as_double() {
	stringstream ss;
	ss << svalue;
	double k;
	ss >> k;
	return k;
}
bool jValue::as_bool() {
	if (svalue == "true") return true;
	return false;
}
void* jValue::as_null() {
	return NULL;
}
string jValue::as_string() {
	return svalue;
}
jValue jValue::operator[](int i) {
	if (type == JARRAY) {
		return arr[i];
	}
	if (type == JOBJECT) {
		return properties[i].second;
	}
	return jValue();
}
jValue jValue::operator[](string s) {
	if (mpindex.find(s) == mpindex.end()) return jValue();
	return properties[mpindex[s]].second;
}

struct parser::token {
	string value;
	token_type type;
	token(string value="",token_type type=UNKNOWN): value(value), type(type) {}
};
bool parser::is_whitespace(const char c) {
	return string(" \n\r	").find(c) != string::npos;
}
int parser::next_whitespace(const string& source, int i) {
	while (i<source.length()) {
		if (source[i] == '"') {
			i++;
			while (i < source.length() && (source[i] != '"' || source[i-1] == '\\')) i++;
		}
		if (source[i] == '\'') {
			i++;
			while (i < source.length() && (source[i] != '\'' || source[i-1] == '\\')) i++;
		}
		if (is_whitespace(source[i])) return i;
		i++;
	}
	return source.length();
}
int parser::skip_whitespaces(const string& source, int i) {
	while (i < source.length()) {
		if (!is_whitespace(source[i])) return i;
		i++;
	}
	return -1;
}

vector<parser::token> parser::tokenize(string source) {
	source += " ";
	vector<token> tokens;
	int index = skip_whitespaces(source, 0);
	while (index >= 0) {
		int next = next_whitespace(source, index);
		string str = source.substr(index, next-index);
		
		int k=0;
		int t=-1;
		while (k < str.length()) {
			if (str[k] == '"') {
				int tmp_k = k+1;
				while (tmp_k < str.length() && (str[tmp_k] != '"' || str[tmp_k-1] == '\\')) tmp_k++;
				tokens.push_back(token(str.substr(k+1, tmp_k-k-1), STRING));
				k = tmp_k+1;
				continue;
			}
			if (str[k] == '\'') {
				int tmp_k = k+1;
				while (tmp_k < str.length() && (str[tmp_k] != '\'' || str[tmp_k-1] == '\\')) tmp_k++;
				tokens.push_back(token(str.substr(k+1, tmp_k-k-1), STRING));
				k = tmp_k+1;
				continue;
			}
			if (str[k] == ',') {
				tokens.push_back(token(",", COMMA));
				k++;
				continue;
			}
			if (str[k] == 't' && k+3 < str.length() && str.substr(k, 4) == "true") {
				tokens.push_back(token("true", BOOLEAN));
				k += 4;
				continue;
			}
			if (str[k] == 'f' && k+4 < str.length() && str.substr(k, 5) == "false") {
				tokens.push_back(token("false", BOOLEAN));
				k += 5;
				continue;
			}
			if (str[k] == 'n' && k+3 < str.length() && str.substr(k, 4) == "null") {
				tokens.push_back(token("null", NUL));
				k += 4;
				continue;
			}
			if (str[k] == '}') {
				tokens.push_back(token("}", CROUSH_CLOSE));
				k++;
				continue;
			}
			if (str[k] == '{') {
				tokens.push_back(token("{", CROUSH_OPEN));
				k++;
				continue;
			}
			if (str[k] == ']') {
				tokens.push_back(token("]", BRACKET_CLOSE));
				k++;
				continue;
			}
			if (str[k] == '[') {
				tokens.push_back(token("[", BRACKET_OPEN));
				k++;
				continue;
			}
			if (str[k] == ':') {
				tokens.push_back(token(":", COLON));
				k++;
				continue;
			}
			if (str[k] == '-' || (str[k] <= '9' && str[k] >= '0')) {
				int tmp_k = k;
				if (str[tmp_k] == '-') tmp_k++;
				while (tmp_k < str.size() && ((str[tmp_k] <= '9' && str[tmp_k] >= '0') || str[tmp_k] == '.')) tmp_k++;
				tokens.push_back(token(str.substr(k, tmp_k-k), NUMBER));
				k = tmp_k;
				continue;
			}
			tokens.push_back(token(str.substr(k), UNKNOWN));
			k = str.length();
		}
		
		index = skip_whitespaces(source, next);
	}
	// for (int i=0;i<tokens.size();i++) {
		// cout << i << " " << tokens[i].value << endl;
	// }
	return tokens;
}
	

jValue parser::json_parse(vector<token> v, int i, int& r) {
	jValue current;
	if (v[i].type == CROUSH_OPEN) {
		current.set_type(JOBJECT);
		int k = i+1;
		while (v[k].type != CROUSH_CLOSE) {
			string key = v[k].value;
			k+=2; // k+1 should be ':'
			int j = k;
			jValue vv = json_parse(v, k, j);
			current.add_property(key, vv);
			k = j;
			if (v[k].type == COMMA) k++;
		}
		r = k+1;
		return current;
	}
	if (v[i].type == BRACKET_OPEN) {
		current.set_type(JARRAY);
		int k = i+1;
		while (v[k].type != BRACKET_CLOSE) {
			int j = k;
			jValue vv = json_parse(v, k, j);
			current.add_element(vv);
			k = j;
			if (v[k].type == COMMA) k++;
		}
		r = k+1;
		return current;
	}
	if (v[i].type == NUMBER) {
		current.set_type(JNUMBER);
		current.set_string(v[i].value);
		r = i+1;
		return current;
	}
	if (v[i].type == STRING) {
		current.set_type(JSTRING);
		current.set_string(v[i].value);
		r = i+1;
		return current;
	}
	if (v[i].type == BOOLEAN) {
		current.set_type(JBOOLEAN);
		current.set_string(v[i].value);
		r = i+1;
		return current;
	}
	if (v[i].type == NUL) {
		current.set_type(JNULL);
		current.set_string("null");
		r = i+1;
		return current;
	}
	return current;
}

jValue parser::parse(const string& str) {
	int k;
	return json_parse(tokenize(str), 0, k);
}

