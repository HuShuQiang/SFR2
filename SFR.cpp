#pragma once
#pragma warning(disable:4244)
#pragma warning(disable:4305)
#pragma warning(disable:4267)
#include<opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>
#include<iostream>
#include<cmath>
#include<cstring>
#include<vector>
#include<direct.h>
#include<io.h>
#include"cJSON.h"
#include<Windows.h>
#include <ctime>
using namespace cv;
using namespace std;
#define MINROTATE 0.017
#define M_PI 3.141592654
#define POSNUM 5

string moduleCode = "";
char* Result;

const int BlockArealimitlower = 6500;
const int BlockArealimitup = 15000;
const int sfrRoishortside = 50;
const int sfrRoilongside = 60;
int SaveDemosaicBMP = 1;
int SaveDebugResult = 1;
int Width = 2560;
int Height = 1440;
float SfrAtNyquist = 0.25;
float Field = 0.7;

typedef struct STCTROIHV
{
	Rect2f R2f_H[POSNUM];
	Rect2f R2f_V[POSNUM];
}StctRoiHV;

typedef struct STCTSFRSCORE
{
	float MTF_H[POSNUM];
	float MTF_V[POSNUM];
}StctSfrScore;


string errorCode("");
void recordError(string str)
{
	errorCode = str;
}

typedef struct STCTRESULT
{
	Point positionH;
	Point positionV;
	float sfrvalueH;
	float sfrvalueV;
	vector<float> sfrcurveH;
	vector<float> sfrcurveV;
}StctResult;

StctResult sresult[POSNUM];

string getTime()
{
	time_t t = time(NULL);   //获取当前时间句柄
	tm* pt = localtime(&t);  //将时间句柄转换为tm时间结构体
	int year = pt->tm_year + 1900;
	int month = pt->tm_mon + 1;
	int day = pt->tm_mday;
	int hour = pt->tm_hour;
	int minute = pt->tm_min;
	int second = pt->tm_sec;
	char str[128];
	sprintf_s(str, "%04d-%02d-%02d-%02d:%02d:%02d", year, month, day, hour, minute, second);
	return str;
}
void writeLog(string str)
{
	char filename[1024];
	sprintf(filename, "%s//result.txt",moduleCode.c_str());
	/*fstream fw(filename, ios::out);
	if (fw.bad())
	{
		fw.close();
		return;
	}
	fw << str << endl;
	fw.close();*/
}


//读取INT FLOAT参数
int GetCfgParamInt(LPCSTR lpKeyName, INT nDefault)
{
	char buffer[256];
	_getcwd(buffer, 256);
	string path = string(buffer) + "\\config.txt";
	return GetPrivateProfileIntA("MAIN", lpKeyName, nDefault, path.c_str());
}
float GetCfgParamFloat(LPCSTR lpKeyName, float fDefault)
{
	char bufTmp[64] = { 0 };
	char bufTmp1[64] = { 0 }; sprintf_s(bufTmp1, "%f", fDefault);
	char buffer[256];
	_getcwd(buffer, 256);
	string path = string(buffer) + "\\config.txt";
	GetPrivateProfileStringA("MAIN", lpKeyName, bufTmp1, bufTmp, 64, path.c_str());
	return atof(bufTmp);
}

