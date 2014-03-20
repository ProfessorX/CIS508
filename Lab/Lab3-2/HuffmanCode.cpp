/*
* Copyright TES@NPU
* All rights reserved.
*
* FileName: HuffmanCode.cpp
* 
* Version: 1.0
* Updated @ Dec. 30 2011
* Finished @ Dec. 30 2011
 */

/* 哈弗曼编码 */
#include "HuffmanCode.h"

/*
	2011-12-30 C503 09:35
	Remaining few days within the year
	2011, and the lady still hates me...
	苦逼的一年就这样即将离开我们...哎...
	2011-12-30 C503 12:10
	有时候，的确貌似是我们的生活太苦逼了。
	不过，人生有太多太多的不得已。只得，
	拍拍身上的灰尘，然后继续前行。
	2011-12-30 C503 12:30
	HuffmanCoding()中某些过程有问题，吃晚饭
	继续Debug.
	2011-12-30 C503 16:00
	要以不一样的方式来迎接新年，那就是研究课题，
	年终总结。Debugging...
	2011-12-30 C503 18:18
	这一个的编码程序中，根本就没有赋0值。问题就出在
	HuffmanCoding()上面，继续修改。
 */

int min(HuffmanTree t, int i)
{
	/*
		函数void select()调用，
		两个结点互相比较，求较小者
	 */
	int j;
	int flag;
	// limits.h中定义了若干数据类型的上下极限
	unsigned int k = UINT_MAX;  // 取k为不小于可能的值
	for (j=1; j<=i; j++)
	{
		if (t[j].weight<k && t[j].parent==0)
		{
			k = t[j].weight;
			flag = j;
		}// end of if
	}// end of for
	t[flag].parent = 1;  // 最后一个结点的双亲置为1
	return flag;
}

void select(HuffmanTree t, int i, int &s1, int &s2)
{
	int j;
	s1 = min(t, i);
	s2 = min(t, i);
	if (s1 > s2)
	{
		// 微型的冒泡排序
		j = s1;
		s1 = s2;
		s2 = j;
	}// end of if
}

void HuffmanCoding(HuffmanTree &HT, HuffmanCode &HC, 
	int *w, int n)  // Algo 6.12
{
	/*
		w存放n个字符的权值（均>0），构造哈夫曼树HT，
		并求出n个字符的哈弗曼编码HC
	 */
	int m;
	int i;
	int s1;
	int s2;
	int start;
	unsigned c;
	unsigned f;
	HuffmanTree p;
	char *cd;

	if (n <= 1)
	{
		// 这一点在面试的时候常考，算作是错误处理。
		return;
	}
	m = 2*n-1;
	// 分配的0号单元未用
	HT = (HuffmanTree)malloc((m+1) * sizeof(HTNode));
	for (p=HT+1,i=1; i<=n; ++i,++p,++w)
	{
		// 初始化，逐一赋值
		(*p).weight = *w;
		(*p).parent = 0;
		(*p).lchild = 0;
		(*p).rchild = 0;
	}
	for (; i<=m; ++i,++p)
	{
		(*p).parent = 0;
	}
	for (i=n+1; i<=m; ++i)  // 建立哈夫曼树
	{
		/*
			在HT[1~i-1]中选择parent为0且weight最小
			的两个结点，其序号分别为s1和s2
		 */
		select(HT, i-1, s1, s2);
		HT[s2].parent = i;
		HT[s1].parent = HT[s2].parent;  // 感觉这一行有点问题
		HT[i].lchild = s1;
		HT[i].rchild = s2;
		HT[i].weight = HT[s1].weight + HT[s2].weight;
	}
	// 写到这一步，感觉没有出错。

	/* 从叶子到根逆向求每个字符的哈弗曼编码 */
	// 分配n个字符编码的头指针向量（[0]不用）
	HC = (HuffmanCode)malloc((n+1) * sizeof(char*));
	// 分配求编码的工作空间
	cd = (char*)malloc(n * sizeof(char));
	cd[n-1] = '\0';  // 编码结束符
	for (i=1; i<=n; i++)
	{
		// 逐个字符求哈弗曼编码
		start = n-1;  // 编码结束符位置，逆向求
		for (c=i,f=HT[i].parent; f!=0; 
			c=f, f=HT[f].parent)
		{
			// 从叶子到根逆向求编码
			if (HT[i].lchild == c)
			{
				cd[--start] = '0';
			}
			else
			{
				cd[--start] = '1';
			}// end of if...else
		}// end of for
		// 为第i个字符编码分配空间
		HC[i] = (char*)malloc((n-start) * sizeof(char));
		strcpy(HC[i], &cd[start]);  // 从cd复制编码串到HC
		// strcpy_s(HC[i], &cd[start]);
	}// end of for
	free(cd);  // 释放临时工作空间

	// 整个函数检查完感觉没有错误...
}

void main()
{
	HuffmanTree HT;
	HuffmanCode HC;
	int *w;
	int n;
	int i;

	cout << "请输入权值的个数（>1）：";
	// cin >> n;
	scanf_s("%d", &n);  // 换教材上给的这个函数试一下

	w = (int*)malloc(n * sizeof(int));
	cout << "请依次输入" << n <<"个权值（整型）："
		 << endl;
	for (i=0; i<=n-1; i++)
	{
		// cin >> (w+i)
		scanf_s("%d", w+i);
	}// end of for

	HuffmanCoding(HT, HC, w, n);
	for (i=1; i<=n; i++)
	{
		// cout << HC[i] << endl;
		puts(HC[i]);
	}

	system("pause");
}