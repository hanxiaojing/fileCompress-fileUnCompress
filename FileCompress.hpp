#define _CRT_SECURE_NO_WARNINGS 1
#pragma once

#include<iostream>
#include<string>
#include<assert.h>
#include"Huffman.hpp"
using namespace std;

//设置有效字符信息
typedef unsigned long long LongType;
struct CharInfo
{
	unsigned char _ch;   
	LongType _count;    
	string _code;			 
	CharInfo(LongType count = 0)
		:_ch(0)
		, _count(count)
	{}
	bool operator!=(const CharInfo& info)		const
	{
		return _count != info._count;
	}
	CharInfo operator + (const CharInfo& info) 
	{
		return _count + info._count;
	}
	bool operator<(const CharInfo& info)
	{
		return _count < info._count;
	}
};

class FileCompress
{
public:
	//构造函数，初始化字符数组
	FileCompress()
	{
		for (int i= 0; i < 256; i++)
		{
			_info[i]._ch = i;
		}
	}	
	//压缩
	void Compress(const char* fileName)
	{
		assert(fileName);
		// 1. 读取文件，统计每个字符出现的次数
		FILE *fOut = fopen(fileName, "rb"); //以二进制流读文件
		assert(fOut);
		cout <<endl<< "	文件打开成功，开始统计字符信息 ..." << endl;
		char ch = fgetc(fOut);
		int count = 0;  
		while (ch != EOF)
		{
			count++;
			_info[(unsigned char)ch]._count++;
			ch = fgetc(fOut);			
		}
		printf("	(共出现%d个字符)\n\n",count);
		// 2. 构建Huffman树，并生成Huffman编码
		cout << "	正在生成Huffman编码 ..." << endl;
		CharInfo invalid(0);
		HuffmanTree<CharInfo> tree(_info, 256, invalid);
		string code;
		GenerateHuffmanCode(tree.GetTop(), code);

		// 3. 将Huffman编码写入压缩文件
		cout << "	正在将压缩字符写入压缩文件 ..." << endl;
		string CompressFileName = fileName;
		CompressFileName += ".compress";
		FILE* finCompress = fopen(CompressFileName.c_str(), "wb");
		assert(finCompress);
		fseek(fOut, 0, SEEK_SET);
		ch = fgetc(fOut);
		unsigned char value = 0;
		int pos = 0;
		while (ch != EOF)
		{
				for (size_t i = 0; i <_info[(unsigned char)ch]._code.size(); i++)
				{
						value <<= 1;
						if (_info[(unsigned char)ch]._code[i] == '1')
							value |= 1;
		
						if (++pos==8)
						{
							// 将压缩字符写入文件，重置并继续进行链接
							fputc(value, finCompress);
							pos = 0;
							value = 0;
					}

				}
				ch = fgetc(fOut);
			}
			if (pos!=0)
			{	//不足8位			
				value = value << (8 - pos);
				fputc(value, finCompress);
			}

		// 4.写配置文件
		string configFileName = fileName;
		configFileName += ".config";
		FILE* finConfig = fopen(configFileName.c_str(), "wb");
		assert(finConfig);
	
		
		char strCount[32];
		_itoa(count , strCount, 10);
		fputs("The total number of characters :	", finConfig);
		fputs(strCount, finConfig);
		fputc('\n', finConfig);

		for (int i = 0; i < 256; i++)
		{
			if (_info[i] != invalid)
			{
				fputc(_info[i]._ch, finConfig);	
				fputc(',', finConfig);
				_itoa((int)_info[i]._count, strCount, 10);
				fputs(strCount, finConfig);
				fputc('\n', finConfig);
			}
		}
		fclose(fOut);
		fclose(finCompress);
		fclose(finConfig);
		
	}
	//解压缩
	void UnCompress(const char* fileName)
	{
		// 1. 读取配置文件信息，初始化_info[]
		cout << endl<<"	正在读取配置文件信息 ..." << endl;
		string configFileName = fileName;
		configFileName += ".config";
		FILE* foutConfig = fopen(configFileName.c_str(), "rb");
		string line;
		unsigned char ch ;
		GetLine(foutConfig, line);
		line.clear();
	
		while (GetLine(foutConfig, line))
		{
			if (!line.empty())
			{
				ch = line[0];
				_info[ch]._count = atoi(line.substr(2).c_str());
				line.clear();
			}
			else	
				line = '\n';
		}
	
		// 2.根据读出的配置信息，重建Huffman树
	
		CharInfo invalid(0);
		HuffmanTree<CharInfo> tree(_info,256,invalid);

		// 3.读取压缩文件信息，解压缩
		
		string uncompressFileName = fileName;
		uncompressFileName += ".uncompress";
		FILE* fin = fopen(uncompressFileName.c_str(), "wb");
		string compressFileName = fileName;
		compressFileName += ".compress";
		FILE* fout = fopen(compressFileName.c_str(), "rb");
		assert(fin);
		assert(fout);

		HuffmanTreeNode<CharInfo>* root = tree.GetTop();
		HuffmanTreeNode<CharInfo>* cur = root;
		ch = fgetc(fout);
		int pos = 8;
		LongType charCount = root->_weight._count;  //字符总数
		while (charCount > 0)
		{
			//找叶子节点
			while (cur!=NULL && cur->_left!=NULL && cur->_right!=NULL)
			{
				if (pos == 0)
				{
					pos = 8;
					ch = fgetc(fout);
				}
				--pos;

				if (ch & (1 << pos))
				{
					cur = cur->_right;
				}
				else
				{
					cur = cur->_left;
				}
			}
			//cur是叶子节点
			if (cur && charCount--)
			{
				fputc(cur->_weight._ch, fin);
				cur = root;
			}
		}
		
		fclose(fout);
		fclose(fin);
		fclose(foutConfig);
		
	}

protected:
	//Huffman编码
	void GenerateHuffmanCode(HuffmanTreeNode<CharInfo>* root, string code)
	{
		assert(root);
		if (root->_left != NULL&&root->_right != NULL)
		{
			GenerateHuffmanCode(root->_left, code + '0');
			GenerateHuffmanCode(root->_right, code + '1');
		}
		if (root->_left == NULL&&root->_right == NULL)
		{	//此时的root为叶子节点，即要进行编码的节点
			_info[root->_weight._ch]._code = code;
		}
	}
	// 读取文件中的一行
	bool GetLine(FILE* fout, string &line)
	{
		char ch = fgetc(fout);
		if (ch == EOF)
			return false;

		while (ch!=EOF && ch != '\n')
		{
			line += ch;
			ch = fgetc(fout);
		}
		return true;
	}

protected:
	CharInfo _info[256];
};