void recordJsonData()
{
	cJSON* array = cJSON_CreateArray();
	if (!array)
		return ;
	for (int i = 0; i < POSNUM; i++)
	{
		cJSON* temp = cJSON_CreateObject();
		if (!temp)
			return ;
		//加入节点(键值对)
		cJSON_AddStringToObject(temp, "errorinfo", errorCode.c_str());

		char tempstr[1024];
		memset(tempstr, 0, 1024);
		sprintf(tempstr, "%.2f", GetCfgParamFloat("SfrAtNyquist", 0));
		cJSON_AddStringToObject(temp, "frequence", tempstr);

		memset(tempstr, 0, 1024);
		sprintf(tempstr, "(%d,%d)", sresult[i].positionH.x, sresult[i].positionH.y);
		cJSON_AddStringToObject(temp, "positionH", tempstr);

		memset(tempstr, 0, 1024);
		sprintf(tempstr, "(%d,%d)", sresult[i].positionV.x, sresult[i].positionV.y);
		cJSON_AddStringToObject(temp, "positionV", tempstr);

		memset(tempstr, 0, 1024);
		sprintf(tempstr, "%.5f", sresult[i].sfrvalueH);
		cJSON_AddStringToObject(temp, "sfrvalueH", tempstr);

		memset(tempstr, 0, 1024);
		sprintf(tempstr, "%.5f", sresult[i].sfrvalueV);
		cJSON_AddStringToObject(temp, "sfrvalueV", tempstr);

		string sfrstrH("");
		for (int k = 0; k < sresult[i].sfrcurveH.size(); k++)
		{
			memset(tempstr, 0, 1024);
			sprintf(tempstr, "%.5f", sresult[i].sfrcurveH[k]);
			sfrstrH += tempstr;
		}
		cJSON_AddStringToObject(temp, "sfrcurveH", sfrstrH.c_str());

		string sfrstrV("");
		for (int k = 0; k < sresult[i].sfrcurveV.size(); k++)
		{
			memset(tempstr, 0, 1024);
			sprintf(tempstr, "%.5f", sresult[i].sfrcurveV[k]);
			sfrstrV += tempstr;
		}
		cJSON_AddStringToObject(temp, "sfrcurveV", sfrstrV.c_str());

		cJSON_AddItemToArray(array, temp);
	}

	cJSON* root = cJSON_CreateObject();
	if (!root)
		return ;
	cJSON_AddItemToObject(root, "SFRRESULT", array);
	char* strJson = cJSON_PrintUnformatted(root);
	Result = strJson;
	writeLog(strJson);
	if (strJson)
		free(strJson);
	cJSON_Delete(root);
}





int RAW10toYUV(string rawdatapath, Mat& yuvMat)
{
	//int rawW = GetCfgParamInt("Width", 0);
	//int rawH = GetCfgParamInt("Height", 0);
	int rawW = Width;
	int rawH = Height;
	FILE* fp0;
	fopen_s(&fp0, rawdatapath.c_str(), "rb");
	if (fp0==nullptr)
	{
		recordError("Read the raw file failed");
		return -1;
	}
	unsigned char* tmp = new unsigned char[rawW * rawH * 2];
	Mat raw8(rawH, rawW, CV_8UC1);
	if (!tmp)
	{
		fclose(fp0);
		recordError("Malloc for  raw file failed");
		return -2;
	}
	for (int i = 0; i < rawH; i++)
	{
		int k = 0;
		fread(tmp, rawW * 2, 1, fp0);
		unsigned short* tmp1 = (unsigned short*)tmp;
		for (int j = 0; j < rawW; j++)
		{
			raw8.data[i * rawW + j] = tmp1[j] >> 2;
		}
	}
	fclose(fp0);
	if (tmp)
		free(tmp);
	Mat bgr, YUV;
	cvtColor(raw8, bgr, COLOR_BayerBG2BGR);
	//int bSaveDemosaicBMP = GetCfgParamInt("bSaveDemosaicBMP",0);
	if (SaveDemosaicBMP)
	{
		string bmpname = moduleCode+"//" + moduleCode + ".bmp";
		imwrite(bmpname, bgr);
	}
	cvtColor(bgr, YUV, COLOR_BGR2YUV_YV12);
	YUV(Rect(0, 0, rawW, rawH)).copyTo(yuvMat);
	return 0;
}

