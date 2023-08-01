#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define APP_VERSION     "1.0.0"

#define  LOOP 1000 //times of compression

#define A       0
#define R       1
#define G       2
#define B       3
#define ARGB    4

// error codes
#define ERROR_OK			0
#define ERROR_PARAM_NOT_ENOUGH		-1
#define ERROR_INVALID_PARAM		-2	
#define ERROR_INPUT_FILE		-3
#define ERROR_OUTPUT_FILE		-4
#define ERROR_INVALID_INPUT_FILE	-5
#define ERROR_CUSTOM			-6


FILE *fp1;
FILE *fp2;

void bmp_dataread();            //bmp数据读取
void bmp_headwrite();
void enfile_headwrite();
void enfile_dataread();
void predict_LOCO_I();          //LOCO-I预测
void restore_LOCO_I();          //LOCO-I还原
void LOCO_I_encode();           //LOCO-I编码
void LOCO_I_decode();           //LOCO-I解码
void code_length_compress();    //游程编码
void code_length_decompress();  //游程解码
void select_huf();                  //选择最小的两个节点以建立哈夫曼树
void CreatTree();               //建立哈夫曼树
void HufCode();                 //生成哈夫曼编码
int huffman_compress();         //哈夫曼压缩
int huffman_extract();          //哈夫曼解压
int getFileSize();              //计算文件大小

//ARGB数据通道
typedef struct{                
    unsigned char color_A;
    unsigned char color_R;
    unsigned char color_G;
    unsigned char color_B;
}BLOCK_DATA;

// 统计字符频度的临时结点
typedef struct {
	unsigned char uch;			// 以8bits为单元的无符号字符
	unsigned long weight;		// 每类（以二进制编码区分）字符出现频度
} TmpNode;

// 哈夫曼树结点
typedef struct {
	unsigned char uch;				// 以8bits为单元的无符号字符
	unsigned long weight;			// 每类（以二进制编码区分）字符出现频度
	char *code;						// 字符对应的哈夫曼编码（动态分配存储空间）
	int parent, lchild, rchild;		// 定义双亲和左右孩子
} HufNode, *HufTree;

//全局变量
int width;
int height;
int block_width;
int block_height;

int maxd(int a,int b){
	a = a>b?a:b;
	return a;
}
int mind(int a,int b){
	a = a>b?b:a;
	return a;
}

void bmp_dataread(char *ifname){
    int i;
    int offset;
    unsigned char temp;
    FILE *infile;
    FILE *outdatafile;
    infile = fopen(ifname,"rb");
    
    fseek(infile,sizeof(unsigned char)*10, SEEK_SET);//获取头文件字节数
    fread(&offset, sizeof(int), 1, infile);

    fseek(infile,sizeof(unsigned char)*18, SEEK_SET);//获取像素宽高
    fread(&width, sizeof(int), 1, infile);
    fread(&height, sizeof(int), 1, infile);

    fseek(infile,sizeof(unsigned char)*offset,SEEK_SET);//
    outdatafile = fopen("bmp.dat","wb");//数据文件
    while(1){
        fread(&temp, sizeof(unsigned char), 1, infile);
        if(feof(infile))break;
        fwrite(&temp, sizeof(unsigned char), 1, outdatafile);
    }
    fclose(infile);
    fclose(outdatafile);
}

void bmp_headwrite(char *ifname, int width, int height){
    int i;
    int data_size;
    int temp;
    FILE *infile;
    FILE *outfile;
    infile = fopen("recov.dat","rb");
    outfile = fopen(ifname,"wb");
    temp = 19778;
        fwrite(&temp, sizeof(unsigned short), 1, outfile);
    temp = 4*width*height+122;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 0;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 122;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);

    temp = 108;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = width;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = height;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);

    temp = 1;
        fwrite(&temp, sizeof(unsigned short), 1, outfile);
    temp = 32;
        fwrite(&temp, sizeof(unsigned short), 1, outfile);
    temp = 3;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);

    temp = 0;
        for(i=0;i<11;i++)fwrite(&temp, sizeof(unsigned short), 1, outfile);
    temp = 4278190335;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 16711680;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 0;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 65280;
        fwrite(&temp, sizeof(unsigned int), 1, outfile);
    temp = 0;
        for(i=0;i<25;i++)fwrite(&temp, sizeof(unsigned short), 1, outfile);
        
    while(1){
        fread(&temp, sizeof(unsigned char), 1, infile);
        if(feof(infile))break;
        fwrite(&temp, sizeof(unsigned char), 1, outfile);
    }
    fclose(infile);
    fclose(outfile);
}

