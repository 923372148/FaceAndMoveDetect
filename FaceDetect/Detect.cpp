//�������������ô�����ͷ��⵽������ͼƬ���Ƚ���ֱ��ͼ���⻯
//�����ŵ�92 * 112��ͼƬ��С��Ȼ�����train.txt�Ĳɼ���������ģ��
//����ƥ��ʶ���������ͳһ�����£��ɼ���ͬ�Ƕȵ�����ͼƬ��һ�ţ�
//ע�⣺Ӱ��ļ����������ڹ��գ�ģ������ɼ���ͼ����ղ�һ����ʶ���ʺܵ͡�
//�����ԣ�ģ���������ͼ����ͬһ�����µĻ�����������������������ʶ����ʶ���ʽϸ�

#include <stdio.h>  
#include <string.h>  
#include "cv.h"  
#include "cvaux.h"  
#include "highgui.h"  
#include <stdlib.h>  
#include <assert.h>    
#include <math.h>    
#include <float.h>    
#include <limits.h>    
#include <time.h>    
#include <ctype.h>    

////���弸����Ҫ��ȫ�ֱ���  
IplImage ** faceImgArr = 0; // ָ��ѵ�������Ͳ���������ָ�루��ѧϰ��ʶ��׶�ָ��ͬ��  
CvMat    *  personNumTruthMat = 0; // ����ͼ���ID��  
int nTrainFaces = 0; // ѵ��ͼ�����Ŀ  
int nEigens = 0; // �Լ�ȡ����Ҫ����ֵ��Ŀ  
IplImage * pAvgTrainImg = 0; // ѵ���������ݵ�ƽ��ֵ  
IplImage ** eigenVectArr = 0; // ͶӰ����Ҳ������������  
CvMat * eigenValMat = 0; // ����ֵ  
CvMat * projectedTrainFaceMat = 0; // ѵ��ͼ���ͶӰ  
char *filename[5] = { "face1.jpg","face2.jpg","face3.jpg","face4.jpg","face5.jpg" };

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
int j = 0;//ͳ�Ƽ�¼��������  
char a[512] = { 0 };
int a1, a2, a3, a4;
time_t timeBegin, timeEnd;
int timeuse;
//// ����ԭ��  
void learn();
void doPCA();
void storeTrainingData();
int  loadTrainingData(CvMat ** pTrainPersonNumMat);
int  findNearestNeighbor(float * projectedTestFace);
int  loadFaceImgArray(char * filename);
void printUsage();
int detect_and_draw(IplImage* image);
int recognize(IplImage *faceimg);

//����������Ҫ����ѧϰ��ʶ�������׶Σ���Ҫ�������Σ�ͨ�������д���Ĳ�������  
int main(int argc, char** argv)
{
	CvCapture* capture = 0;
	IplImage *frame, *frame_copy = 0;
	int optlen = strlen("--cascade=");
	char *cascade_name = "haarcascade_frontalface_alt2.xml";
	//opencvװ�ú�haarcascade_frontalface_alt2.xml��·��,    
	//Ҳ���԰�����ļ�������Ĺ����ļ�����Ȼ����д·����cascade_name= "haarcascade_frontalface_alt2.xml";      
	//����cascade_name ="C:\\Program Files\\OpenCV\\data\\haarcascades\\haarcascade_frontalface_alt2.xml"    
	cascade = (CvHaarClassifierCascade*)cvLoad(cascade_name, 0, 0, 0);

	if (!cascade)
	{
		fprintf(stderr, "ERROR: Could not load classifier cascade\n");
		fprintf(stderr,
			"Usage: facedetect --cascade=\"<cascade_path>\" [filename|camera_index]\n");
		return -1;
	}
	storage = cvCreateMemStorage(0);
	capture = cvCreateCameraCapture(-1);
	cvNamedWindow("result", 1);

	if (capture)
	{
		timeBegin = time(NULL);
		learn();
		for (;;)
		{
			timeEnd = time(NULL);
			timeuse = timeEnd - timeBegin;//���㾭����ʱ��,ͳ������  
			if (!cvGrabFrame(capture))
				break;
			frame = cvRetrieveFrame(capture);
			if (!frame)
				break;
			if (!frame_copy)
				frame_copy = cvCreateImage(cvSize(frame->width, frame->height),
					IPL_DEPTH_8U, frame->nChannels);
			if (frame->origin == IPL_ORIGIN_TL)//���ͼ�����������Ͻ�    
				cvCopy(frame, frame_copy, 0);
			else
				cvFlip(frame, frame_copy, 0);//���ͼ�����㲻�����Ͻǣ��������½�ʱ������X��Գ�    

			detect_and_draw(frame_copy);  //��Ⲣ��ʶ��  

			if (cvWaitKey(10) >= 0)
				break;
		}

		cvReleaseImage(&frame_copy);
		cvReleaseCapture(&capture);
	}
	else
	{
		printf("Cannot read from CAM");
		return -1;
	}

	cvDestroyWindow("result");

	return 0;
}