//SFR-1定位roi矩心位置
int Cal_locate_centroids(vector<double> &farea, vector<double> &temp, vector<double> &shifts, unsigned short size_x, unsigned short size_y, double *offset)
{
	double dt(0.0), dt1(0.0), dt2(0.0);

	for (int j = 0; j < size_y; j++)
	{
		dt = 0.0;
		dt1 = 0.0;
		for (int i = 0; i < size_x - 1; i++)
		{
			dt2 = farea[j*size_x + (i + 1)] - farea[j*size_x + i];
			dt += dt2 * (double)i;
			dt1 += dt2;
		}
		shifts[j] = dt / dt1;//每行的矩心所在列（矩心计算公式）
	}

	//这个地方检测矩心不能太靠近边缘，太靠近边缘的时候会影响数据的准确性，最小设置为6，为了更安全可以设置为更大
	if (min(shifts[size_y - 1], size_x - shifts[size_y - 1]) < 6 || min(shifts[0], size_x - shifts[0]) < 6)
	{
		recordError("Sfr Block ROI problem");
		return -1;
	}
		


	int cr = size_y / 2;//中心行
	dt = shifts[cr];//中心行矩心所在列
	for (int i = 0; i < size_y; i++) {
		temp[i] = (double)i - (double)cr;//所有行与中心行的间距
		shifts[i] -= dt;//每行的矩心所在列与中心行矩心所在列的间距
						//cout << shifts[i] << endl;
	}
	*offset = dt;//中心行矩心所在列
	return 0;
}

//SFR-2矩心直线拟合
int FitLine(int ndata, vector<double> &x, vector<double> &y, double *b, double *a)
{
	double t(0.0), sxoss(0.0), syoss(0.0), sx(0.0), sy(0.0), st2(0.0);
	*b = 0.0;
	for (int i = 0; i < ndata; i++)
	{
		sx += x[i];
		sy += y[i];
	}
	double ss = (double)ndata;
	sxoss = sx / ss;
	syoss = sy / ss;
	for (int i = 0; i < ndata; i++)
	{
		t = x[i] - sxoss;
		st2 += t*t;
		*b += t * y[i];
	}
	*b /= st2;         /* slope  斜率*/
	*a = (sy - sx*(*b)) / ss; /* intercept  截距*/

	if (abs(*b) > 0.57 || abs(*b) < 0.01) //不允许超过实际倾斜角度，仅仅是个附加管控项，具体可以根据实际拍摄的图片来定spec.
	{
		/*cout << abs(*b) << endl;*/
		recordError("ROI rotation too large");
		return -2;
	}
		


	//检查拟合的直线上的点和未拟合直线前的边缘点误差(残差平方和-拟合优度R2)，误差太大则认为边缘有问题，无法保证计算有效性
	double chi2(0.0), sst(0.0);
	for (int i = 0; i < ndata; i++)
	{
		chi2 += pow((y[i] - (*a) - (*b) * x[i]), 2);
		sst += pow((y[i] - syoss), 2);
	}
	double R2 = 1.0 - chi2 / sst;//太大说明边缘参差太大，计算会有误差
	float minRotateSpec_R2 = 0;
	if (R2 < 0.3)
	{
		recordError("lineration FitLine error too large");
		return -1;
	}
		
	return 0;
}

//SFR-3定位lsf最大值的列标
void Cal_Locate_PSF_max(int len, vector<double> &AveEdge, int &pcnt2)
{

	double dt(0.0), dt_new(0.0);
	int left(-1), right(-1);

	//寻找最大值
	for (int i = 0; i < len; i++)
	{
		dt_new = fabs(AveEdge[i]);
		if (dt_new > dt)
		{
			pcnt2 = i;
			dt = dt_new;
		}
	}
	//寻找最大值的左右列标，保证最大值列标是在所有最大值列得中间
	for (int i = 0; i < len; i++)
	{
		dt_new = fabs(AveEdge[i]);
		if (dt_new == dt)
		{
			if (left < 0) left = i;
			right = i;
		}
	}
	/* find centre of maxima */
	pcnt2 = (right + left) / 2;
}

