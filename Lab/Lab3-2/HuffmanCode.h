/*
* Copyright TES@NPU
* All rights reserved.
*
* FileName: HuffmanCode.h
* 
* Version: 1.0
* Updated @ Dec. 30 2011
* Finished @ Dec. 30 2011
 */

#include <string.h>
#include <ctype.h>
#include <malloc.h>  //mallc() and so on
#include <limits.h>  //INT_MAX and so on
#include <stdio.h>  //EOF(=^Z or F6), NULL
#include <stdlib.h>  //atoi()
#include <io.h>  //eof()
#include <math.h>  //floor(), ceil(), abs()
#include <process.h>  //exit()
#include <iostream>  //cout, cin

using namespace std;

// Codes for the result of functions
#define TRUE 1
#define FALSE 0
#define OK 1
#define ERROR 0
#define INFEASIBLE -1
// #define OVERFLOW -2 for there was the definition 
// in file "math.h" so it could be omitted

typedef int Status;  // Status是函数类型， 其值是函数结果状态代码，如OK等
typedef int Boolean;  // Boolean是布尔类型，其值是TRUE或FALSE


/* 哈夫曼树和哈弗曼编码的存储表示 */
// 动态分配数组存储哈夫曼树
typedef struct 
{
	unsigned int weight;
	unsigned int parent;
	unsigned int lchild;
	unsigned int rchild;
}HTNode, *HuffmanTree;

typedef char **HuffmanCode;  // 动态分配数组存储哈弗曼编码表

/* 操作函数的声明部分 */
int min(HuffmanTree t, int i);

void select(HuffmanTree t, int i, int &s1, int &s2);
void HuffmanCoding(HuffmanTree &HT, HuffmanCode &HC, 
	int *w, int n);