//输入文件loco_length_huf.dat
//输出总压缩文件，自定义名称
void enfile_headwrite(char *ifname, int width, int height, int block_width, int block_height){
    int i;
    unsigned long temp;
    FILE *infile;
    FILE *outfile;
    infile = fopen("loco_length_huf.dat","rb");
    outfile = fopen(ifname,"wb");
    temp = 0x4C4A4443;//JLCD
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = width;//图像宽度
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = height;//图像高度
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = block_width;//数据块宽度
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = block_height;//数据块高度
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = (width*height)/(block_width*block_height);//数据块总数
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    temp = 20;//数据偏移地址
        fwrite(&temp, sizeof(unsigned char)*4, 1, outfile);
    
    while(1){
        fread(&temp, sizeof(unsigned char), 1, infile);
        if(feof(infile))break;
        fwrite(&temp, sizeof(unsigned char), 1, outfile);
    }
    fclose(infile);
    fclose(outfile);
}

//解压文件名称，自定义
void enfile_dataread(char *ifname){
    int i;
    int offset;
    unsigned long temp;
    FILE *infile;
    FILE *outdatafile;
    infile = fopen(ifname,"rb");

    fseek(infile,sizeof(unsigned char)*4, SEEK_SET);//获取像素宽高
    fread(&width, sizeof(int), 1, infile);
    fread(&height, sizeof(int), 1, infile);
    fread(&block_width, sizeof(int), 1, infile);
    fread(&block_height, sizeof(int), 1, infile);

    fseek(infile,sizeof(unsigned char)*28,SEEK_SET);//
    outdatafile = fopen("loco_length_huf.dat","wb");//数据文件
    while(1){
        fread(&temp, sizeof(unsigned char), 1, infile);
        if(feof(infile))break;
        fwrite(&temp, sizeof(unsigned char), 1, outdatafile);
    }
    fclose(infile);
    fclose(outdatafile);
}

void predict_LOCO_I(unsigned char *data, int width, int height, unsigned char *difft){
    int i;
    unsigned char pred; 
    for(i=0;i<width*height;i++){
        if(i == 0) pred = 0;
        //上边界：预测数据 = 左数据
        else if(i/width == 0) pred = data[i-1];
        //左边界：预测数据 = 上数据   
        else if(i%width == 0) pred = data[i-width];
        else{
            if(data[i - (width+1)] >= maxd(data[i-1], data[i-width])){
                pred = mind(data[i-1], data[i - width]);
            }else if(data[i - (width+1)] <= mind(data[i-1], data[i-width])){
                pred = maxd(data[i-1], data[i-width]);
            }else{
                pred = data[i-1] + data[i-width] - data[i-(width+1)];
            }
        }
        difft[i] = data[i] ^ pred;
    }
}

void restore_LOCO_I(unsigned char *difft, int width, int height, unsigned char *data){
    int i;
    unsigned char pred;
    for(i=0;i<width*height;i++){
        if(i == 0) pred = 0;
        else if(i/width == 0) pred = data[i-1];
        else if(i%width == 0) pred = data[i-width]; 
        else{
            if(data[i - (width+1)] >= maxd(data[i-1], data[i-width])){
                pred = mind(data[i-1], data[i - width]);
            }else if(data[i - (width+1)] <= mind(data[i-1], data[i-width])){
                pred = maxd(data[i-1], data[i-width]);
            }else{
                pred = data[i-1] + data[i-width] - data[i-(width+1)];
            }
        }
        data[i] = difft[i] ^ pred;
    }
}