//SFR-4汉明窗可以抑制高频处的震荡增加平滑程度，其实就是一个低通滤波器
void Apply_hamingWindow_denoise(int oldlen, unsigned short newxwidth, vector<double> &AveEdge, int pcnt2)
{
	long edge_offset;

	//将LSF的最大值移到中心位置， 两边由于移动造成的空位由0补齐
	edge_offset = pcnt2 - (oldlen / 2);
	if (edge_offset < 0)
	{
		for (int i = oldlen - 1; i > -edge_offset - 1; i--)
			AveEdge[i] = (AveEdge[i + edge_offset]);
		for (int i = 0; i < -edge_offset; i++)
			AveEdge[i] = 0.00; /* last operation */
	}
	else if (edge_offset > 0)//左移
	{
		for (int i = 0; i < oldlen - edge_offset; i++)
			AveEdge[i] = (AveEdge[i + edge_offset]);
		for (int i = oldlen - edge_offset; i < oldlen; i++)
			AveEdge[i] = 0.00;
	}

	// 给begin和end之间的数据加上汉明窗
	// 汉明窗 W(n,α ) = (1 -α ) - α cos(2*PI*n/(N-1)) ，（0≤n≤N-1）
	// 一般情况下，α取0.46
	double hanming(0.0);
	for (int i = 0, j = -oldlen / 2; i < oldlen; i++, j++)
	{
		hanming = 0.54 + 0.46*cos((M_PI*(double)j) / (newxwidth * 4 / 2));
		AveEdge[i] = (AveEdge[i])*hanming;
	}

}

//SFR-5This is the DFT magnitude code
void DTF(long bin_len, double dx, vector<double> &lsf, long ns, double ds, vector<double> &sfr)
{
	double a(0.0), b(0.0), twopi(2.0 * M_PI), g(0.0);
	int i(0), j(0);
	for (j = 0; j < ns; j++) {
		g = twopi * dx * ds * (double)j;
		for (i = 0, a = 0, b = 0; i < bin_len; i++) {
			a += lsf[i] * cos(g * (double)(i));
			b += lsf[i] * sin(g * (double)(i));
		}
		sfr[j] = sqrt(a * a + b * b);
	}
}

//SFR-6超采样得到ESF
int Oversample4x(int bin_len, vector<double> &edgex, vector<double> &Signal, vector<double> &AveEdge, vector<int> &counts, int size_x, int size_y)
{
	int datalen(size_x*size_y);
	int bin_number;
	for (int i = 0; i < datalen; i++)//关键步骤1.对之前坐标转换之后的数据,利用4*float类型坐标进行采样，对相同坐标的数据取平均赋值给四倍采样数组
	{
		bin_number = (int)floor((double)4 * edgex[i]);

		if (bin_number >= 0)
		{
			if (bin_number <= (bin_len - 1))
			{
				AveEdge[bin_number] = AveEdge[bin_number] + Signal[i];
				counts[bin_number] = (counts[bin_number]) + 1;
			}
		}
	}
	int nzeros = 0;
	for (int i = 0; i < bin_len; i++)
	{
		int j(0);
		int k(1);
		if (counts[i] == 0) //防止统计数据某个坐标下统计数目为0的情况，选择向前查找替换的方式填充
		{
			nzeros++;
			if (i == 0)
			{
				while (!j)
				{
					if (counts[i + k] != 0)
					{
						AveEdge[i] = AveEdge[i + k] / ((double)counts[i + k]);
						j = 1;
					}
					else k++;
				}
			}
			else
			{
				while (!j && ((i - k) >= 0))
				{
					if (counts[i - k] != 0)
					{
						AveEdge[i] = AveEdge[i - k];
						j = 1;
					}
					else k++;
				}
				if ((i - k) < 0)
				{
					k = 1;
					while (!j)
					{
						if (counts[i + k] != 0)
						{
							AveEdge[i] = AveEdge[i + k] / ((double)counts[i + k]);
							j = 1;
						}
						else k++;
					}
				}
			}
		}
		else
			AveEdge[i] = (AveEdge[i]) / ((double)counts[i]);//关键步骤2.对步骤1中统计的相同坐标的数据取平均赋值给四倍采样数组
	}
#ifdef DEBUG__
	cout << "bin_len=" << bin_len << ", " << "nzeros=" << nzeros << ";if nzeros are too big,maybe inaccurate" << endl;
#endif
	if (nzeros > (bin_len*0.25))//不得超过总统计的1/4
	{
		recordError("zero points too much");
		return -1;
	}
	return 0;
}

