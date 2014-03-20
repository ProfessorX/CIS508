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

/* ���������� */
#include "HuffmanCode.h"

/*
	2011-12-30 C503 09:35
	Remaining few days within the year
	2011, and the lady still hates me...
	��Ƶ�һ������������뿪����...��...
	2011-12-30 C503 12:10
	��ʱ�򣬵�ȷò�������ǵ�����̫����ˡ�
	������������̫��̫��Ĳ����ѡ�ֻ�ã�
	�������ϵĻҳ���Ȼ�����ǰ�С�
	2011-12-30 C503 12:30
	HuffmanCoding()��ĳЩ���������⣬����
	����Debug.
	2011-12-30 C503 16:00
	Ҫ�Բ�һ���ķ�ʽ��ӭ�����꣬�Ǿ����о����⣬
	�����ܽᡣDebugging...
	2011-12-30 C503 18:18
	��һ���ı�������У�������û�и�0ֵ������ͳ���
	HuffmanCoding()���棬�����޸ġ�
 */

int min(HuffmanTree t, int i)
{
	/*
		����void select()���ã�
		������㻥��Ƚϣ����С��
	 */
	int j;
	int flag;
	// limits.h�ж����������������͵����¼���
	unsigned int k = UINT_MAX;  // ȡkΪ��С�ڿ��ܵ�ֵ
	for (j=1; j<=i; j++)
	{
		if (t[j].weight<k && t[j].parent==0)
		{
			k = t[j].weight;
			flag = j;
		}// end of if
	}// end of for
	t[flag].parent = 1;  // ���һ������˫����Ϊ1
	return flag;
}

void select(HuffmanTree t, int i, int &s1, int &s2)
{
	int j;
	s1 = min(t, i);
	s2 = min(t, i);
	if (s1 > s2)
	{
		// ΢�͵�ð������
		j = s1;
		s1 = s2;
		s2 = j;
	}// end of if
}

void HuffmanCoding(HuffmanTree &HT, HuffmanCode &HC, 
	int *w, int n)  // Algo 6.12
{
	/*
		w���n���ַ���Ȩֵ����>0���������������HT��
		�����n���ַ��Ĺ���������HC
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
		// ��һ�������Ե�ʱ�򳣿��������Ǵ�����
		return;
	}
	m = 2*n-1;
	// �����0�ŵ�Ԫδ��
	HT = (HuffmanTree)malloc((m+1) * sizeof(HTNode));
	for (p=HT+1,i=1; i<=n; ++i,++p,++w)
	{
		// ��ʼ������һ��ֵ
		(*p).weight = *w;
		(*p).parent = 0;
		(*p).lchild = 0;
		(*p).rchild = 0;
	}
	for (; i<=m; ++i,++p)
	{
		(*p).parent = 0;
	}
	for (i=n+1; i<=m; ++i)  // ������������
	{
		/*
			��HT[1~i-1]��ѡ��parentΪ0��weight��С
			��������㣬����ŷֱ�Ϊs1��s2
		 */
		select(HT, i-1, s1, s2);
		HT[s2].parent = i;
		HT[s1].parent = HT[s2].parent;  // �о���һ���е�����
		HT[i].lchild = s1;
		HT[i].rchild = s2;
		HT[i].weight = HT[s1].weight + HT[s2].weight;
	}
	// д����һ�����о�û�г���

	/* ��Ҷ�ӵ���������ÿ���ַ��Ĺ��������� */
	// ����n���ַ������ͷָ��������[0]���ã�
	HC = (HuffmanCode)malloc((n+1) * sizeof(char*));
	// ���������Ĺ����ռ�
	cd = (char*)malloc(n * sizeof(char));
	cd[n-1] = '\0';  // ���������
	for (i=1; i<=n; i++)
	{
		// ����ַ������������
		start = n-1;  // ���������λ�ã�������
		for (c=i,f=HT[i].parent; f!=0; 
			c=f, f=HT[f].parent)
		{
			// ��Ҷ�ӵ������������
			if (HT[i].lchild == c)
			{
				cd[--start] = '0';
			}
			else
			{
				cd[--start] = '1';
			}// end of if...else
		}// end of for
		// Ϊ��i���ַ��������ռ�
		HC[i] = (char*)malloc((n-start) * sizeof(char));
		strcpy(HC[i], &cd[start]);  // ��cd���Ʊ��봮��HC
		// strcpy_s(HC[i], &cd[start]);
	}// end of for
	free(cd);  // �ͷ���ʱ�����ռ�

	// �������������о�û�д���...
}

void main()
{
	HuffmanTree HT;
	HuffmanCode HC;
	int *w;
	int n;
	int i;

	cout << "������Ȩֵ�ĸ�����>1����";
	// cin >> n;
	scanf_s("%d", &n);  // ���̲��ϸ������������һ��

	w = (int*)malloc(n * sizeof(int));
	cout << "����������" << n <<"��Ȩֵ�����ͣ���"
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