//输入为bmp.dat
//输出为dif.dat
void LOCO_I_encode(char *ifname, int width, int height, int block_width, int block_height, int n){
    int i, j;
    int w, h;
    int w_div, h_div;
    unsigned char dif[ARGB][block_width*block_height];
    FILE *infile;
    FILE *outfile;
    FILE *outdif;
    FILE *indif;
    BLOCK_DATA block_data[block_height*block_width];//用于存储块数据
    unsigned char data_temp[ARGB][block_height*block_width];//结构体数据读出转存，用于后续预测差值
    unsigned char data_out[ARGB][block_height*block_width];//根据差值解压的数据

    infile = fopen("bmp.dat","rb");

    if((width%block_width==0) && (height%block_height==0)){
        w_div = width / block_width;
        h_div = height / block_height;
    }else{
        printf("error!, Integer decompression is not possible");
        exit(1);
    }
    if(n<0 || n>w_div*h_div){
        printf("error!, Out of bounds or not supported");
        exit(1);
    }else{ 
        //文件光标移动到所需块的开头
        fseek(infile,ARGB*sizeof(unsigned char)*(width*block_height*(int)(n/w_div) + block_width*(n%w_div)), SEEK_SET);
    }
    //从源文件读取ARGB数据
    for(h=0;h<block_height;h++){
        for(i=0;i<block_width;i++){
            fread(&block_data[i+h*block_width].color_A, sizeof(unsigned char), 1, infile);
            fread(&block_data[i+h*block_width].color_R, sizeof(unsigned char), 1, infile);
            fread(&block_data[i+h*block_width].color_G, sizeof(unsigned char), 1, infile);
            fread(&block_data[i+h*block_width].color_B, sizeof(unsigned char), 1, infile); 
        }
        //文件光标移动到下一行
        fseek(infile, ARGB*sizeof(unsigned char)*(width-block_width),SEEK_CUR);    
    }
    fclose(infile);
    //将数据转到数组
    for(i=0;i<block_width*block_height;i++){
        data_temp[A][i] = block_data[i].color_A;
        data_temp[R][i] = block_data[i].color_R;
        data_temp[G][i] = block_data[i].color_G;
        data_temp[B][i] = block_data[i].color_B;
    }
    //预测差值
    for(i=0;i<ARGB;i++)
        predict_LOCO_I(data_temp[i], block_width, block_height, dif[i]);       

	outdif = fopen("dif.dat", "rb+");

    /*
    将指定块差值数据存储到指定位置
    排列按照全部的A，全部的R...G...B...存入差值文件
    一共有 width*height*ARGB*sizeof(unsigned char)位数据
    ARGB分别占用四等份
    A从width*height*sizeof(unsigned char)*A开始，存入width*height*sizeof(unsigned char)个数据
    R从width*height*sizeof(unsigned char)*R开始
    G从width*height*sizeof(unsigned char)*G开始
    B从width*height*sizeof(unsigned char)*B开始
    内部的ARGB各个小块又加上偏移sizeof(unsigned char)*block_width*block_height*n
    也就是说，指针定在width*height*sizeof(unsigned char)*i + sizeof(unsigned char)*block_width*block_height*n
    即sizeof(unsigned char)*（width*height*i+block_width*block_height*n）
    */
        fseek(outdif,sizeof(unsigned char)*(width*height*A+block_width*block_height*n), SEEK_SET);
        fwrite(dif[A], sizeof(unsigned char), block_width*block_height, outdif);
        fseek(outdif,sizeof(unsigned char)*(width*height*R+block_width*block_height*n), SEEK_SET);
        fwrite(dif[R], sizeof(unsigned char), block_width*block_height, outdif);
        fseek(outdif,sizeof(unsigned char)*(width*height*G+block_width*block_height*n), SEEK_SET);
        fwrite(dif[G], sizeof(unsigned char), block_width*block_height, outdif);
        fseek(outdif,sizeof(unsigned char)*(width*height*B+block_width*block_height*n), SEEK_SET);
        fwrite(dif[B], sizeof(unsigned char), block_width*block_height, outdif);
    fclose(outdif);
}

void LOCO_I_decode(char *ifname, int width, int height, int block_width, int block_height, int n){
    int i,j;
    int h,w;
    int w_div, h_div;
    unsigned char dif[ARGB][block_width*block_height];
    FILE *infile;
    FILE *outfile;
    unsigned char data_out[ARGB][block_height*block_width];//根据差值解压的数据
    unsigned char data_temp[block_height*block_width][ARGB];
    infile = fopen(ifname,"rb");

    if((width%block_width==0) && (height%block_height==0)){
        w_div = width / block_width;
        h_div = height / block_height;
    }else{
        printf("error!, Integer decompression is not possible");
        exit(1);
    }
    if(n<0 || n>w_div*h_div){
        printf("error!, Out of bounds or not supported");
        exit(1);
    }else{//排列按照全部的A，全部的R...G...B...读取差值文件
        fseek(infile,sizeof(unsigned char)*(width*height*A+block_width*block_height*n), SEEK_SET);
        fread(dif[A], sizeof(unsigned char), block_width*block_height, infile);
        fseek(infile,sizeof(unsigned char)*(width*height*R+block_width*block_height*n), SEEK_SET);
        fread(dif[R], sizeof(unsigned char), block_width*block_height, infile);
        fseek(infile,sizeof(unsigned char)*(width*height*G+block_width*block_height*n), SEEK_SET);
        fread(dif[G], sizeof(unsigned char), block_width*block_height, infile);
        fseek(infile,sizeof(unsigned char)*(width*height*B+block_width*block_height*n), SEEK_SET);
        fread(dif[B], sizeof(unsigned char), block_width*block_height, infile);
    }
    fclose(infile);

    //恢复数据
    for(i=0;i<ARGB;i++)
        restore_LOCO_I(dif[i], block_width, block_height,data_out[i]);  

    //将重建数据按照ARGB相间写入重建原文件位置   
    outfile = fopen("recov.dat", "rb+");
    fseek(outfile,ARGB*sizeof(unsigned char)*(width*block_height*(int)(n/w_div) + block_width*(n%w_div)), SEEK_SET);//文件光标移动到所需块的开头
    for(h=0;h<block_height;h++){
        if(h>0){
            fseek(outfile, ARGB*sizeof(unsigned char)*(width-block_width),SEEK_CUR);    //文件光标移动到下一行
        }
        for(i=0;i<block_width;i++){
            fwrite(&data_out[A][i+h*block_width], sizeof(unsigned char), 1, outfile);
            fwrite(&data_out[R][i+h*block_width], sizeof(unsigned char), 1, outfile);
            fwrite(&data_out[G][i+h*block_width], sizeof(unsigned char), 1, outfile);
            fwrite(&data_out[B][i+h*block_width], sizeof(unsigned char), 1, outfile); 
        }
    }
    fclose(outfile);
}