//SFR-7计算得到LSF
void Cal_derivative(int len, vector<double> &AveTmp, vector<double> &AveEdge, double &centroid)
{
	double dt(0.0), dt1(0.0);

	for (int i = 0; i < len; i++)
		AveTmp[i] = AveEdge[i];


	for (int i = 1; i < len; i++)
	{
		AveEdge[i] = (AveTmp[i] - AveTmp[i - 1]);
		dt += AveEdge[i] * (double)i;
		dt1 += AveEdge[i];

	}
	centroid = dt / dt1;//矩心计算
	AveEdge[0] = AveEdge[1];
}

//SFR-8计算得到SFR
int calculateSFR(Mat ROI, float *mtfValue, vector<float> & mtfcurve)
{

	int recode = 0;
	//float nyquist = GetCfgParamFloat("SfrAtNyquist", 0);
	float nyquist = SfrAtNyquist;
	//第1步：Mat降维一维数组化
	vector<double> farea;
	for (int i = 0; i < ROI.rows; i++)
		for (int j = 0; j < ROI.cols; j++)
			farea.push_back(ROI.at<uchar>(i, j));


	//第2步：计算矩心
	/* Allocate memory */
	int size_x(ROI.cols);
	int size_y(ROI.rows);
	vector<double> shifts(size_y);//每行的矩心所在列与中心行矩心所在列的间距
	vector<double> temp(size_y);  //所有行与中心行的间距
	vector<double> edgex(size_y*size_x);
	vector<double> Signal(size_y*size_x);
	double offset1(0.0);

	recode = Cal_locate_centroids(farea, temp, shifts, size_x, size_y, &offset1);
	if (recode != 0)
		return recode;


	//第3步：对矩心点线性回归拟合，确定边缘准确位置,斜率和截距
	double slope(0.0);
	double offset2(0.0);
	vector<double> shiftSmooth;
	recode = FitLine(size_y, temp, shifts, &slope, &offset2);
	if (recode != 0)
		return recode;



	//第4部：计算每个点到边缘距离（相对距离），确定新坐标信息
	double offset = offset1 + 0.5 + offset2 - size_x / 2;//主要对计算误差补偿
	int centerRow = size_y / 2;
	//cout << size_y << endl;
	for (int i = 0; i < size_y; i++)
	{
		shifts[i] = slope * (double)(i - centerRow) + offset;
		//cout << offset<<" "<<shifts[i] << endl;
	}
	int pcnt(0);
	for (int j = 0; j < size_y; j++)
	{
		for (int i = 0; i < size_x; i++)
		{
			edgex[pcnt] = (double)i - shifts[j];
			//cout << edgex[pcnt] << " ";
			pcnt++;
		}
		//cout << endl;
	}


	//第5步：四部超采样获取esf数据
	int bin_len(4 * size_x);//4倍超采样
	//cout << bin_len << endl;
	vector<double> AveEdge(bin_len);
	vector<double> AveTmp(bin_len);
	vector<int> counts(bin_len);
	for (int i = 0; i < bin_len; i++)
	{
		AveEdge[i] = 0;
		counts[i] = 0;
		counts[i] = 0;
	}
	recode = Oversample4x(bin_len, edgex, farea, AveEdge, counts, size_x, size_y);
	if (recode != 0)
		return recode;

	double centroid(0.0);
	Cal_derivative(bin_len, AveTmp, AveEdge, centroid);

	//计算得到LSF中心
	Cal_Locate_PSF_max(bin_len, AveEdge, pcnt);


	//应用汉明窗减少两边波动,应用汉明窗降噪，此段理论上无噪声才对，但因为sensor的动态热噪声，需要屏蔽掉。
	Apply_hamingWindow_denoise(bin_len, size_x, AveEdge, pcnt);

	/* From now on this is the length used. */
	int len(bin_len / 2);

	double tmp(1.0);
	double tmp2(1.0 / ((double)bin_len));
	(void)DTF(bin_len, tmp, AveEdge, len, tmp2, AveTmp);

	int sampleFreq1x(len / 4);//1倍奈奎斯特频率
	if (nyquist >= 1 || nyquist <= 0)
	{
		recordError("nyquist spec error");
		return -1;
	}
	int starta = sampleFreq1x*nyquist;
	int endb = starta + 1;
	*mtfValue = ((double)((double)(AveTmp[endb] / AveTmp[0])-(double)AveTmp[starta] / AveTmp[0]))*((float)sampleFreq1x*nyquist - starta) + (double)(AveTmp[starta] / AveTmp[0]);
	//cout << *mtfValue << endl;
	for (int i = 0; i < sampleFreq1x; i++)
		mtfcurve.push_back((float)(AveTmp[i] / AveTmp[0]));
#ifdef SHOWRESULT_
	cout << "{ ";
	for (int i = 0; i < sampleFreq2x; i++)
	{
		/*(*freq)[i]= ((double)i/(double)size_x);
		(*sfr)[i] = *//*(double) (AveTmp[i]/AveTmp[0]); */
		std::cout << (double)(AveTmp[i] / AveTmp[0]) << ", ";
	}
	cout << " }" << endl;
#endif
	return 0;
}



