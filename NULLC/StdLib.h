#pragma once

struct ArrayPtr{ char* ptr; int len; };

double nullcCos(double deg);
double nullcSin(double deg);
double nullcTan(double deg);
double nullcCtg(double deg);

double nullcCosh(double deg);
double nullcSinh(double deg);
double nullcTanh(double deg);
double nullcCoth(double deg);

double nullcAcos(double deg);
double nullcAsin(double deg);
double nullcAtan(double deg);

double nullcCeil(double num);
double nullcFloor(double num);
double nullcExp(double num);
double nullcLog(double num);

double nullcSqrt(double num);

int strEqual(ArrayPtr a, ArrayPtr b);
int strNEqual(ArrayPtr a, ArrayPtr b);