//输入为dif.dat
//输出为loco_length.dat
void code_length_compress(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "rb");
    FILE *output_file = fopen(output_filename, "wb");
    if (!input_file || !output_file) {
        perror("Failed to open file");
        return;
    }
    int flag = 0;
    unsigned char zero = 0;
    unsigned char count = 0;
    unsigned char current;
    while(1){
        fread(&current, sizeof(unsigned char), 1, input_file);
        if(feof(input_file))break;
        //printf("%02x ",current);//文件数据正确读出
        if (current==0){
            fread(&current, sizeof(unsigned char), 1, input_file);
            if(feof(input_file)){
                flag = 1;
                break;
            }
            //printf("%02x ",current);//文件数据正确读出
            if(current == 0){
                count++;
                if (count == 255) {
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);//先写两个0，再写重复次数
                    fwrite(&count, sizeof(unsigned char), 1, output_file);
                    count = 0;
                }
            }else{
                if(count > 0){
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);//先写两个0，再写重复次数
                    fwrite(&count, sizeof(unsigned char), 1, output_file);
                    count = 0;
                }
                fwrite(&zero, sizeof(unsigned char), 1, output_file);
                fwrite(&current, sizeof(unsigned char), 1, output_file);
            }
        }else{
            if (count > 0) {
                fwrite(&zero, sizeof(unsigned char), 1, output_file);
                fwrite(&zero, sizeof(unsigned char), 1, output_file);//先写两个0，再写重复次数
                fwrite(&count, sizeof(unsigned char), 1, output_file);
                count = 0;
            }
            fwrite(&current, sizeof(unsigned char), 1, output_file);
        }
    }
    if(count > 0){//如果文件末尾一直是0，但不够255个，则最后输出重复个数
        fwrite(&zero, sizeof(unsigned char), 1, output_file);
        fwrite(&zero, sizeof(unsigned char), 1, output_file);//先写两个0，再写重复次数
        fwrite(&count, sizeof(unsigned char), 1, output_file);
        count = 0;
    }
    if(flag == 1){
        fwrite(&zero, sizeof(unsigned char), 1, output_file);
        flag =0;
    }
        
    fclose(input_file);
    fclose(output_file);
}