int DetectROI(Mat src_0, StctRoiHV& ostctroihv)
{
	//float field2 = GetCfgParamFloat("field2", 0);
	//float field1 = GetCfgParamFloat("field1", 0);


	int W = src_0.cols;
	int H = src_0.rows;
	Mat src;
	src_0.copyTo(src);
	if (src.channels() == 3)
		cvtColor(src, src, COLOR_BGR2GRAY);
	medianBlur(src, src, 3);
	Mat src1;
	adaptiveThreshold(src, src1, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 125, 3);
	erode(src1, src1, getStructuringElement(MORPH_RECT, Size(5, 5)));
	if (SaveDebugResult)//调试使用
	{
		string pathroijpg = moduleCode + "//threshold.jpg";
		imwrite(pathroijpg, src1);
	}
	//统计所有正常的黑块
	vector<vector<Point>> contours; vector<cv::Vec4i>hierarchy;
	cv::findContours(src1, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, cv::Point(0, 0));
	vector<RotatedRect> vrr;
	vector<vector<Point>> contours_4p(contours.size());//用于存放矩形四个角点集
	vector<vector<Point>> contours_4p_filtered;//存放满足条件的正常黑块
	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_4p[i], 15, true);
		float areai = contourArea(contours[i]); 
		if (4 == contours_4p[i].size() && (areai > BlockArealimitlower && areai < BlockArealimitup))
		{
			if (contours_4p[i][0].x > contours_4p[i][2].x)//保证第0个角点在黑块的左上角
			{
				Point tmpp = contours_4p[i][0];
				contours_4p[i][0] = contours_4p[i][1];
				contours_4p[i][1] = contours_4p[i][2];
				contours_4p[i][2] = contours_4p[i][3];
				contours_4p[i][3] = tmpp;
			}
			contours_4p_filtered.push_back(contours_4p[i]);
		}
	}
	//if (field2 > 1 || field2 < 0 || field1 > field2 || field1<0)
	//{
	//	recordError("请检查filed参数，要求范围0~1,且field1≤field2");
	//	return -1;
	//}

	if (Field > 1 || Field<0)
	{
		recordError("请检查filed参数，要求范围0~1");
		return -1;
	}

	//将所有的黑块分解为HV方向斜边,并过滤掉小角度的斜边(因为小角度斜边计算sfr不准)。
	vector<Point2f> p2fH, p2fV;
	for (int i = 0; i < contours_4p_filtered.size(); i++)
	{
		for (int k = 0; k < 4; k++)
		{
			Point2f delt_xy = contours_4p_filtered[i][k] - contours_4p_filtered[i][(k+1)%4];
			if ((abs(delt_xy.x) > abs(delt_xy.y)) && (abs(delt_xy.y) / abs(delt_xy.x) > MINROTATE/*最低倾斜1°*/))
				p2fV.push_back((contours_4p_filtered[i][k] + contours_4p_filtered[i][(k + 1) % 4]) / 2);
			else if ((abs(delt_xy.x) < abs(delt_xy.y)) && (abs(delt_xy.x) / abs(delt_xy.y) > MINROTATE/*最低倾斜1°*/))
				p2fH.push_back((contours_4p_filtered[i][k] + contours_4p_filtered[i][(k + 1) % 4]) / 2);
		}
	}
	






	/*筛选出离着视场点最近的斜边*/
	Point2f filedPnt1[POSNUM] = { Point2f(W * 0.5 * (1 - Field), H * 0.5 * (1 - Field)),
							Point2f(W * 0.5 * (1 + Field), H * 0.5 * (1 - Field)),
							Point2f(W * 0.5 * (1 + Field), H * 0.5 * (1 + Field)),
							Point2f(W * 0.5 * (1 - Field), H * 0.5 * (1 + Field)),
							Point2f(W * 0.5 , H * 0.5) }; //数组下标表示的意思：field左上，field右上，field右下，field左下，中心
	float distanceTmpH[POSNUM] = { 655535,655535, 655535, 655535, 655535 };
	float distanceTmpV[POSNUM] = { 655535,655535, 655535, 655535, 655535 };
	Point2f posH[POSNUM];
	Point2f posV[POSNUM];
	for (int i = 0; i < p2fH.size(); i++)
	{
		for (int k = 0; k < POSNUM; k++)
		{
			float distemp = pow(pow(p2fH[i].x - filedPnt1[k].x, 2) + pow(p2fH[i].y - filedPnt1[k].y, 2), 0.5);
			if (distemp < distanceTmpH[k])
			{
				distanceTmpH[k] = distemp;
				posH[k] = p2fH[i];
			}
			
		}
	}
	for (int i = 0; i < p2fV.size(); i++)
	{
		for (int k = 0; k < POSNUM; k++)
		{
			float distemp = pow(pow(p2fV[i].x - filedPnt1[k].x, 2) + pow(p2fV[i].y - filedPnt1[k].y, 2), 0.5);
			if (distemp < distanceTmpV[k])
			{
				distanceTmpV[k] = distemp;
				posV[k] = p2fV[i];
			}

		}
	}
	StctRoiHV stctroihv;//存放需要计算的ROI
	for (int i = 0; i < POSNUM; i++)
	{
     	stctroihv.R2f_H[i] = Rect2f(posH[i].x - sfrRoishortside / 2, posH[i].y - sfrRoilongside / 2,  sfrRoishortside, sfrRoilongside);
		stctroihv.R2f_V[i] = Rect2f(posV[i].x - sfrRoilongside  / 2, posV[i].y - sfrRoishortside / 2, sfrRoilongside,  sfrRoishortside);
		
	}
	ostctroihv = stctroihv;
	if (SaveDebugResult)
	{
		Mat matROIpos;
		src_0.copyTo(matROIpos);
		for (int i = 0; i < POSNUM; i++)
		{

			rectangle(matROIpos, stctroihv.R2f_H[i], 200, 2);
			rectangle(matROIpos, stctroihv.R2f_V[i], 200, 2);
		}
		string pathroijpg = moduleCode + "//ROI.jpg";
		imwrite(pathroijpg, matROIpos);
	}

	return 0;
}