//ѧϰ�׶δ���  
void learn()
{
	int i, offset;

	//����ѵ��ͼ��  
	nTrainFaces = loadFaceImgArray("train.txt");
	if (nTrainFaces < 2)
	{
		fprintf(stderr,
			"Need 2 or more training faces\n"
			"Input file contains only %d\n", nTrainFaces);

		return;
	}

	// �������ɷַ���  
	doPCA();

	//��ѵ��ͼ��ͶӰ���ӿռ���  
	projectedTrainFaceMat = cvCreateMat(nTrainFaces, nEigens, CV_32FC1);
	offset = projectedTrainFaceMat->step / sizeof(float);
	for (i = 0; i<nTrainFaces; i++)
	{
		//int offset = i * nEigens;  
		cvEigenDecomposite(
			faceImgArr[i],
			nEigens,
			eigenVectArr,
			0, 0,
			pAvgTrainImg,
			//projectedTrainFaceMat->data.fl + i*nEigens);  
			projectedTrainFaceMat->data.fl + i*offset);
	}

	//��ѵ���׶εõ�������ֵ��ͶӰ��������ݴ�Ϊ.xml�ļ����Ա�����ʱʹ��  
	storeTrainingData();
}


//���ر������ѵ�����  
int loadTrainingData(CvMat ** pTrainPersonNumMat)
{
	CvFileStorage * fileStorage;
	int i;


	fileStorage = cvOpenFileStorage("facedata.xml", 0, CV_STORAGE_READ);
	if (!fileStorage)
	{
		fprintf(stderr, "Can't open facedata.xml\n");
		return 0;
	}

	nEigens = cvReadIntByName(fileStorage, 0, "nEigens", 0);
	nTrainFaces = cvReadIntByName(fileStorage, 0, "nTrainFaces", 0);
	*pTrainPersonNumMat = (CvMat *)cvReadByName(fileStorage, 0, "trainPersonNumMat", 0);
	eigenValMat = (CvMat *)cvReadByName(fileStorage, 0, "eigenValMat", 0);
	projectedTrainFaceMat = (CvMat *)cvReadByName(fileStorage, 0, "projectedTrainFaceMat", 0);
	pAvgTrainImg = (IplImage *)cvReadByName(fileStorage, 0, "avgTrainImg", 0);
	eigenVectArr = (IplImage **)cvAlloc(nTrainFaces * sizeof(IplImage *));
	for (i = 0; i<nEigens; i++)
	{
		char varname[200];
		sprintf(varname, "eigenVect_%d", i);
		eigenVectArr[i] = (IplImage *)cvReadByName(fileStorage, 0, varname, 0);
	}


	cvReleaseFileStorage(&fileStorage);

	return 1;
}

//�洢ѵ�����  
void storeTrainingData()
{
	CvFileStorage * fileStorage;
	int i;
	fileStorage = cvOpenFileStorage("facedata.xml", 0, CV_STORAGE_WRITE);

	//�洢����ֵ��ͶӰ����ƽ�������ѵ�����  
	cvWriteInt(fileStorage, "nEigens", nEigens);
	cvWriteInt(fileStorage, "nTrainFaces", nTrainFaces);
	cvWrite(fileStorage, "trainPersonNumMat", personNumTruthMat, cvAttrList(0, 0));
	cvWrite(fileStorage, "eigenValMat", eigenValMat, cvAttrList(0, 0));
	cvWrite(fileStorage, "projectedTrainFaceMat", projectedTrainFaceMat, cvAttrList(0, 0));
	cvWrite(fileStorage, "avgTrainImg", pAvgTrainImg, cvAttrList(0, 0));
	for (i = 0; i<nEigens; i++)
	{
		char varname[200];
		sprintf(varname, "eigenVect_%d", i);
		cvWrite(fileStorage, varname, eigenVectArr[i], cvAttrList(0, 0));
	}
	cvReleaseFileStorage(&fileStorage);
}