void code_length_decompress(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "rb");
    FILE *output_file = fopen(output_filename, "wb");
    static const char zeros[] = "\0\0\0\0\0\0\0\0";
    if (!input_file || !output_file) {
        perror("Failed to open file");
        return;
    }
    int i;
    unsigned char zero = 0;
    unsigned char count = 0;
    unsigned char current;
    while (1) {
        fread(&current, sizeof(current), 1, input_file);
        if(feof(input_file))break;
        //printf("%02x ",current);//文件数据正确读出
        if(current == 0){
            fread(&current, sizeof(current), 1, input_file);
            if(feof(input_file)){
                fwrite(&zero, sizeof(unsigned char), 1, output_file);
                break;
            }
            //printf("%02x ",current);//文件数据正确读出
            if(current == 0){
                fread(&count, sizeof(current), 1, input_file);
                /*  基于该游程编码的文件末尾不可能存在两个字节的0，（大概）
                if(feof(input_file)){
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                    break;
                }
                */
                //printf("%02x ",count);//文件数据正确读出
                for(i=0;i<count;i++){
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                    fwrite(&zero, sizeof(unsigned char), 1, output_file);
                }
            }else{
                fwrite(&zero, sizeof(unsigned char), 1, output_file);
                fwrite(&current, sizeof(unsigned char), 1, output_file);
            }
        }else{
            fwrite(&current, sizeof(unsigned char), 1, output_file);
        }
    }

    fclose(input_file);
    fclose(output_file);
}
/*
void Specific(int start, int end, int width, int height, int block_width, int block_height){
    unsigned char data[end-start+1];//定义指定数据长度
    //首先看指定的数据范围在哪个块内
    //(width*block_height*(int)(n/w_div) + block_width*(n%w_div)) = start
    //首先判断end是否比start大，
    //找到这个范围占用哪些块，然后进行块压缩或者解压
    //再读取出来
}
*/
// 选择最小和次小的两个结点，建立哈夫曼树调用
void select_huf(HufNode *huf_tree, unsigned int n, int *s1, int *s2)
{
	// 找最小
	unsigned int i;
	unsigned long min = ULONG_MAX;
	for(i = 0; i < n; ++i)           
		if(huf_tree[i].parent == 0 && huf_tree[i].weight < min)
		{
			min = huf_tree[i].weight;
			*s1 = i;
		}
		huf_tree[*s1].parent=1;   // 标记此结点已被选中

	// 找次小
	min=ULONG_MAX;
	for(i = 0; i < n; ++i)            
		if(huf_tree[i].parent == 0 && huf_tree[i].weight < min)
		{
			min = huf_tree[i].weight;
			*s2 = i;
		}
} 

// 建立哈夫曼树
void CreateTree(HufNode *huf_tree, unsigned int char_kinds, unsigned int node_num)
{
	unsigned int i;
	int s1, s2;
	for(i = char_kinds; i < node_num; ++i)  
	{ 
		select_huf(huf_tree, i, &s1, &s2);		// 选择最小的两个结点
		huf_tree[s1].parent = huf_tree[s2].parent = i; 
		huf_tree[i].lchild = s1; 
		huf_tree[i].rchild = s2; 
		huf_tree[i].weight = huf_tree[s1].weight + huf_tree[s2].weight; 
	} 
}

// 生成哈夫曼编码
void HufCode(HufNode *huf_tree, unsigned char_kinds)
{
	unsigned int i;
	int cur, next, index;
	char *code_tmp = (char *)malloc(256*sizeof(char));		// 暂存编码，最多256个叶子，编码长度不超多255
	code_tmp[256-1] = '\0'; 

	for(i = 0; i < char_kinds; ++i) 
	{
		index = 256-1;	// 编码临时空间索引初始化

		// 从叶子向根反向遍历求编码
		for(cur = i, next = huf_tree[i].parent; next != 0; 
			cur = next, next = huf_tree[next].parent)  
			if(huf_tree[next].lchild == cur) 
				code_tmp[--index] = '0';	// 左‘0’
			else 
				code_tmp[--index] = '1';	// 右‘1’
		huf_tree[i].code = (char *)malloc((256-index)*sizeof(char));			// 为第i个字符编码动态分配存储空间 
		strcpy(huf_tree[i].code, &code_tmp[index]);     // 正向保存编码到树结点相应域中
	} 
	free(code_tmp);		// 释放编码临时空间
}