int DetectAndCalculate(Mat src, StctRoiHV& ostctroihv, StctSfrScore& ostctsfrhv)
{
	int re = 0;
	float mtfValueH(0), mtfValueV(0);
	for (int i = 0; i < POSNUM; i++)
	{
		Mat roiH(src(ostctroihv.R2f_H[i])), roiV(src(ostctroihv.R2f_V[i]));
		vector<float> mtfcurveH, mtfcurveV;
		re = calculateSFR(roiH,  &mtfValueH, mtfcurveH);
		Mat roiV1 = Mat(roiV.rows, roiV.cols, roiV.depth());
		transpose(roiV, roiV1);
		flip(roiV1, roiV1, 0);
		re = calculateSFR(roiV1,  &mtfValueV, mtfcurveV);
		if (re)
			return re;

		sresult[i].positionH = (ostctroihv.R2f_H[i].br()+ ostctroihv.R2f_H[i].tl())/2;
		sresult[i].positionV = (ostctroihv.R2f_V[i].br() + ostctroihv.R2f_V[i].tl()) / 2;
		sresult[i].sfrvalueH = mtfValueH;
		sresult[i].sfrvalueV = mtfValueV;
		sresult[i].sfrcurveH = mtfcurveH;
		sresult[i].sfrcurveV = mtfcurveV;
		ostctsfrhv.MTF_H[i] = mtfValueH;
		ostctsfrhv.MTF_V[i] = mtfValueV;
		cout << mtfValueH << " " << mtfValueV << endl;
		
	}


	//int bSaveDebugResult = GetCfgParamInt("bSaveDebugResult", 0);
	if (SaveDebugResult)
	{
		Mat sfrresult;
		src.copyTo(sfrresult);
		for (int i = 0; i < POSNUM; i++)
		{
			rectangle(sfrresult, ostctroihv.R2f_H[i], 255, 2);
			rectangle(sfrresult, ostctroihv.R2f_V[i], 255, 2);
			char a[64];
			sprintf_s(a, "%.2f", sresult[i].sfrvalueH);
			putText(sfrresult, a, sresult[i].positionH, FONT_HERSHEY_SIMPLEX, 0.5, 255);
			char b[64];
			sprintf_s(b, "%.2f", sresult[i].sfrvalueV);
			putText(sfrresult, b, sresult[i].positionV, FONT_HERSHEY_SIMPLEX, 0.5, 255);
		}
		string pathroijpg = moduleCode + "//sfrResult.jpg";
		imwrite(pathroijpg, sfrresult);
	}


	return 0;
}