//Ѱ����ӽ���ͼ��  
int findNearestNeighbor(float * projectedTestFace)
{

	double leastDistSq = DBL_MAX;       //������С���룬����ʼ��Ϊ�����  
	int i, iTrain, iNearest = 0;

	for (iTrain = 0; iTrain<nTrainFaces; iTrain++)
	{
		double distSq = 0;

		for (i = 0; i<nEigens; i++)
		{
			float d_i =
				projectedTestFace[i] -
				projectedTrainFaceMat->data.fl[iTrain*nEigens + i];
			distSq += d_i*d_i / eigenValMat->data.fl[i];  // Mahalanobis�㷨����ľ��룬��ľ����ƽ������ƽ����������ֵ  
														  //  distSq += d_i*d_i; // Euclidean�㷨����ľ���  
		}

		if (distSq < leastDistSq)
		{
			leastDistSq = distSq;
			iNearest = iTrain;
		}
	}
	//printf("leastdistsq==%f",leastDistSq);  
	return iNearest;
}



//���ɷַ���  
void doPCA()
{
	int i;
	CvTermCriteria calcLimit;
	CvSize faceImgSize;

	// �Լ�����������ֵ����  
	nEigens = nTrainFaces - 1;

	//�������������洢�ռ�  
	faceImgSize.width = faceImgArr[0]->width;
	faceImgSize.height = faceImgArr[0]->height;
	eigenVectArr = (IplImage**)cvAlloc(sizeof(IplImage*) * nEigens);    //�������Ϊס����ֵ����  
	for (i = 0; i<nEigens; i++)
		eigenVectArr[i] = cvCreateImage(faceImgSize, IPL_DEPTH_32F, 1);

	//����������ֵ�洢�ռ�  
	eigenValMat = cvCreateMat(1, nEigens, CV_32FC1);

	// ����ƽ��ͼ��洢�ռ�  
	pAvgTrainImg = cvCreateImage(faceImgSize, IPL_DEPTH_32F, 1);

	// �趨PCA������������  
	calcLimit = cvTermCriteria(CV_TERMCRIT_ITER, nEigens, 1);//����������ΪnEigens  

															 // ����ƽ��ͼ������ֵ����������  
	cvCalcEigenObjects(
		nTrainFaces,
		(void*)faceImgArr,
		(void*)eigenVectArr,
		CV_EIGOBJ_NO_CALLBACK,
		0,
		0,
		&calcLimit,
		pAvgTrainImg,
		eigenValMat->data.fl//�洢��õ�eigenvalue  
	);

	cvNormalize(eigenValMat, eigenValMat, 1, 0, CV_L1, 0);
}



//����txt�ļ����оٵ�ͼ��  
int loadFaceImgArray(char * filename)
{
	FILE * imgListFile = 0;
	char imgFilename[512];
	int iFace, nFaces = 0;


	if (!(imgListFile = fopen(filename, "r")))
	{
		fprintf(stderr, "Can\'t open file %s\n", filename);
		return 0;
	}

	// ͳ��������  
	while (fgets(imgFilename, 512, imgListFile)) ++nFaces;//char *fgets(char *buf, int bufsize, FILE *stream);���ļ��ṹ��ָ��stream�ж�ȡ���ݣ�ÿ�ζ�ȡһ�С���ȡ�����ݱ�����bufָ����ַ������У�ÿ������ȡbufsize-1���ַ�����bufsize���ַ���'\0'��������ļ��еĸ��У�����bufsize���ַ����������оͽ��������������ȡ�ɹ����򷵻�ָ��buf��ʧ���򷵻�NULL��  
	rewind(imgListFile);//���ļ��ڲ���λ��ָ������ָ��һ������������/�ļ����Ŀ�ͷ  

						// ��������ͼ��洢�ռ������ID�Ŵ洢�ռ�  
	faceImgArr = (IplImage **)cvAlloc(nFaces * sizeof(IplImage *));
	personNumTruthMat = cvCreateMat(1, nFaces, CV_32SC1);//CvMat* cvCreateMat( int rows, int cols, int type );  

	for (iFace = 0; iFace<nFaces; iFace++)
	{
		// ���ļ��ж�ȡ��ź���������  
		fscanf(imgListFile,
			"%d %s", personNumTruthMat->data.i + iFace, imgFilename);// fscanf(FILE *stream, char *format,[argument...])�� ��: ��һ������ִ�и�ʽ������,fscanf�����ո�ͻ���ʱ����  

																	 // ��������ͼ��  
		faceImgArr[iFace] = cvLoadImage(imgFilename, CV_LOAD_IMAGE_GRAYSCALE);

		if (!faceImgArr[iFace])
		{
			fprintf(stderr, "Can\'t load image from %s\n", imgFilename);
			return 0;
		}
	}

	fclose(imgListFile);

	return nFaces;
}