// 压缩函数
int huffman_compress(char *ifname, char *ofname)
{
	unsigned int i, j;
	unsigned int char_kinds;		// 字符种类
	unsigned char char_temp;		// 暂存8bits字符
	unsigned long file_len = 0;
	FILE *infile, *outfile;
	TmpNode node_temp;				//排序数据中转
	unsigned int node_num;
	HufTree huf_tree;
	char code_buf[256] = "\0";		// 待存编码缓冲区
	unsigned int code_len;
	/*
	** 动态分配256个结点，暂存字符频度，
	** 统计并拷贝到树结点后立即释放
	*/
	TmpNode *tmp_nodes =(TmpNode *)malloc(256*sizeof(TmpNode));  // 动态分配256个结点
	// 初始化暂存结点
	for(i = 0; i < 256; ++i){
		tmp_nodes[i].weight = 0;
		tmp_nodes[i].uch = (unsigned char)i;		// 数组的256个下标与256种字符对应
	}
	//遍历文件，获取字符频度
	infile = fopen(ifname, "rb");
	// 判断输入文件是否存在
	if (infile == NULL)
		return -1;
	fread((char *)&char_temp, sizeof(unsigned char), 1, infile);		// 从输入文件数据流读入一个字符暂存
	while(!feof(infile)){	//判断文件是否结束
		++tmp_nodes[char_temp].weight;		// 统计下标对应字符的权重，利用数组的随机访问快速统计字符频度
		++file_len;
		fread((char *)&char_temp, sizeof(unsigned char), 1, infile);		// 读入一个字符
	}
	fclose(infile);
	// 排序，将频度为零的放最后，剔除
	for(i = 0; i < 256-1; ++i)           
		for(j = i+1; j < 256; ++j)			//冒泡排序
			if(tmp_nodes[i].weight < tmp_nodes[j].weight){
				node_temp = tmp_nodes[i];
				tmp_nodes[i] = tmp_nodes[j];
				tmp_nodes[j] = node_temp;
			}
	// 统计实际的字符种类（出现次数不为0）
	for(i = 0; i < 256; ++i)
		if(tmp_nodes[i].weight == 0) 
			break;
	char_kinds = i;

	if (char_kinds == 1){
		outfile = fopen(ofname, "wb");					// 打开压缩后将生成的文件
		fwrite((char *)&char_kinds, sizeof(unsigned int), 1, outfile);		// 写入字符种类
		fwrite((char *)&tmp_nodes[0].uch, sizeof(unsigned char), 1, outfile);		// 写入唯一的字符
		fwrite((char *)&tmp_nodes[0].weight, sizeof(unsigned long), 1, outfile);		// 写入字符频度，也就是文件长度
		free(tmp_nodes);
		fclose(outfile);
	}
	else{
		node_num = 2 * char_kinds - 1;		// 根据字符种类数，计算建立哈夫曼树所需结点数 
		huf_tree = (HufNode *)malloc(node_num*sizeof(HufNode));		// 动态建立哈夫曼树所需结点     
		// 初始化前char_kinds个结点
		for(i = 0; i < char_kinds; ++i) { 
			// 将暂存结点的字符和频度拷贝到树结点
			huf_tree[i].uch = tmp_nodes[i].uch; 
			huf_tree[i].weight = tmp_nodes[i].weight;
			huf_tree[i].parent = 0; 
		}	
		free(tmp_nodes); // 释放字符频度统计的暂存区

		// 初始化后node_num-char_kins个结点
		for(; i < node_num; ++i) 
			huf_tree[i].parent = 0; 

		CreateTree(huf_tree, char_kinds, node_num);		// 创建哈夫曼树
		HufCode(huf_tree, char_kinds);		// 生成哈夫曼编码

		// 写入字符和相应权重，供解压时重建哈夫曼树
		outfile = fopen(ofname, "wb");					// 打开压缩后将生成的文件
		fwrite((char *)&char_kinds, sizeof(unsigned int), 1, outfile);		// 写入字符种类
		for(i = 0; i < char_kinds; ++i){
			fwrite((char *)&huf_tree[i].uch, sizeof(unsigned char), 1, outfile);			// 写入字符（已排序，读出后顺序不变）
			fwrite((char *)&huf_tree[i].weight, sizeof(unsigned long), 1, outfile);		// 写入字符对应权重
		}

		// 紧接着字符和权重信息后面写入文件长度和字符编码
		fwrite((char *)&file_len, sizeof(unsigned long), 1, outfile);		// 写入文件长度
		infile = fopen(ifname, "rb");		// 以二进制形式打开待压缩的文件
		fread((char *)&char_temp, sizeof(unsigned char), 1, infile);     // 每次读取8bits
		while(!feof(infile)){
			// 匹配字符对应编码
			for(i = 0; i < char_kinds; ++i)
				if(char_temp == huf_tree[i].uch)
					strcat(code_buf, huf_tree[i].code);

			// 以8位（一个字节长度）为处理单元
			while(strlen(code_buf) >= 8){
				char_temp = '\0';		// 清空字符暂存空间，改为暂存字符对应编码
				for(i = 0; i < 8; ++i)
				{
					char_temp <<= 1;		// 左移一位，为下一个bit腾出位置
					if(code_buf[i] == '1')
						char_temp |= 1;		// 当编码为"1"，通过或操作符将其添加到字节的最低位
				}
				fwrite((char *)&char_temp, sizeof(unsigned char), 1, outfile);		// 将字节对应编码存入文件
				strcpy(code_buf, code_buf+8);		// 编码缓存去除已处理的前八位
			}
			fread((char *)&char_temp, sizeof(unsigned char), 1, infile);     // 每次读取8bits
		}
		// 处理最后不足8bits编码
		code_len = strlen(code_buf);
		if(code_len > 0){
			char_temp = '\0';		
			for(i = 0; i < code_len; ++i){
				char_temp <<= 1;		
				if(code_buf[i] == '1')
					char_temp |= 1;
			}
			char_temp <<= 8-code_len;       // 将编码字段从尾部移到字节的高位
			fwrite((char *)&char_temp, sizeof(unsigned char), 1, outfile);       // 存入最后一个字节
		}

		// 关闭文件
		fclose(infile);
		fclose(outfile);

		// 释放内存
		for(i = 0; i < char_kinds; ++i)
			free(huf_tree[i].code);
		free(huf_tree);
	}
}//compress