void mkdir_(string pathname)
{
	if (_access(pathname.c_str(), 0) == -1)
		int re=_mkdir(pathname.c_str());
}
int main(int argc, char* argv[])
{


	//if (argc != 3)
	//{
	//	cout << "参数无效；使用方法举例(xxx.exe 12345678 demo.raw)，其中12345678代表模组编号" << endl;
	//	return -1;
	//}
	//moduleCode = argv[1];
	//string rawpath(argv[2]);
	moduleCode = "04";
	//输入16bit格子图RAW文件--HSQ
	string rawpath = "D:\\桌面\\数据文件夹\\10台样本\\1\\RAW_ffffffff_2560X1440_16bit__2023_03_23,10,31,50.raw";
	

	int re = 0;
	mkdir_(moduleCode);//建立一个以modulecode为名字的文件夹，用来存放与此模组相关的所有测试输出文件或结果

	Mat yuvMat;
	StctRoiHV ostctroihv;
	StctSfrScore stctsfrhv;

	re = RAW10toYUV(rawpath, yuvMat);
	if (yuvMat.empty() || re)
		goto END;



	re = DetectROI(yuvMat, ostctroihv);
	if (re)
		goto END;

	re = DetectAndCalculate(yuvMat, ostctroihv, stctsfrhv);
	if (re)
		goto END;

END:
	recordJsonData();
	return re;
}

void Sfrconfig(int width,int height,float field,float Nyquist,int SaveBMP = 0,int SaveDebug = 0)
{
	SaveDemosaicBMP = SaveBMP;
	SaveDebugResult = SaveDebug;
	Width = width;
	Height = height;
	SfrAtNyquist = Nyquist;
	Field = field;
}

int Sfrcalculate(char* ImagePath, char* ResultJson) 
{
	moduleCode = "01";
	string rawpath = ImagePath;
	int re = 0;
	mkdir_(moduleCode);//建立一个以modulecode为名字的文件夹，用来存放与此模组相关的所有测试输出文件或结果

	Mat yuvMat;
	StctRoiHV ostctroihv;
	StctSfrScore stctsfrhv;

	re = RAW10toYUV(rawpath, yuvMat);
	if (yuvMat.empty() || re)
		goto END;

	re = DetectROI(yuvMat, ostctroihv);
	if (re)
		goto END;

	re = DetectAndCalculate(yuvMat, ostctroihv, stctsfrhv);
	if (re)
		goto END;

	END:
		recordJsonData();
		ResultJson = Result;
		return re;
}