//  
void printUsage()
{
	printf("Usage: eigenface <command>\n",
		"  Valid commands are\n"
		"    train\n"
		"    test\n");
}

int detect_and_draw(IplImage* img)
{
	CvFont font;
	cvInitFont(&font, CV_FONT_VECTOR0, 1, 1, 0, 1, 8);
	static CvScalar colors[] =
	{
		{ { 0,0,255 } },
		{ { 0,128,255 } },
		{ { 0,255,255 } },
		{ { 0,255,0 } },
		{ { 255,128,0 } },
		{ { 255,255,0 } },
		{ { 255,0,0 } },
		{ { 255,0,255 } }
	};

	double scale = 1.3;
	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale),
		cvRound(img->height / scale)),
		8, 1);
	int i, personnum = 0;

	cvCvtColor(img, gray, CV_BGR2GRAY);
	cvResize(gray, small_img, CV_INTER_LINEAR);
	cvEqualizeHist(small_img, small_img);
	cvClearMemStorage(storage);

	if (cascade)
	{
		double t = (double)cvGetTickCount();
		CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
			1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
			cvSize(30, 30));
		t = (double)cvGetTickCount() - t;
		//  printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );    
		IplImage* temp_img = cvCreateImage(cvSize(92, 112), 8, 1);
		for (i = 0; i < (faces ? faces->total : 0); i++)
		{
			CvRect* r = (CvRect*)cvGetSeqElem(faces, i);

			IplImage *dst = cvCreateImage(cvSize(r->width, r->height), 8, 1);//cvsizeֻ��ѡȡr->width,r->height�����ٺ���*scale��+100    
			CvPoint p1;
			p1.x = cvRound((r->x)*scale);
			p1.y = cvRound((r->y)*scale);
			CvPoint p2;
			p2.x = cvRound((r->x + r->width)*scale);
			p2.y = cvRound((r->y + r->height)*scale);
			cvRectangle(img, p1, p2, colors[i % 8], 3, 8, 0);
			cvSetImageROI(small_img, *r);
			cvCopy(small_img, dst);
			cvResize(dst, temp_img);
			cvEqualizeHist(temp_img, temp_img);
			cvResetImageROI(small_img);
			cvSaveImage(filename[i], temp_img);
			cvReleaseImage(&dst);
			//��ʼʶ��temp_img  

			personnum = recognize(temp_img);
			if (personnum == 1)
				cvPutText(img, "Yanming", cvPoint(20, 20), &font, CV_RGB(255, 255, 255));//����ȷʶ����˵�������ʾ����Ļ��  

		}
	}

	cvShowImage("result", img);
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
	return -1;
}

int recognize(IplImage *faceimg)
{
	int i, nTestFaces = 0;         // ����������  
	CvMat * trainPersonNumMat = 0;  // ѵ���׶ε�������  
	float * projectedTestFace = 0;
	// ���ر�����.xml�ļ��е�ѵ�����  
	if (!loadTrainingData(&trainPersonNumMat)) return -3;

	projectedTestFace = (float *)cvAlloc(nEigens * sizeof(float));

	int iNearest, nearest;

	//������ͼ��ͶӰ���ӿռ���  
	cvEigenDecomposite(
		faceimg,
		nEigens,
		eigenVectArr,
		0, 0,
		pAvgTrainImg,
		projectedTestFace);
	//cvNormalize(projectedTestFace, projectedTestFace, 1, 0, CV_L1, 0);  
	iNearest = findNearestNeighbor(projectedTestFace);
	nearest = trainPersonNumMat->data.i[iNearest];
	printf("nearest = %d", nearest);
	if (timeuse <= 10)
	{

		if ((nearest == 1) | (nearest == 11) | (nearest == 111))//���Ը���train.txt��ѵ��ͼƬ�ı��,���ｫ��������������������Ϊһ��  
			a1++;
		if (nearest == 2)
			a2++;
		if (nearest == 3)
			a3++;
		if (nearest == 4)
			a4++;


		if (a1>7)//���10s��ʶ��Ĵ���Ϊ6���϶�Ϊa1  
		{
			printf("yanming\n");
			return 1;
		}
		if (a2>6)
		{
			printf("others\n");
		}
		if (a3>6)
		{
			printf("ma\n");
		}
		if (a4>6)
		{
			printf("ba\n");
		}
	}
	else
	{
		timeBegin = time(NULL);
		a1 = 0;
		a2 = 0;
		a3 = 0;
		a4 = 0;
		return 0;
	}
	return -1;
}