// 解压函数
int huffman_extract(char *ifname, char *ofname)
{
	unsigned int i;
	unsigned long file_len;
	unsigned long writen_len = 0;		// 控制文件写入长度
	FILE *infile, *outfile;
	unsigned int char_kinds;		// 存储字符种类
	unsigned int node_num;
	HufTree huf_tree;
	unsigned char code_temp;		// 暂存8bits编码
	unsigned int root;		// 保存根节点索引，供匹配编码使用

	infile = fopen(ifname, "rb");		// 以二进制方式打开压缩文件
	// 判断输入文件是否存在
	if (infile == NULL)
		return -1;

	// 读取压缩文件前端的字符及对应编码，用于重建哈夫曼树
	fread((char *)&char_kinds, sizeof(unsigned int), 1, infile);     // 读取字符种类数
	if (char_kinds == 1){
		fread((char *)&code_temp, sizeof(unsigned char), 1, infile);     // 读取唯一的字符
		fread((char *)&file_len, sizeof(unsigned long), 1, infile);     // 读取文件长度
		outfile = fopen(ofname, "wb");					// 打开压缩后将生成的文件
		while (file_len--)
			fwrite((char *)&code_temp, sizeof(unsigned char), 1, outfile);	
		fclose(infile);
		fclose(outfile);
	}
	else{
		node_num = 2 * char_kinds - 1;		// 根据字符种类数，计算建立哈夫曼树所需结点数 
		huf_tree = (HufNode *)malloc(node_num*sizeof(HufNode));		// 动态分配哈夫曼树结点空间
		// 读取字符及对应权重，存入哈夫曼树节点
		for(i = 0; i < char_kinds; ++i){
			fread((char *)&huf_tree[i].uch, sizeof(unsigned char), 1, infile);		// 读入字符
			fread((char *)&huf_tree[i].weight, sizeof(unsigned long), 1, infile);	// 读入字符对应权重
			huf_tree[i].parent = 0;
		}
		// 初始化后node_num-char_kins个结点的parent
		for(; i < node_num; ++i) 
			huf_tree[i].parent = 0;
		CreateTree(huf_tree, char_kinds, node_num);		// 重建哈夫曼树（与压缩时的一致）
		// 读完字符和权重信息，紧接着读取文件长度和编码，进行解码
		fread((char *)&file_len, sizeof(unsigned long), 1, infile);	// 读入文件长度
		outfile = fopen(ofname, "wb");		// 打开压缩后将生成的文件
		root = node_num-1;
		while(1){
			fread((char *)&code_temp, sizeof(unsigned char), 1, infile);		// 读取一个字符长度的编码
			// 处理读取的一个字符长度的编码（通常为8位）
			for(i=0;i<8;++i)
			{
				// 由根向下直至叶节点正向匹配编码对应字符
				if(code_temp & 128)
					root = huf_tree[root].rchild;
				else
					root = huf_tree[root].lchild;
				if(root < char_kinds){
					fwrite((char *)&huf_tree[root].uch, sizeof(unsigned char), 1, outfile);
					++writen_len;
					if (writen_len == file_len) break;		// 控制文件长度，跳出内层循环
					root = node_num-1;        // 复位为根索引，匹配下一个字符
				}
				code_temp <<= 1;		// 将编码缓存的下一位移到最高位，供匹配
			}
			if (writen_len == file_len) break;		// 控制文件长度，跳出外层循环
		}

		// 关闭文件
		fclose(infile);
		fclose(outfile);

		// 释放内存
		free(huf_tree);
	}
}//extract()


int getFileSize(char * file)
{
    int size = -1;
    FILE * path;
    path = fopen(file, "rb");
    //path = fopen(file, "r");
    if (NULL == path){
        printf("File opening failed!\n");
        return  size; //文件大小不可能为负数
    }
    else{
        fseek(path, 0, SEEK_END);//设置流文件指针的位置,以SEEK_END为起点，偏移量是0,亦即SEEK_END
        size = ftell(path);//函数结果：当前文件流指针位置相对于文件起始位置的字节偏移量
        fclose(path);
        path = NULL;
    }

    return size;
}

int compareBMP(char *file1, char *file2){
	int n=0;
	unsigned char fd1;
	unsigned char fd2;
	fp1 = fopen(file1,"rb");
	fp2 = fopen(file2,"rb");
	while(1){
		fread(&fd1, sizeof(unsigned char), 1, fp1);
		fread(&fd2, sizeof(unsigned char), 1, fp2);
        	if(feof(fp1)||feof(fp2))break;
        	if (fd1 != fd2) {
                    n++;
                }
	}
	if (n > 0) {
        	printf("there are %d different pixels\n",n);
        	return ERROR_CUSTOM;
        }else printf("the two files are same\n");
        
}

void main(int argc, char *argv[]){
    int i;
    char *inFileName;
    char *outFileName;
    int n;
    int start,end,temp;
    FILE *filetemp;//用于创建文件
    int opt;
    clock_t time_start = 0;
    clock_t time_end = 0;
    double function_time = 0;
    block_width = 8;//默认8*8的块
    block_height = 8;
    int flag  = 0;		//初始化flag为0
    
#define USAGE   "USAGE: ./fblcd.out [--version] [-{en,de,cp} infile outfile]"

    if (strcmp(argv[1], "--version") == 0) {
        printf("%s\n",USAGE);
        printf("made by: ChuJiangheng \n");
        printf("Registration name: sansan \n");
        printf("Registration number: CICC1709 \n");
        printf("version: %s\n",APP_VERSION);
    }
    else if (strcmp(argv[1], "-en") == 0) {
        opt = 1;
    }
    else if (strcmp(argv[1], "-de") == 0) {
        opt = 2;
    }
    else if (strcmp(argv[1], "-cp") == 0) {
        opt = 3;
    }
    if (strcmp(argv[1], "--version") != 0 && argc < 4) {
        	printf("ERROR: parameter is not enough!");
        	opt = 0;
    }
    inFileName = argv[2];
    outFileName = argv[3];

    time_start = time(NULL);
	switch(opt){
            case 1:{
            	    //创建差值文件 
    		    filetemp = fopen("dif.dat","wb");
    		    fclose(filetemp); 
    		    
                    printf("Compressing...\n");
                    //去除bmp文件数据头
                    bmp_dataread(inFileName);//输出为bmp.dat
                    //线性数据块预测 
                    for(i=0;i<(width*height)/(block_width*block_height);i++)
                        LOCO_I_encode("bmp.dat", width, height, block_width, block_height, i);//输出为dif.dat
                    //全文件游程压缩
                    code_length_compress("dif.dat","loco_length.dat");//输出为loco_length.dat
                    //全文件Huffman压缩
                    flag = huffman_compress("loco_length.dat", "loco_length_huf.dat");//输出为loco_length_huf.dat
                    //添加压缩文件数据头
                    enfile_headwrite(outFileName,width,height,block_width,block_height);
                    }break;
            case 2:{
                    //创建解压文件
    		    filetemp = fopen("recov.dat", "wb");
    		    fclose(filetemp);  
    		    
                    printf("Decompressing...\n"); 
                    //去除压缩文件数据头
                    enfile_dataread(inFileName);
                    //全文件Huffman解压
                    flag = huffman_extract("loco_length_huf.dat", "loco_length.dat");	
                    //全文件游程解压
                    code_length_decompress("loco_length.dat","dif.dat");
                    //线性数据块恢复
                    for(i=0;i<(width*height)/(block_width*block_height);i++)
                        LOCO_I_decode("dif.dat", width, height, block_width, block_height, i);
                    bmp_headwrite(outFileName,width,height);
                    }break;	
            case 3:{
            	    compareBMP(inFileName,outFileName);
                   }break;		
            case 4:{

                   }break;
	
		}
        time_end = time(NULL);
        //移除中转文件
        remove("bmp.dat");
        remove("dif.dat");
        remove("loco_length.dat");
        remove("loco_length_huf.dat");
        remove("recov.dat");
		if (flag == -1)
			printf("Sorry, infile \"\" doesn't exist!\n");		// 如果标志为‘-1’则输入文件不存在
		else
			printf("\nExecution time:%.1fs\n",1E3*(time_end-time_start)/LOOP);		// 操作完成
		if(opt == 1)
			printf ("compressibility:%2.2lf%%\n",1E2*(double)getFileSize(outFileName) / (double)getFileSize(inFileName));
		printf("\n");
	
    exit(0);
}


